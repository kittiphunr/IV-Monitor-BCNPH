// ทดสอบฐานบอร์ด Host-CYD28 ด่านที่ 1 บนเครื่อง PC (ไม่ต้องใช้บอร์ดจริง)
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "Arduino.h"
#include "touch_cal.h"

FILE *g_ops = nullptr;
SerialClass Serial;
ESPClass ESP;

// ---------------- แบบจำลองเวลาและชิปทัช ----------------
static const int P_CLK = 25, P_MOSI = 32, P_MISO = 39, P_CS = 33, P_IRQ = 36;
unsigned long g_ms = 100;
unsigned long millis() { return g_ms; }
unsigned long micros() { return g_ms * 1000UL; }
void delay(unsigned long ms) { g_ms += ms; }
void delayMicroseconds(unsigned long) {}
void pinMode(int, int) {}
bool ledcAttach(int, int, int) { return true; }
void ledcWrite(int, int) {}
void ledcSetup(int, int, int) {}
void ledcAttachPin(int, int) {}
void analogSetAttenuation(adc_attenuation_t) {}
void analogReadResolution(int) {}
int analogRead(int) { return 0; }
void tone(int, unsigned int, unsigned long) {}
void noTone(int) {}
long map(long x, long a, long b, long c, long d) { return (x - a) * (d - c) / (b - a) + c; }

static const float W = 320, H = 240;                        // แนวนอน
// ความจริงของบอร์ดสมมติ: แกนสลับและกลับด้าน (ผู้ใช้เคยเจอแบบนี้) raw = f(screen)
static void truthRaw(float sx, float sy, int &rx, int &ry) {
  rx = (int)((1.0f - sy / H) * 3800 + 150);
  ry = (int)((sx / W) * 3700 + 200);
}
static CalPoint targets[3];
static unsigned long simT0 = 0;
static bool simEnabled = false;
static bool penDown = false;
static bool holdPen = false;
static int penRx = 0, penRy = 0;
static int lvl[64], edge, cmdByte, outBits, outBit;

static void simUpdate() {
  if (holdPen) { penDown = true; return; }
  if (!simEnabled) { penDown = false; return; }
  const unsigned long t = g_ms - simT0, slot = 1100;        // 400 ms ปล่อย + 700 ms กด ต่อหนึ่งจุด
  const unsigned long i = t / slot, ph = t % slot;
  if (i < 3 && ph >= 400) { penDown = true; truthRaw(targets[i].sx, targets[i].sy, penRx, penRy); penRx += (int)(g_ms % 5) - 2; }
  else penDown = false;
}
static int chan(int c) {
  switch (c & 0x70) {
    case 0x50: return penRx;
    case 0x10: return penRy;
    case 0x30: return 600;
    case 0x40: return 3600;               // z = 600 + 4095 - 3600 = 1095
  }
  return 0;
}
void digitalWrite(int pin, int v) {
  int old = lvl[pin]; lvl[pin] = v;
  if (pin == P_CS && v == 0) { edge = 0; cmdByte = 0; outBit = 0; }
  if (pin == P_CLK && v == 1 && old == 0 && lvl[P_CS] == 0) {
    edge++;
    if (edge <= 8) cmdByte = (cmdByte << 1) | lvl[P_MOSI];
    if (edge == 8) outBits = chan(cmdByte) & 0xFFF;
  }
  if (pin == P_CLK && v == 0 && old == 1 && lvl[P_CS] == 0) {
    if (edge >= 9 && edge <= 20) outBit = (outBits >> (20 - edge)) & 1; else outBit = 0;
  }
}
int digitalRead(int pin) {
  if (pin == P_IRQ) { simUpdate(); return penDown ? 0 : 1; }
  if (pin == P_MISO) return outBit;
  return 1;                                // BOOT ไม่กด
}

#include "sketch.cpp"

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { printf("ล้มเหลว: %s (บรรทัด %d)\n", msg, __LINE__); fails++; } } while (0)

static void testCalMath() {
  // ความจริงสามแบบ: แกนตรง / สลับแกน / กลับด้านทั้งสองแกน
  for (int model = 0; model < 3; model++) {
    CalPoint p[3];
    touchCalTargets((int)W, (int)H, p);
    for (int i = 0; i < 3; i++) {
      float u = p[i].sx / W, v = p[i].sy / H;
      float rx, ry;
      if (model == 0) { rx = u * 3800 + 150; ry = v * 3700 + 200; }
      else if (model == 1) { rx = (1 - v) * 3800 + 150; ry = u * 3700 + 200; }
      else { rx = (1 - u) * 3800 + 150; ry = (1 - v) * 3700 + 200; }
      p[i].rx = rx; p[i].ry = ry;
    }
    TouchCal k;
    CHECK(touchCalSolve(p, (int)W, (int)H, k), "แก้สมการไม่ได้");
    float worst = 0;
    for (int t = 0; t < 50; t++) {
      float u = (t % 10) / 9.0f, v = (t / 10) / 4.0f, rx, ry;
      if (model == 0) { rx = u * 3800 + 150; ry = v * 3700 + 200; }
      else if (model == 1) { rx = (1 - v) * 3800 + 150; ry = u * 3700 + 200; }
      else { rx = (1 - u) * 3800 + 150; ry = (1 - v) * 3700 + 200; }
      int16_t sx, sy;
      touchCalApply(k, rx, ry, (int)W, (int)H, sx, sy);
      float dx = fabsf(sx - u * (W - 1)), dy = fabsf(sy - v * (H - 1));
      if (dx > worst) worst = dx;
      if (dy > worst) worst = dy;
    }
    CHECK(worst < 24.0f, "ความคลาดเคลื่อนเกินกำหนด (จุดนอกกรอบเป้าหมายถูกประมาณค่าเพิ่ม)");
    printf("  แบบจำลอง %d: ผิดพลาดสูงสุด %.1f พิกเซลทั่วทั้งจอ\n", model, worst);
  }
  // จุดสามจุดในแนวเดียวกัน ต้องถูกปฏิเสธ
  CalPoint bad[3] = {{500, 500, 48, 36}, {1500, 1500, 272, 48}, {2500, 2500, 160, 204}};
  TouchCal k;
  CHECK(!touchCalSolve(bad, (int)W, (int)H, k), "ต้องปฏิเสธจุดที่อยู่แนวเดียวกัน");
  // ค่าดิบนิ่งเกินไป (แตะที่เดิมสามครั้ง) ต้องถูกปฏิเสธ
  CalPoint same[3] = {{2000, 2000, 48, 36}, {2001, 2002, 272, 48}, {2002, 2001, 160, 204}};
  CHECK(!touchCalSolve(same, (int)W, (int)H, k), "ต้องปฏิเสธเมื่อค่าดิบแทบไม่ต่างกัน");
}

static void testMedianAndTracker() {
  int16_t a[5] = {900, 10, 12, 11, 4000};
  CHECK(touchMedian5(a) == 12, "ค่ากลางต้องตัดค่าโดดสองข้าง");
  TouchTracker t;
  uint32_t ms = 1000;
  CHECK(t.update(true, ms) == TOUCH_NONE, "การกดครั้งแรกยังไม่นับ");
  CHECK(t.update(true, ms += 4) == TOUCH_DOWN, "กดครั้งที่สองต้องเป็น DOWN");
  CHECK(t.update(true, ms += 4) == TOUCH_MOVE, "ลากต้องเป็น MOVE");
  CHECK(t.update(false, ms += 10) == TOUCH_NONE, "สะดุดสั้นไม่ใช่ UP");
  CHECK(t.update(true, ms += 10) == TOUCH_MOVE, "กลับมากดต่อได้เป็น MOVE ไม่เกิด DOWN ซ้ำ");
  CHECK(t.update(false, ms += 10) == TOUCH_NONE, "ยังไม่ครบเวลา");
  CHECK(t.update(false, ms += 60) == TOUCH_UP, "หายเกินเวลาต้องเป็น UP");
  CHECK(t.update(true, ms += 4) == TOUCH_NONE, "กดใหม่ต้องรอสองครั้งอีก");
  TouchTracker t2;                                     // สัญญาณกระตุกครั้งเดียวต้องไม่เกิดเหตุการณ์
  CHECK(t2.update(true, 0) == TOUCH_NONE && t2.update(false, 5) == TOUCH_NONE && t2.update(false, 100) == TOUCH_NONE, "สัญญาณกระตุกครั้งเดียวต้องถูกกรอง");
}

static void testDriverAndSketch() {
  g_ops = fopen("/dev/null", "w");
  touchChip.begin();
  penDown = false;
  simEnabled = false;
  int16_t x, y, z;
  CHECK(!touchChip.read(x, y, z), "ไม่แตะต้องไม่ขึ้นแตะ");
  // ให้ชิปจำลองแตะเป้าหมายของหน้าคาลิเบรตเอง แล้วเรียก setup() ของสเก็ตช์จริงทั้งไฟล์
  touchCalTargets((int)W, (int)H, targets);
  simT0 = g_ms; simEnabled = true;
  setup();
  CHECK(cal.valid, "setup() ต้องคาลิเบรตสำเร็จเมื่อยังไม่มีค่าในหน่วยความจำ");
  float worst = 0;
  for (int i = 0; i < 3; i++) {
    int rx, ry; truthRaw(targets[i].sx, targets[i].sy, rx, ry);
    int16_t sx, sy; touchCalApply(cal, (float)rx, (float)ry, (int)W, (int)H, sx, sy);
    worst = fmaxf(worst, fmaxf(fabsf(sx - targets[i].sx), fabsf(sy - targets[i].sy)));
  }
  CHECK(worst < 4.0f, "จุดเป้าหมายต้องแม่นภายใน 4 พิกเซล แม้ค่าดิบมีสัญญาณรบกวน");
  printf("  คาลิเบรตผ่านสเก็ตช์จริง ผิดพลาดที่จุดเป้าหมาย %.1f พิกเซล\n", worst);
  simEnabled = false;
  for (int i = 0; i < 200; i++) { g_ms += 5; loop(); }   // วนหลักไม่ค้าง ไม่มีนิ้วแตะ
  // แตะปุ่ม + ที่ตำแหน่งจริง ต้องเพิ่มความสว่าง ปุ่ม - ต้องลดลง
  int rx, ry;
  brightness = 50;
  truthRaw(buttons[3].x + buttons[3].w / 2, buttons[3].y + buttons[3].h / 2, rx, ry);
  penRx = rx; penRy = ry; holdPen = true;
  for (int i = 0; i < 12; i++) { g_ms += 5; loop(); }
  holdPen = false;
  for (int i = 0; i < 30; i++) { g_ms += 5; loop(); }
  CHECK(brightness == 60, "แตะปุ่ม + ต้องเพิ่มความสว่างเป็น 60");
  truthRaw(buttons[2].x + buttons[2].w / 2, buttons[2].y + buttons[2].h / 2, rx, ry);
  penRx = rx; penRy = ry; holdPen = true;
  for (int i = 0; i < 12; i++) { g_ms += 5; loop(); }
  holdPen = false;
  for (int i = 0; i < 30; i++) { g_ms += 5; loop(); }
  CHECK(brightness == 50, "แตะปุ่ม - ต้องลดความสว่างเป็น 50");
  fclose(g_ops);
}

int main() {
  printf("ทดสอบคณิตศาสตร์คาลิเบรต\n");  testCalMath();
  printf("ทดสอบตัวกรองและตัวติดตามนิ้ว\n"); testMedianAndTracker();
  printf("ทดสอบไดรเวอร์และสเก็ตช์ (CYD_PANEL=%d)\n", CYD_PANEL); testDriverAndSketch();
  if (fails) { printf("รวมล้มเหลว %d รายการ\n", fails); return 1; }
  printf("ผ่านทั้งหมด\n");
  return 0;
}
