// ทดสอบตัวคำนวณอัตราไหลของ Host (rate_estimator.h) เทียบกับวิธีเดิมที่ Station 7.7.3 ใช้
// จำลองสายน้ำเกลือที่หยดตามเวลาจริง ส่งแพ็กเก็ตวินาทีละใบแบบเดียวกับ Station แล้วดูว่า Host ได้ตัวเลขตรงกันไหม
#include "rate_estimator.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <random>
#include <algorithm>

static int failures = 0;
static void check(bool ok, const char* what) {
  printf("  %-72s %s\n", what, ok ? "ผ่าน" : "ไม่ผ่าน");
  if (!ok) failures++;
}

// ---- วิธีเดิมของ Station 7.7.3 (หน้าต่าง 8 หยด จากเวลาของทุกหยด) ใช้เป็นเกณฑ์เทียบ ----
struct StationWindow {
  uint32_t t[8]; uint8_t idx = 0, cnt = 0;
  void reset() { idx = 0; cnt = 0; }
  void push(uint32_t x) { t[idx] = x; idx = (idx + 1) % 8; if (cnt < 8) cnt++; }
  float rate(int df) const {
    if (cnt < 2) return 0;
    uint8_t oldest = (idx + 8 - cnt) % 8, newest = (idx + 8 - 1) % 8;
    uint32_t span = t[newest] - t[oldest];
    return span ? ((float)(cnt - 1) / df) * 3600000.0f / span : 0;
  }
};

struct Sim {
  std::mt19937 rng{12345};
  int df = 20;
  StationWindow ref;
  RateEstimator est;
  uint32_t drops = 0, lastDrop = 0; bool seen = false;
  uint32_t now = 100000;
  double nextDrop = 0;
  float refRate = 0;
  // ส่งแพ็กเก็ตทุก 1 วินาที (บวกสั่น +-100 ms) หายตาม lossPct
  void run(double mlPerHr, double jitterPct, int lossPct, int seconds, bool running = true) {
    double meanMs = 3600000.0 / (mlPerHr * df);
    std::normal_distribution<double> noise(0, jitterPct / 100.0);
    if (nextDrop == 0) nextDrop = now + meanMs;
    for (int s = 0; s < seconds; s++) {
      uint32_t end = now + 1000;
      while (running && nextDrop <= end) {
        uint32_t td = (uint32_t)nextDrop;
        drops++; lastDrop = td; seen = true;
        // Station: ล้างหน้าต่างเมื่ออัตราเปลี่ยนก้าวกระโดด (เหมือน onDropDetected เดิม)
        if (ref.cnt >= 2) {
          uint32_t span = ref.t[(ref.idx + 7) % 8] - ref.t[(ref.idx + 8 - ref.cnt) % 8];
          uint32_t mean = span / (ref.cnt - 1), dt = td - ref.t[(ref.idx + 7) % 8];
          if (mean > 0 && (dt > mean * 3 || dt * 3 < mean)) ref.reset();
        }
        ref.push(td);
        float r = ref.rate(df); if (r > 0) refRate = r;
        double iv = meanMs * (1.0 + noise(rng)); if (iv < 80) iv = 80;
        nextDrop += iv;
      }
      now = end;
      if ((int)(rng() % 100) < lossPct) continue;
      uint32_t rx = now + (rng() % 7);                         // ESP-NOW ใช้เวลาส่ง 0-6 ms
      uint32_t since = seen ? (now - lastDrop) : 0;            // Station วัด ณ ตอนส่ง (now)
      if (seen && since == 0) since = 1;
      est.update(rx, drops, since, running, df);
    }
  }
};

int main() {
  printf("1) สภาวะคงที่: ค่าที่ Host คำนวณเทียบกับที่ Station เคยคำนวณเอง\n");
  struct Case { double ml; double jit; int loss; const char* name; };
  Case cases[] = {
    {3,    10, 0,  "3 mL/h (หนึ่งหยดต่อนาที)"},
    {30,   10, 0,  "30 mL/h"},
    {150,  15, 0,  "150 mL/h หยดไม่สม่ำเสมอ"},
    {600,  10, 0,  "600 mL/h"},
    {1200, 5,  0,  "1200 mL/h (หลายหยดต่อแพ็กเก็ต)"},
    {150,  15, 30, "150 mL/h แพ็กเก็ตหาย 30%"},
    {30,   15, 50, "30 mL/h แพ็กเก็ตหาย 50%"},
  };
  for (auto& c : cases) {
    Sim s; s.run(c.ml, c.jit, c.loss, c.ml < 10 ? 3600 : 600);
    float host = s.est.rateHr, ref = s.refRate;
    double errTrue = fabs(host - c.ml) / c.ml * 100.0;
    double errRef = fabs(host - ref) / ref * 100.0;
    char buf[160];
    snprintf(buf, sizeof(buf), "%s: Host %.1f | Station เดิม %.1f | จริง %.0f (ต่างจากเดิม %.1f%%)", c.name, host, ref, c.ml, errRef);
    check(s.est.valid && errTrue < 12.0 && errRef < 6.0, buf);
  }

  printf("\n2) ไม่มีข้อมูลก็ไม่แต่งตัวเลข\n");
  { Sim s; check(s.est.update(1000, 0, 0, true, 20) == 0 && !s.est.valid, "ยังไม่มีหยดเลย = 0 และไม่ valid"); }
  { RateEstimator e; e.update(5000, 1, 100, true, 20); check(!e.valid, "หยดเดียว (จุดยึดเดียว) ยังคำนวณไม่ได้"); }

  printf("\n3) หยุดชั่วคราว / เริ่มถุงใหม่ / รีบูต\n");
  { Sim s; s.run(150, 10, 0, 120);
    check(s.est.valid && s.est.rateHr > 100, "ไหลปกติได้ค่า");
    s.run(150, 10, 0, 5, false);
    check(s.est.rateHr == 0 && !s.est.valid, "หยุดชั่วคราว -> อัตราเป็น 0");
    s.nextDrop = 0; s.run(150, 10, 0, 60);
    check(s.est.valid && fabs(s.est.rateHr - 150) / 150 < 0.12, "กลับมานับ -> คำนวณใหม่ได้ ไม่ปนช่วงที่หยุด"); }
  { Sim s; s.run(150, 10, 0, 120);
    uint32_t keep = s.drops; s.drops = 0; s.lastDrop = s.now; s.nextDrop = s.now + 400;   // Station รีบูต ตัวนับเริ่มจาก 0
    s.est.update(s.now + 3, 0, 0, true, 20);
    check(s.est.rateHr == 0, "ตัวนับถอยหลัง (รีบูต/เริ่มถุงใหม่) -> ล้างอัตรา");
    (void)keep; }

  printf("\n4) เปลี่ยนอัตราก้าวกระโดด (หมุนโรลเลอร์แคลมป์)\n");
  { Sim s; s.run(60, 5, 0, 300);
    s.run(240, 5, 0, 40);
    check(fabs(s.est.rateHr - 240) / 240 < 0.15, "ค่าตามอัตราใหม่ภายใน 40 วินาที ไม่ถูกค่าเก่าถ่วง");
    s.run(30, 5, 0, 200);
    check(fabs(s.est.rateHr - 30) / 30 < 0.2, "ลดอัตราลงก็ตามทัน"); }

  printf("\n5) ค่านอกปกติ\n");
  { RateEstimator e; uint32_t now = 100000; uint32_t tot = 0;
    for (int i = 0; i < 10; i++) { now += 1000; tot += 3; e.update(now, tot, 100, true, 20); }
    float before = e.rateHr;
    e.update(now + 1000, tot + 3, 100, true, 0);                // dropFactor 0 ต้องไม่หารศูนย์
    check(std::isfinite(e.rateHr) && before > 0, "dropFactor เป็น 0 ไม่ทำให้ล้ม");
    e.update(now + 1000, tot + 3, 5000000, true, 20);           // msSince เกินเวลาที่ผ่านมา (ย้อนเวลา)
    check(std::isfinite(e.rateHr), "msSinceLastDrop ใหญ่ผิดปกติไม่ทำให้ล้ม"); }
  { Sim s; s.run(150, 10, 0, 200); uint32_t before = s.est.n;
    for (int i = 0; i < 20; i++) s.est.update(s.now + 1000 * i, s.drops, s.now - s.lastDrop + 1000 * i, true, 20);
    check(s.est.n == before, "แพ็กเก็ตซ้ำที่ไม่มีหยดใหม่ไม่เพิ่มจุดยึด"); }

  printf("\n%s\n", failures ? "มีข้อที่ไม่ผ่าน" : "ผ่านทุกข้อ");
  return failures ? 1 : 0;
}
