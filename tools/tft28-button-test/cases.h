// ทดสอบปุ่มเดียวและบัซเซอร์ของ Host-TFT28: ป้อนระดับขาปุ่ม (ขา 4) ตามจังหวะเวลา แล้วดูสถานะและเสียงที่ออกจริง
// ใช้ได้ทั้ง BUZZER_ACTIVE 0 (Passive: tone) และ 1 (Active: เปิด/ปิดขา)
#include <vector>
#include <cstdlib>
struct Beep { unsigned long start, dur; int hz; };
static std::vector<Beep> g_beeps;
static bool bzOn = false; static unsigned long bzStart = 0, bzEndAt = 0; static int bzHz = 0;
int g_btn = 1;                // ระดับขาปุ่ม (1 = ปล่อย, 0 = กด)
unsigned long g_pressStart = 0;
int g_sleepCalls = 0;
static int bzBad = 0;
#define TCK(c, m) do { if (!(c)) { printf("ผิด: %s\n", m); bzBad++; } else printf("ผ่าน: %s\n", m); } while (0)

static void bzFinish(unsigned long t) { if (bzOn) { g_beeps.push_back({bzStart, (t < bzEndAt || bzEndAt == 0) ? t - bzStart : bzEndAt - bzStart, bzHz}); bzOn = false; } }
static void bzFlushTimed() { if (bzOn && bzEndAt && g_millis >= bzEndAt) bzFinish(g_millis); }   // tone(f, ms) หมดเวลาเอง
void bzTone(int hz, unsigned long ms) { bzFlushTimed(); bzFinish(g_millis); bzOn = true; bzStart = g_millis; bzEndAt = ms ? g_millis + ms : 0; bzHz = hz; }
void bzStopEv() { bzFinish(g_millis); }
void bzLevel(bool on) { bzFlushTimed(); if (on && !bzOn) { bzOn = true; bzStart = g_millis; bzEndAt = 0; bzHz = 0; } else if (!on) bzFinish(g_millis); }

static void step(unsigned long ms) {          // เดินเวลาทีละ 5 ms พร้อมเรียกตัวจัดการปุ่มและบัซเซอร์เหมือน loop()
  for (unsigned long t = 0; t < ms; t += 5) {
    g_millis += 5;
    bzFlushTimed();
    checkMultifunctionButton();
    serviceBuzzer();
    handleBuzzerAlarm();
  }
}
static void press(unsigned long ms) { g_btn = 0; g_pressStart = g_millis; step(ms); g_btn = 1; }
static void idle(unsigned long ms) { g_btn = 1; step(ms); }
static void clickOnce() { press(80); }
static void resetState() {
  g_beeps.clear(); bzOn = false;
  manualFocusBed = -1; screensaverActive = false; displaySleeping = false;
  globalAlarmTriggered = false; globalSnoozeUntil = 0; isPowerOffProgressActive = false;
  for (int i = 0; i < MAX_SUPPORTED_STATIONS; i++) { stations[i].alertCode = ALERT_NONE; stations[i].nearEndAck = false; }
  idle(1000);
}
static int bzRun() {
  g_ops = fopen("/dev/null", "w");
  activeStationCount = 5;
  for (int i = 0; i < 5; i++) { stations[i].active = true; stations[i].lastRecvTime = g_millis; }
  resetState();

  // ---- คลิกเดียว = เตียงถัดไป ----
  int base = currentFocusBed();
  clickOnce(); idle(400);
  TCK(manualFocusBed == (base + 1) % 5, "คลิกเดียว: เลื่อนไปเตียงถัดไป");
  clickOnce(); idle(400);
  TCK(manualFocusBed == (base + 2) % 5, "คลิกเดียวอีกครั้ง: เลื่อนต่ออีกเตียง");
  TCK(!screensaverActive && !displaySleeping, "คลิกเดียว: ไม่เข้าโหมดนาฬิกาและไม่ดับจอ");

  // ---- ดับเบิลคลิก = โหมดนาฬิกา (ไม่เลื่อนเตียง) ----
  resetState();
  clickOnce(); idle(100); clickOnce(); idle(400);
  TCK(screensaverActive && manualFocusBed == -1, "ดับเบิลคลิก: เข้าโหมดนาฬิกา ไม่เลื่อนเตียง");
  clickOnce(); idle(400);
  TCK(!screensaverActive, "คลิกอีกครั้งในโหมดนาฬิกา: ออกจากโหมดนาฬิกา");

  // ---- ทริปเปิลคลิก = ปิด/เปิดจอ ----
  resetState();
  clickOnce(); idle(100); clickOnce(); idle(100); clickOnce(); idle(400);
  TCK(displaySleeping && !screensaverActive && manualFocusBed == -1, "ทริปเปิลคลิก: ปิดจอ ไม่เลื่อนเตียง ไม่เข้าโหมดนาฬิกา");
  clickOnce(); idle(400);
  TCK(!displaySleeping, "คลิกขณะจอดับ: ปลุกจอ");

  // ---- เหตุวิกฤต: พักเสียงทันที ----
  resetState();
  globalAlarmTriggered = true; globalSnoozeUntil = 0;
  press(80); idle(60);                                   // ปล่อยแล้วผ่านไปแค่ 60 ms (น้อยกว่าหน้าต่างนับคลิก 280 ms)
  TCK(isSnoozed(), "มีเสียงเตือนวิกฤต: กดสั้นแล้วพักเสียงทันที ไม่รอหน้าต่างนับคลิก");
  TCK(manualFocusBed == -1, "พักเสียง: ไม่ถูกนับเป็นคลิกเลื่อนเตียง");
  globalAlarmTriggered = false; globalSnoozeUntil = 0;

  // ---- เตือนใกล้หมด: รับทราบทันที ----
  resetState();
  stations[0].alertCode = ALERT_NEAR_END; stations[0].nearEndAck = false;
  press(80); idle(60);
  TCK(stations[0].nearEndAck, "มีเตือนใกล้หมด: กดสั้นแล้วรับทราบทันที");
  TCK(manualFocusBed == -1, "รับทราบ: ไม่ถูกนับเป็นคลิกเลื่อนเตียง");

  // ---- กดค้าง: ปล่อยก่อนครบ = ยกเลิก / ครบ 2 วินาที = ปิดเครื่อง ----
  resetState(); g_sleepCalls = 0;
  press(800); idle(300);
  TCK(!isPowerOffProgressActive && g_sleepCalls == 0, "กดค้าง 0.8 วินาทีแล้วปล่อย: ยกเลิกการปิดเครื่อง ไม่หลับ");
  TCK(manualFocusBed == -1 && !screensaverActive, "กดค้างที่ยกเลิก: ไม่ถูกนับเป็นคลิก");
  resetState(); g_sleepCalls = 0;
  g_btn = 0; g_pressStart = g_millis; step(2300); g_btn = 1;
  TCK(g_sleepCalls == 1, "กดค้างเกิน 2 วินาที: ปิดเครื่อง (เข้า deep sleep หนึ่งครั้ง)");

  // ---- เสียง ----
  resetState(); g_beeps.clear();
  clickOnce(); idle(400);
  bzFlushTimed();
  TCK(g_beeps.size() == 1, "คลิกเดียว: มีเสียงยืนยันหนึ่งครั้ง");
#if BUZZER_ACTIVE
  TCK(g_beeps.size() == 1 && g_beeps[0].dur >= 30 && g_beeps[0].dur <= 45, "Active: เสียงคลิกเปิดไม่ต่ำกว่า 30 ms แล้วปิดเอง");
  TCK(!bzOn, "Active: ปิดขาเมื่อจบ");
#else
  TCK(g_beeps.size() == 1 && g_beeps[0].hz == 2400 && g_beeps[0].dur == 15, "Passive: เสียงคลิกโทน 2400 Hz นาน 15 ms (เหมือนรุ่นก่อน)");
#endif

  resetState(); g_beeps.clear();
  playWelcomeMelody(); bzFlushTimed();
  TCK(g_beeps.size() == 4, "เมโลดี้เปิดเครื่อง: 4 โน้ต");
  {
    const int dur[4] = {80, 80, 80, 200}, hz[4] = {523, 659, 784, 1046};
    bool ok = g_beeps.size() == 4;
    for (size_t i = 0; ok && i < 4; i++) {
      ok = labs((long)g_beeps[i].dur - dur[i]) <= 5;
#if !BUZZER_ACTIVE
      ok = ok && g_beeps[i].hz == hz[i];
#endif
    }
    TCK(ok, "เมโลดี้เปิดเครื่อง: ความยาวโน้ตเท่าเดิม (Passive มีโทนตรงเดิมด้วย)");
  }
  TCK(!bzOn, "เมโลดี้ทิ้งขาบัซเซอร์ในสถานะปิด");

  resetState(); g_beeps.clear();
  globalAlarmTriggered = true; globalSnoozeUntil = 0;
  g_btn = 1; step(3200);
  bzFlushTimed();
  TCK(g_beeps.size() >= 2, "เสียงเตือนวิกฤต: บี๊ปคู่ (อย่างน้อยสองครั้งใน 3.2 วินาที)");
  if (g_beeps.size() >= 2) TCK(labs((long)(g_beeps[1].start - g_beeps[0].start) - 200) <= 15 && labs((long)g_beeps[0].dur - 100) <= 10, "เสียงเตือนวิกฤต: บี๊ป 100 ms สองครั้งห่าง 200 ms เหมือนเดิม");
  globalAlarmTriggered = false;
  return bzBad ? 1 : 0;
}
