// ทดสอบจังหวะเสียงเตือนของ Station ทั้งโหมด Passive (tone) และ Active (เปิด/ปิดขา)
// จับเหตุการณ์ที่ขาบัซเซอร์ (เวลา เปิด/ปิด ความถี่) ขณะเรียก handlePassiveBuzzerEngine ทุก 5 ms เป็นเวลา 12 วินาทีต่อสัญญาณ
#include <vector>
#include <cstdlib>
struct BzEv { unsigned long t; bool on; int hz; };
static std::vector<BzEv> g_bz;
static int bzBad = 0;
#define BCK(c, m) do { if (!(c)) { printf("ผิด: %s\n", m); bzBad++; } else printf("ผ่าน: %s\n", m); } while (0)

struct Beep { unsigned long start, dur; int hz; };
static std::vector<Beep> beepsOf() {          // แปลงเหตุการณ์เป็นรายการช่วงที่เสียงดัง
  std::vector<Beep> out; bool on = false; unsigned long st = 0; int hz = 0;
  for (auto &e : g_bz) {
    if (e.on && !on) { on = true; st = e.t; hz = e.hz; }
    else if (!e.on && on) { on = false; out.push_back({st, e.t - st, hz}); }
  }
  return out;
}
static std::vector<Beep> runAlert(uint8_t code, unsigned long ms = 12000) {
  g_bz.clear();
  buzzerBeepUntil = 0; buzzerNextBeepTime = 0; snoozeUntilTime = 0; nearChimeRemaining = 0; nearNextChimeTime = 0;
  unsigned long t0 = g_millis;
  while (g_millis - t0 < ms) { handlePassiveBuzzerEngine(code); g_millis += 5; }
  return beepsOf();
}
// จัดกลุ่มเป็นชุด (burst): ช่วงที่ห่างกันเกิน 500 ms ถือเป็นชุดใหม่ คืนจำนวนครั้งต่อชุด ความยาวครั้งแรก และคาบของชุด
struct Sig { int n; unsigned long on; unsigned long period; int hz; };
static Sig sigOf(const std::vector<Beep> &b) {
  Sig s{0, 0, 0, 0}; if (b.empty()) return s;
  s.on = b[0].dur; s.hz = b[0].hz;
  size_t i = 0; s.n = 1;
  while (i + 1 < b.size() && b[i + 1].start - (b[i].start + b[i].dur) < 500) { s.n++; i++; }
  size_t j = i + 1;
  if (j < b.size()) s.period = b[j].start - b[0].start;     // ต้นชุดแรกถึงต้นชุดถัดไป
  return s;
}
static int bzRun() {
  g_ops = fopen("/dev/null", "w");
  const uint8_t codes[5] = { ALERT_OCCLUSION, ALERT_TOO_FAST, ALERT_TOO_SLOW, ALERT_COMPLETE, ALERT_NO_SIGNAL };
  const char *names[5] = { "สายพับ", "เร็วเกิน", "ช้าเกิน", "ให้ครบ", "NO DROP" };
  Sig sg[5];
  for (int i = 0; i < 5; i++) { sg[i] = sigOf(runAlert(codes[i])); printf("  %-8s ครั้ง/ชุด=%d ยาว=%lu ms คาบ=%lu ms ความถี่=%d\n", names[i], sg[i].n, sg[i].on, sg[i].period, sg[i].hz); }
#if BUZZER_ACTIVE
  // ค่าตามที่ออกแบบ (ยอมคลาด 15 ms จากขั้นเวลา 5 ms ของตัวจำลอง)
  const int wantN[5]   = { 3, 4, 1, 2, 2 };
  const int wantOn[5]  = { 150, 60, 350, 250, 100 };
  const int wantPer[5] = { 150+100+150+100+150+900, 60*4+60*3+900, 350+1200, 250+200+250+1800, 100+100+100+1500 };
  for (int i = 0; i < 5; i++) {
    char m[120];
    snprintf(m, sizeof(m), "Active %s: %d ครั้ง/ชุด ครั้งละ %d ms คาบ %d ms", names[i], wantN[i], wantOn[i], wantPer[i]);
    BCK(sg[i].n == wantN[i] && labs((long)sg[i].on - wantOn[i]) <= 15 && labs((long)sg[i].period - wantPer[i]) <= 25, m);
  }
  bool distinct = true;
  for (int i = 0; i < 5; i++) for (int j = i + 1; j < 5; j++)
    if (sg[i].n == sg[j].n && labs((long)sg[i].on - (long)sg[j].on) < 40) distinct = false;
  BCK(distinct, "Active: ทั้งห้าสัญญาณแยกกันได้ด้วยจังหวะ (จำนวนครั้งหรือความยาวต่างกัน)");
  auto none = runAlert(ALERT_NONE);
  BCK(none.empty(), "Active: ไม่มีสัญญาณเตือนต้องเงียบ");
  BCK(g_bz.empty() || !g_bz.back().on, "Active: ปิดขาเมื่อจบ");
  // ช่วงสงบเงียบ (snooze) ต้องไม่ดัง
  g_bz.clear(); buzzerBeepUntil = 0; buzzerNextBeepTime = 0; snoozeUntilTime = g_millis + 5000;
  unsigned long t0 = g_millis; while (g_millis - t0 < 4000) { handlePassiveBuzzerEngine(ALERT_OCCLUSION); g_millis += 5; }
  BCK(beepsOf().empty(), "Active: ขณะพักเสียง (snooze) ไม่ดัง");
  auto ne = runAlert(ALERT_NEAR_END, 2000);
  BCK(ne.size() == 3, "Active: เตือนใกล้หมด 3 ครั้งสั้น ๆ");
#else
  const int wantHz[5] = { 1800, 2200, 1200, 1500, 1400 };
  const int wantOn[5] = { 150, 80, 120, 250, 100 };
  const int wantPer[5] = { 900, 700, 1200, 1500, 1500 };
  for (int i = 0; i < 5; i++) {
    char m[120];
    snprintf(m, sizeof(m), "Passive %s: โทน %d Hz ครั้งละ %d ms คาบ %d ms (เหมือนรุ่นก่อน)", names[i], wantHz[i], wantOn[i], wantPer[i]);
    BCK(sg[i].n == 1 && sg[i].hz == wantHz[i] && labs((long)sg[i].on - wantOn[i]) <= 15 && labs((long)sg[i].period - wantPer[i]) <= 15, m);
  }
  BCK(runAlert(ALERT_NONE).empty(), "Passive: ไม่มีสัญญาณเตือนต้องเงียบ");
#endif
  // ---- NO DROP (รหัส 6 จาก Host): ตัดสินที่เครื่องประจำเตียงอย่างไร ----
  {
    isRunning = true; isHostOnline = true; hostAlertCode = ALERT_NO_SIGNAL; hasFirstDropOccurred = false;
    BCK(effectiveAlertCode() == ALERT_NO_SIGNAL, "NO DROP: Host ส่งรหัส 6 ขณะยังไม่เคยเห็นหยด ต้องเตือน");
    BCK(isCriticalAlert(ALERT_NO_SIGNAL), "NO DROP: เป็นเหตุวิกฤต (จอเตือนเต็มจอ ไฟแดง snooze ได้)");
    BCK(uiStatusOf(ALERT_NO_SIGNAL) == UI_NODROP && strcmp(uiStatusWord(UI_NODROP), "NO DROP") == 0, "NO DROP: สถานะและคำบนจอ");
    BCK(uiStatusColor(UI_NODROP) == COLOR_RED, "NO DROP: สีแดง");
    hasFirstDropOccurred = true; lastDropTimestamp = millis();      // เพิ่งเห็นหยด (ไม่งั้นจะกลายเป็นสายพับในเครื่อง)
    BCK(effectiveAlertCode() == ALERT_NONE, "NO DROP: เห็นหยดในเครื่องแล้วต้องเลิกเตือน (ข้อมูลในเครื่องใหม่กว่า)");
    hasFirstDropOccurred = false; isRunning = false;
    BCK(effectiveAlertCode() == ALERT_NONE, "NO DROP: หยุดชั่วคราวต้องไม่เตือน");
    isRunning = true; isHostOnline = false;
    BCK(effectiveAlertCode() == ALERT_NONE, "NO DROP: Host ไม่อยู่ ห้ามเตือนจากค่าเก่าที่ค้าง");
    isHostOnline = true; hostAlertCode = ALERT_NONE; hasFirstDropOccurred = false;
    BCK(effectiveAlertCode() == ALERT_NONE, "NO DROP: Host ส่ง 0 ต้องไม่เตือน");
    BCK(MSG_FLAG_NO_SIGNAL_OK == 0x02, "NO DROP: บิตที่ประกาศความสามารถต้องเป็น 0x02 ตรงกับ Host");
  }
#ifdef HAS_CALBEEP
  // เสียงบี๊ปตอนคาลิเบรต (เฉพาะสาย Station-Stable): เปิดแล้วต้องปิดเองเมื่อครบเวลา ทั้ง Passive และ Active
  g_bz.clear(); calBeepUntil = 0;
  calBeep(2000, 100);
  unsigned long c0 = g_millis;
  while (g_millis - c0 < 300) { calServiceBeep(); g_millis += 5; }
  auto cb = beepsOf();
  BCK(cb.size() == 1 && cb[0].dur >= 95 && cb[0].dur <= 110, "เสียงบี๊ปคาลิเบรตเปิดราว 100 ms แล้วปิดเอง");
#if !BUZZER_ACTIVE
  BCK(cb.size() == 1 && cb[0].hz == 2000, "เสียงบี๊ปคาลิเบรต Passive ใช้โทน 2000 Hz เหมือนเดิม");
#endif
#endif
  return bzBad ? 1 : 0;
}
