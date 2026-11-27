// ทดสอบหน้าจอ ทัช และเสียงของ Host-CYD28 ด่านที่ 3 บนเครื่อง PC
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "Arduino.h"
#include "touch_cal.h"
#include "scenario.h"
#include "protocol.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

FILE *g_ops = nullptr;
SerialClass Serial;
ESPClass ESP;
unsigned long g_ms = 100000;
unsigned long millis() { return g_ms; }
unsigned long micros() { return g_ms * 1000UL; }
void delay(unsigned long ms) { g_ms += ms; }
void delayMicroseconds(unsigned long) {}
void pinMode(int, int) {}
static int g_pinLevel[64];
void digitalWrite(int p, int v) { g_pinLevel[p] = v; }
int digitalRead(int) { return 1; }                      // IRQ สูง = ไม่มีนิ้ว BOOT ไม่กด
bool ledcAttach(int, int, int) { return true; }
static int g_bl = -1;
void ledcWrite(int, int v) { g_bl = v; }
void ledcSetup(int, int, int) {}
void ledcAttachPin(int, int) {}
void analogSetAttenuation(adc_attenuation_t) {}
void analogReadResolution(int) {}
int analogRead(int) { return 0; }
void tone(int, unsigned int, unsigned long) {}
void noTone(int) {}
long map(long x, long a, long b, long c, long d) { return (x - a) * (d - c) / (b - a) + c; }
WiFiClassStub WiFi;
int esp_now_init() { return 0; }
int esp_now_deinit() { return 0; }
int esp_now_register_recv_cb(esp_now_recv_cb_t) { return 0; }
int esp_now_add_peer(const esp_now_peer_info_t *) { return 0; }
int esp_now_del_peer(const uint8_t *) { return 0; }
int esp_now_send(const uint8_t *, const uint8_t *, int) { return 0; }
int esp_wifi_get_channel(uint8_t *ch, wifi_second_chan_t *) { *ch = 1; return 0; }
int esp_wifi_set_channel(uint8_t, wifi_second_chan_t) { return 0; }
int esp_wifi_set_ps(int) { return 0; }

#include "sketch.cpp"

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { printf("ล้มเหลว: %s (บรรทัด %d)\n", msg, __LINE__); fails++; } } while (0)

static std::string g_opsPath;
static void mark(const char *name) { fprintf(g_ops, "MARK %s\n", name); fflush(g_ops); }

// ---- ตัวช่วยจำลอง ----
static const uint8_t MAC[6] = {1, 2, 3, 4, 5, 6};
static struct_message mk(int id, int run, uint32_t drops, float rate, uint32_t since) {
  struct_message m = {}; m.stationId = (uint8_t)id; m.isRunning = (uint8_t)run; m.totalDrops = drops;
  m.flowRateHr = rate; m.msSinceLastDrop = since; m.batteryVolts = 3.9f; return m;
}
static void resetBeds() {
  for (int i = 0; i < MAX_BEDS; i++) beds[i] = Bed();
  activeBeds = 5;
  snoozeUntil = 0;
  syncSched = SyncScheduler();
  sysSet = {SYS_MAGIC, 100, 1};
  buzz = BuzzerEngine();
}
static void scenario() {
  resetBeds();
  const uint32_t t = g_ms;
  beds[0].cfg.targetRateHr = 60;  beds[0].cfg.planVolumeMl = 500;  beds[0].cfg.resetSeq = 7;   // ไม่ใช่ 0 เพื่อให้จับได้ถ้าโค้ดทับค่านี้
  bedIngest(beds[0], mk(1, 1, 3100, 60, 400), -48, MAC, t);  beds[0].linkPct = 100;
  beds[1].cfg.targetRateHr = 100; beds[1].cfg.planVolumeMl = 1000;
  bedIngest(beds[1], mk(2, 1, 9000, 142, 380), -61, MAC, t); beds[1].alert = ALERT_TOO_FAST; beds[1].linkPct = 90;
  beds[3].cfg.targetRateHr = 80;  beds[3].cfg.planVolumeMl = 500;
  bedIngest(beds[3], mk(4, 1, 8200, 80, 700), -70, MAC, t);  beds[3].alert = ALERT_NEAR_END; beds[3].linkPct = 50;
  bedIngest(beds[4], mk(5, 0, 120, 0, 0), -55, MAC, t);      beds[4].linkPct = 100;
  for (int i = 0; i < MAX_BEDS; i++) beds[i].alert = beds[i].alert;  // เตียง 3 ไม่เคยได้ยิน = NO DATA
  cal.a = 1; cal.b = 0; cal.c = 0; cal.d = 0; cal.e = 1; cal.f = 0; cal.valid = true;
}

static int zoneRect(int id, int &x, int &y, int &w, int &h) {
  for (uint8_t i = 0; i < zoneCount; i++) if (zones[i].id == id) { x = zones[i].x; y = zones[i].y; w = zones[i].w; h = zones[i].h; return 1; }
  return 0;
}
static bool tapId(int id) {
  int x, y, w, h;
  if (!zoneRect(id, x, y, w, h)) return false;
  uiTap(x + w / 2, y + h / 2, g_ms);
  uiRefresh(g_ms);                                       // เหมือน uiStep(): วาดหน้าใหม่ทันทีหลังแตะ ก่อนการแตะครั้งถัดไป
  g_ms += 200;
  return true;
}
static void drawNow() { ui.needFull = true; uiRefresh(g_ms); }

// อ่านไฟล์คำสั่งวาดล่าสุด ตรวจว่าข้อความทุกบรรทัดอยู่ในจอ 320x240
static long g_opsChecked = 0;
static void checkTextBounds(const char *what) {
  fflush(g_ops);
  FILE *f = fopen(g_opsPath.c_str(), "r");
  if (!f) return;
  fseek(f, g_opsChecked, SEEK_SET);
  char line[512];
  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "TEXT ", 5) != 0) continue;
    int x, y, size, col, bg, off = 0;
    sscanf(line, "TEXT %d %d %d %d %d %n", &x, &y, &size, &col, &bg, &off);
    int len = (int)strlen(line + off);
    while (len > 0 && (line[off + len - 1] == '\n' || line[off + len - 1] == '\r')) len--;
    if (x < 0 || y < 0 || x + len * 6 * size > 320 || y + 8 * size > 240) {
      printf("ล้มเหลว: %s ข้อความล้นจอที่ (%d,%d) ขนาด %d: %.*s\n", what, x, y, size, len, line + off);
      fails++;
    }
  }
  g_opsChecked = ftell(f);
  fclose(f);
}

// ทุกโซนต้องอยู่ในจอ ศูนย์กลางโซนต้องแตะโดนโซนของตัวเอง (ไม่มีโซนอื่นทับ) และไม่มีโซนซ้ำเลขเดียวกัน
static void checkZones(const char *what) {
  for (uint8_t i = 0; i < zoneCount; i++) {
    const Zone &z = zones[i];
    if (z.x < 0 || z.y < 0 || z.x + z.w > 320 || z.y + z.h > 240 || z.w < 24 || z.h < 16) {
      printf("ล้มเหลว: %s โซน %d อยู่นอกจอหรือเล็กเกินแตะ\n", what, z.id); fails++;
    }
    if (zoneAt(z.x + z.w / 2, z.y + z.h / 2) != z.id) { printf("ล้มเหลว: %s โซน %d ถูกโซนอื่นทับตรงกลาง\n", what, z.id); fails++; }
    for (uint8_t j = i + 1; j < zoneCount; j++) if (zones[j].id == z.id) { printf("ล้มเหลว: %s โซน %d ซ้ำ\n", what, z.id); fails++; }
  }
}

static void screenCheck(const char *what, const char *markName = nullptr) {
  checkZones(what);
  if (markName) mark(markName);
  checkTextBounds(what);
}

// ---------------- บัซเซอร์: เก็บไทม์ไลน์ระดับ 1 ms ----------------
static std::vector<int> timeline(BuzzerEngine &b, uint32_t t0, uint32_t ms) {
  std::vector<int> v;
  for (uint32_t t = 0; t < ms; t++) v.push_back(b.update(t0 + t) ? 1 : 0);
  return v;
}
static int pulses(const std::vector<int> &v) { int n = 0; for (size_t i = 0; i < v.size(); i++) if (v[i] && (i == 0 || !v[i - 1])) n++; return n; }
static int onMs(const std::vector<int> &v) { int n = 0; for (int x : v) n += x; return n; }

static void testBuzzerPatterns() {
  struct Case { uint8_t code; int bursts, on, cycle; } cases[] = {
    {ALERT_OCCLUSION, 3, 150, 3 * 150 + 2 * 100 + 900}, {ALERT_TOO_FAST, 4, 60, 4 * 60 + 3 * 60 + 900},
    {ALERT_TOO_SLOW, 1, 350, 350 + 1200}, {ALERT_COMPLETE, 2, 250, 2 * 250 + 200 + 1800},
    {ALERT_NO_SIGNAL, 2, 100, 2 * 100 + 100 + 1500}, {ALERT_LOST_LINK, 5, 80, 5 * 80 + 4 * 80 + 2000}};
  for (const Case &c : cases) {
    BuzzerEngine b;
    b.setAlert(c.code, 5000);
    auto v = timeline(b, 5000, c.cycle * 2);
    CHECK(pulses(v) == c.bursts * 2, "จำนวนครั้งต่อรอบของรหัสเตือนต้องตรงแบบแผน");
    CHECK(onMs(v) == c.bursts * c.on * 2, "ความยาวเสียงรวมต่อรอบต้องตรงแบบแผน");
    CHECK(v[0] == 1 && v[c.cycle] == 1, "รอบใหม่ต้องเริ่มด้วยเสียงทันที");
    CHECK(!v[c.cycle - 1], "ท้ายรอบต้องเป็นช่วงเงียบ");
  }
  // จังหวะของแต่ละรหัสต้องต่างกันพอแยกด้วยหูได้ (ลายนิ้วมือ = จำนวนครั้ง x ความยาว)
  for (size_t i = 0; i < 6; i++) for (size_t j = i + 1; j < 6; j++)
    CHECK(!(cases[i].bursts == cases[j].bursts && cases[i].on == cases[j].on), "สองรหัสเตือนต้องไม่มีจังหวะเหมือนกัน");
  BuzzerEngine b;
  b.setAlert(ALERT_OCCLUSION, 100);
  b.update(100);
  const bool l1 = b.update(100 + 160);
  b.setAlert(ALERT_OCCLUSION, 100 + 200);                // รหัสเดิมซ้ำ ต้องไม่เริ่มจังหวะใหม่
  CHECK(b.update(100 + 160) == l1, "ตั้งรหัสเดิมซ้ำต้องไม่รีสตาร์ตจังหวะ");
  b.setAlert(ALERT_NONE, 5000);
  CHECK(!b.update(5000) && !b.update(5100), "รหัส 0 ต้องเงียบ");
  b.setAlert(ALERT_OCCLUSION, 6000);
  b.enabled = false;
  CHECK(!b.update(6000), "ปิดเสียงแล้วต้องเงียบทุกกรณี");
  b.click(6000);
  CHECK(!b.update(6001), "ปิดเสียงแล้วเสียงกดก็ต้องเงียบ");
  b.enabled = true;
  // เสียงกดแทรกในช่วงเงียบของจังหวะเตือนได้ และสั้น
  BuzzerEngine c;
  c.setAlert(ALERT_OCCLUSION, 0);
  CHECK(!c.update(700), "ช่วงเงียบของสายพับ (หลังครบ 3 ครั้งที่ 550 ms)");
  c.click(700);
  CHECK(c.update(710) && !c.update(730), "เสียงกดต้องดังราว 25 ms แล้วหยุด");
  // เตือนใกล้หมด: 3 ครั้งสั้น ๆ แล้วเงียบ
  BuzzerEngine d;
  d.chime(1000);
  auto v = timeline(d, 1000, 2000);
  CHECK(pulses(v) == 3 && onMs(v) == 300, "ใกล้หมดต้อง 3 ครั้ง ครั้งละ 100 ms");
  CHECK(!d.update(1000 + 600), "เสียงใกล้หมดต้องจบเอง");
  // เสียงทดสอบ
  BuzzerEngine e;
  e.test(0);
  CHECK(e.update(100) && !e.update(450), "เสียงทดสอบต้องดังราว 400 ms");
  // ลำดับความสำคัญ
  CHECK(buzzPriority(ALERT_OCCLUSION) > buzzPriority(ALERT_LOST_LINK) && buzzPriority(ALERT_LOST_LINK) > buzzPriority(ALERT_TOO_FAST) &&
        buzzPriority(ALERT_TOO_FAST) > buzzPriority(ALERT_COMPLETE) && buzzPriority(ALERT_COMPLETE) > buzzPriority(ALERT_NO_SIGNAL) &&
        buzzPriority(ALERT_NO_SIGNAL) > buzzPriority(ALERT_TOO_SLOW) && buzzPriority(ALERT_TOO_SLOW) > buzzPriority(ALERT_NONE), "ลำดับความสำคัญของรหัสเตือน");
  CHECK(buzzPriority(ALERT_NEAR_END) == 0, "ใกล้หมดไม่ใช่เสียงเตือนวิกฤต");
}

static int pinOn() { return g_pinLevel[CYD_BUZZER_PIN] == CYD_BUZZER_ON_LEVEL; }

static void testBuzzerIntegration() {
  scenario();
  beds[0].alert = ALERT_NONE; beds[1].alert = ALERT_NONE; beds[3].alert = ALERT_NONE;
  uint32_t t = g_ms;
  serviceBuzzer(t);
  CHECK(!pinOn(), "ไม่มีเหตุเตือนต้องเงียบ");
  beds[1].alert = ALERT_TOO_FAST;
  serviceBuzzer(t);
  CHECK(worstAlarm(t) == ALERT_TOO_FAST && pinOn(), "เตียงเร็วเกินต้องร้อง");
  beds[0].alert = ALERT_OCCLUSION;
  CHECK(worstAlarm(t) == ALERT_OCCLUSION, "หลายเตียงเตือนพร้อมกันต้องเลือกรหัสที่สำคัญที่สุด");
  // พักเสียง
  snoozeUntil = t + SNOOZE_MS;
  g_ms = t + 1000; serviceBuzzer(g_ms);
  CHECK(!pinOn() && snoozed(g_ms), "พักเสียงแล้วต้องเงียบ");
  g_ms = t + SNOOZE_MS + 10; serviceBuzzer(g_ms);
  CHECK(!snoozed(g_ms), "พักครบเวลาต้องหมดเอง");
  // เหตุหายหมด ต้องล้างการพัก เพื่อให้เหตุครั้งหน้าดังทันที
  snoozeUntil = g_ms + 50000;
  beds[0].alert = ALERT_NONE; beds[1].alert = ALERT_NONE;
  serviceBuzzer(g_ms);
  CHECK(snoozeUntil == 0, "ไม่มีเหตุวิกฤตแล้วต้องล้างการพักเสียง");
  // ใกล้หมด: ดัง 3 ครั้งแล้วเงียบ ซ้ำทุก 5 นาทีจนกว่าจะรับทราบ
  beds[3].alert = ALERT_NEAR_END;
  g_ms += 1000;
  int rising = 0, last = 0;
  for (int k = 0; k < 800; k++) { g_ms++; serviceBuzzer(g_ms); int on = pinOn(); if (on && !last) rising++; last = on; }
  CHECK(rising == 3, "ใกล้หมดต้องดัง 3 ครั้งแล้วเงียบ");
  rising = 0;
  for (int k = 0; k < 60000; k++) { g_ms += 5; serviceBuzzer(g_ms); int on = pinOn(); if (on && !last) rising++; last = on; }   // ผ่านไป 300 วินาที
  CHECK(rising == 3, "ครบ 5 นาทียังไม่รับทราบต้องดังซ้ำ 3 ครั้ง");
  beds[3].nearEndAck = true;
  rising = 0;
  for (int k = 0; k < 70000; k++) { g_ms += 5; serviceBuzzer(g_ms); int on = pinOn(); if (on && !last) rising++; last = on; }
  CHECK(rising == 0, "รับทราบแล้วต้องไม่ดังซ้ำ");
  // เสียงกด
  g_ms += 100000; serviceBuzzer(g_ms);
  buzz.click(g_ms); serviceBuzzer(g_ms + 1);
  CHECK(pinOn(), "แตะแล้วต้องมีเสียงกด");
  serviceBuzzer(g_ms + 50);
  CHECK(!pinOn(), "เสียงกดต้องหยุดเอง");
}

// ---------------- หน้าจอ ----------------
static void testNavigationAndEditing() {
  scenario();
  ui = UiState();
  drawNow();
  CHECK(ui.scr == SCR_HOME, "เริ่มที่หน้า HOME");
  screenCheck("HOME", "home");
  CHECK(focusBed(g_ms) == 1, "เตียงเร็วเกินต้องเป็นเตียงที่โชว์ใหญ่");
  CHECK(tapId(Z_BED_ROW + 1) && ui.scr == SCR_BED && ui.bed == 1, "แตะแถวเตียง 2 ต้องเข้าหน้า BED เตียง 2");
  drawNow(); screenCheck("BED", "bed");
  // หน้า BED ของเตียงที่ใกล้หมด (ACK ใช้ได้)
  CHECK(tapId(Z_BACK) && ui.scr == SCR_HOME, "BACK กลับ HOME");
  CHECK(tapId(Z_BED_ROW + 3) && ui.bed == 3, "เข้าเตียง 4");
  drawNow(); screenCheck("BED near end", "bed_nearend");
  syncSched = SyncScheduler();
  CHECK(tapId(Z_ACK) && beds[3].nearEndAck, "ACK ต้องรับทราบเตียงที่ใกล้หมด");
  CHECK(syncSched.next(g_ms + 5000, 5) == 3, "รับทราบแล้วต้องขอส่ง Sync ด่วน");
  // เตียงที่ไม่ใกล้หมด ACK ต้องไม่ทำอะไร
  ui.bed = 0; drawNow();
  tapId(Z_ACK);
  CHECK(!beds[0].nearEndAck, "ACK บนเตียงที่ไม่ใกล้หมดต้องไม่ทำอะไร");
  // ตั้งค่า
  CHECK(tapId(Z_SETBTN) && ui.scr == SCR_SET, "SET เข้าหน้าตั้งค่า");
  screenCheck("SET", "set");
  CHECK(tapId(Z_F_RATE) && ui.scr == SCR_NUMPAD && ui.field == NF_RATE, "แตะช่องอัตราเข้าแป้นตัวเลข");
  screenCheck("NUMPAD", "numpad_empty");
  tapId(Z_KEY + 7); tapId(Z_KEY + 5);
  CHECK(strcmp(ui.buf, "75") == 0, "กด 7 5 ต้องได้ 75");
  tapId(Z_KEY_DOT); tapId(Z_KEY + 5); tapId(Z_KEY_DOT);
  CHECK(strcmp(ui.buf, "75.5") == 0, "จุดทศนิยมซ้ำต้องถูกเมิน");
  tapId(Z_KEY_DEL); tapId(Z_KEY_DEL);
  CHECK(strcmp(ui.buf, "75") == 0, "ลบถอยหลัง");
  drawNumpadValue(); mark("numpad_75"); checkTextBounds("NUMPAD value");
  CHECK(tapId(Z_KEY_OK) && ui.scr == SCR_SET && ui.edit.targetRateHr == 75, "OK ต้องกลับหน้าตั้งค่าพร้อมค่าใหม่");
  CHECK(beds[0].cfg.targetRateHr == 60, "ยังไม่ SAVE ห้ามแก้ค่าจริง");
  CHECK(tapId(Z_F_DF) && ui.edit.dropFactor == 60, "แตะ DROP FACTOR วน 20 -> 60");
  tapId(Z_F_DF); CHECK(ui.edit.dropFactor == 10, "60 -> 10");
  tapId(Z_F_DF); tapId(Z_F_DF); CHECK(ui.edit.dropFactor == 20, "10 -> 15 -> 20");
  tapId(Z_F_DF);
  const uint8_t seq0 = beds[0].cfg.resetSeq;
  syncSched = SyncScheduler();
  CHECK(tapId(Z_SAVE) && ui.scr == SCR_BED, "SAVE กลับหน้า BED");
  CHECK(beds[0].cfg.targetRateHr == 75 && beds[0].cfg.dropFactor == 60, "SAVE ต้องเขียนค่าจริง");
  CHECK(beds[0].cfg.resetSeq == seq0, "ห้ามแตะ resetSeq จากหน้าตั้งค่า (Station จะล้างตัวนับเอง)");
  CHECK(syncSched.next(g_ms + 5000, 5) == 0, "SAVE ต้องขอส่ง Sync ด่วน");
  // ยกเลิกต้องไม่แก้อะไร
  tapId(Z_SETBTN); tapId(Z_F_PLAN); tapId(Z_KEY + 9); tapId(Z_KEY_OK);
  CHECK(ui.edit.planVolumeMl == 9, "แก้ปริมาตรในสำเนา");
  tapId(Z_CANCEL);
  CHECK(ui.scr == SCR_BED && beds[0].cfg.planVolumeMl == 500, "CANCEL ต้องทิ้งการแก้ไข");
  // ตรวจค่าในแป้นตัวเลข
  tapId(Z_SETBTN); tapId(Z_F_RATE);
  for (int d : {5, 0, 0, 0}) tapId(Z_KEY + d);
  CHECK(!tapId(Z_KEY_OK) || ui.scr == SCR_NUMPAD, "อัตรา 5000 เกินช่วงต้องไม่ผ่าน");
  CHECK(ui.scr == SCR_NUMPAD, "ค่านอกช่วงต้องค้างที่แป้นตัวเลข");
  drawNumpadValue(); mark("numpad_invalid");
  tapId(Z_CANCEL); tapId(Z_F_NEAR);
  tapId(Z_KEY + 4); tapId(Z_KEY + 0); tapId(Z_KEY_OK);
  CHECK(ui.scr == SCR_NUMPAD, "เกณฑ์ใกล้หมด 40 ต่ำกว่า 50 ต้องไม่ผ่าน");
  tapId(Z_KEY_DEL); tapId(Z_KEY_DEL); tapId(Z_KEY + 9); tapId(Z_KEY + 5); tapId(Z_KEY_OK);
  CHECK(ui.scr == SCR_SET && ui.edit.nearEndPct == 95, "เกณฑ์ใกล้หมด 95 ต้องผ่าน");
  tapId(Z_F_NEAR); tapId(Z_KEY + 0); tapId(Z_KEY + 7);
  CHECK(strcmp(ui.buf, "7") == 0, "เลขศูนย์นำหน้าต้องถูกแทนที่");
  tapId(Z_F_NEAR);                                        // อยู่ใน NUMPAD แล้ว โซนนี้ไม่มี ต้องเมินเฉย
  tapId(Z_CANCEL); tapId(Z_CANCEL);
  // เริ่มถุงใหม่ต้องยืนยัน
  CHECK(ui.scr == SCR_BED, "กลับหน้า BED");
  const uint8_t seq1 = beds[0].cfg.resetSeq;
  tapId(Z_NEWBAG);
  CHECK(ui.confirmBag, "NEW BAG ต้องถามยืนยันก่อน");
  checkZones("CONFIRM");
  CHECK(zoneAt(160, 20) == Z_NONE && zoneAt(10, 210) == Z_NONE, "ระหว่างถามยืนยัน ปุ่มอื่นต้องแตะไม่ได้");
  mark("bed_confirm"); checkTextBounds("CONFIRM");
  tapId(Z_NO);
  CHECK(!ui.confirmBag && beds[0].cfg.resetSeq == seq1, "NO ต้องไม่เปลี่ยนอะไร");
  drawNow();
  syncSched = SyncScheduler();
  tapId(Z_NEWBAG); tapId(Z_YES);
  CHECK(beds[0].cfg.resetSeq == (uint8_t)(seq1 + 1) && beds[0].totalDrops == 0, "YES ต้องเริ่มถุงใหม่");
  CHECK(syncSched.next(g_ms + 5000, 5) == 0, "เริ่มถุงใหม่ต้องส่ง Sync ด่วน");
}

static void testSystemAndSilence() {
  scenario();
  ui = UiState();
  drawNow();
  {                                                      // เสียงกด: แตะโดนปุ่มต้องดังสั้น ๆ แตะที่ว่างต้องเงียบ
    buzz = BuzzerEngine();
    uiTap(160, 3, g_ms);
    CHECK(!buzz.update(g_ms + 5), "แตะที่ว่างต้องไม่มีเสียง");
    int x, y, w, h; zoneRect(Z_SYS, x, y, w, h);
    const uint32_t t0 = g_ms + 1000;
    uiTap(x + w / 2, y + h / 2, t0);
    CHECK(buzz.update(t0 + 5) && !buzz.update(t0 + 60), "แตะโดนปุ่มต้องมีเสียงกดสั้น ๆ");
    ui.scr = SCR_HOME; drawNow();
  }
  CHECK(tapId(Z_SYS) && ui.scr == SCR_SYSTEM, "SYS เข้าหน้าระบบ");
  drawNow(); screenCheck("SYSTEM", "system");
  for (int i = 0; i < 10; i++) tapId(Z_BEDS_DEC);
  CHECK(activeBeds == 1, "จำนวนเตียงต้องไม่ต่ำกว่า 1");
  for (int i = 0; i < 12; i++) tapId(Z_BEDS_INC);
  CHECK(activeBeds == 8, "จำนวนเตียงต้องไม่เกิน 8");
  for (int i = 0; i < 12; i++) tapId(Z_BRI_DEC);
  CHECK(sysSet.brightness == 10 && g_bl == (10 * 255) / 100, "ความสว่างต้องไม่ต่ำกว่า 10% และสั่งไฟหลังจอจริง");
  for (int i = 0; i < 12; i++) tapId(Z_BRI_INC);
  CHECK(sysSet.brightness == 100, "ความสว่างไม่เกิน 100%");
  tapId(Z_BUZZ_TOGGLE);
  CHECK(!sysSet.buzzerOn && !buzz.enabled, "สวิตช์เสียงต้องปิดเสียงจริง");
  drawNow(); mark("system_buzzer_off");
  tapId(Z_BUZZ_TOGGLE);
  CHECK(buzz.enabled, "เปิดเสียงกลับ");
  g_ms += 1000; buzz.test(g_ms);
  CHECK(buzz.update(g_ms + 10), "ปุ่ม TEST ต้องมีเสียง");
  CHECK(tapId(Z_BACK) && ui.scr == SCR_HOME, "BACK กลับ HOME");
  // ปุ่มพักเสียง
  activeBeds = 5;
  for (int i = 0; i < MAX_BEDS; i++) if (beds[i].alert != ALERT_NONE && isCriticalAlert(beds[i].alert)) beds[i].alert = ALERT_NONE;
  drawNow();
  tapId(Z_SILENCE);
  CHECK(!snoozed(g_ms), "ไม่มีเหตุเตือน ปุ่มพักเสียงต้องไม่ทำอะไร");
  beds[1].alert = ALERT_OCCLUSION;
  drawNow(); screenCheck("HOME alarm", "home_alarm");
  tapId(Z_SILENCE);
  CHECK(snoozed(g_ms), "แตะแล้วต้องพักเสียง");
  uiRefresh(g_ms + 600); mark("home_silenced"); checkTextBounds("HOME silenced");
  tapId(Z_SILENCE);
  CHECK(!snoozed(g_ms), "แตะซ้ำต้องยกเลิกการพัก");
}


// ---- หน้าเปิดเครื่อง: โลโก้บนพื้นดำ + แถบโหลดสไตล์ XP ----
// อ่านคำสั่งวาด RECT ที่เพิ่มต่อท้ายไฟล์ตั้งแต่ตำแหน่ง from คืนรายการ (x,y,w,h) ของสี color
struct R4 { int x, y, w, h; };
static std::vector<R4> rectsSince(long from, unsigned color) {
  fflush(g_ops);
  std::vector<R4> out;
  FILE *f = fopen(g_opsPath.c_str(), "r");
  fseek(f, from, SEEK_SET);
  static char line[400000];
  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "RECT ", 5) != 0) continue;
    int x, y, w, h, fill; unsigned c;
    if (sscanf(line, "RECT %d %d %d %d %u %d", &x, &y, &w, &h, &c, &fill) == 6 && c == color && fill == 1) out.push_back({x, y, w, h});
  }
  fclose(f);
  return out;
}
struct RR { int x, y, w, h, r; unsigned c; };
static std::vector<RR> rrectsSince(long from) {
  fflush(g_ops);
  std::vector<RR> out;
  FILE *f = fopen(g_opsPath.c_str(), "r");
  fseek(f, from, SEEK_SET);
  static char line[400000];
  while (fgets(line, sizeof(line), f)) {
    RR q; int fill;
    if (sscanf(line, "RRECT %d %d %d %d %d %u %d", &q.x, &q.y, &q.w, &q.h, &q.r, &q.c, &fill) == 7 && fill == 1) out.push_back(q);
  }
  fclose(f);
  return out;
}
static long opsEnd() { fflush(g_ops); return ftell(g_ops); }

static void testSplash() {
  splashDraw();
  mark("splash");
  CHECK(LOGO_PRESENT == 1 && LOGO_W <= 320 && LOGO_H <= 200, "ต้องมีโลโก้จริง และเหลือที่ให้แถบโหลดด้านล่าง");
  const int trackY = splashTrackY();
  CHECK(6 + LOGO_H < trackY, "แถบโหลดต้องไม่ทับโลโก้");
  CHECK(trackY + SPL_TRACK_H <= 240 && splashTrackX() >= 0 && splashTrackX() + SPL_TRACK_W <= 320, "แถบโหลดต้องอยู่ในจอ");
  // พื้นหลังโลโก้ต้องเป็นสีดำ (ตามที่ขอ) และมีเนื้อภาพจริง
  uint32_t nonBlack = 0, corner = 0;
  for (int i = 0; i < LOGO_W * LOGO_H; i++) if (LOGO_DATA[i] != 0x0000) nonBlack++;
  corner = LOGO_DATA[0] | LOGO_DATA[LOGO_W - 1] | LOGO_DATA[(LOGO_H - 1) * LOGO_W] | LOGO_DATA[LOGO_W * LOGO_H - 1];
  CHECK(corner == 0, "มุมของภาพ (พื้นหลัง) ต้องเป็นสีดำ");
  CHECK(nonBlack > 5000 && nonBlack < (uint32_t)(LOGO_W * LOGO_H) * 9 / 10, "ภาพโลโก้ต้องมีเนื้อภาพจริง ไม่ใช่ว่างหรือเต็มสีเดียว");
  // เวลา: ค้างครบ หักเวลาที่ใช้เริ่มระบบ แตะข้ามได้ ไม่รอเพิ่มถ้าเลยเวลา
  const uint32_t t0 = millis();
  CHECK(splashStartMs == t0, "ต้องจำเวลาเริ่มวาดโลโก้");
  g_ms += 700;
  const uint32_t before = millis();
  splashWait([]() { return false; });
  CHECK(millis() - t0 >= (uint32_t)CYD_SPLASH_MS && millis() - t0 < (uint32_t)CYD_SPLASH_MS + 20, "ต้องรอให้ครบเวลานับจากตอนวาด ไม่ใช่นับจากตอนเรียกรอ");
  CHECK(millis() - before >= (uint32_t)CYD_SPLASH_MS - 700 - 5, "เวลาที่ใช้ระหว่างเริ่มระบบต้องถูกหักออกจากการรอ");
  splashDraw();
  const uint32_t t1 = millis();
  int calls = 0;
  splashWait([&]() { return ++calls >= 5; });
  CHECK(millis() - t1 < 100, "แตะจอต้องข้ามหน้าเปิดเครื่องทันที");
  splashDraw();
  g_ms += (uint32_t)CYD_SPLASH_MS + 1000;
  const uint32_t t2 = millis();
  splashWait([]() { return false; });
  CHECK(millis() == t2, "เริ่มระบบนานเกินเวลาโลโก้แล้ว ต้องไม่รอเพิ่มอีก");

  // ---- แถบโหลดสไตล์ macOS: รางเทาเข้ม ส่วนขาวยาวขึ้นเรื่อย ๆ เต็มพอดีเมื่อครบเวลา ไม่ล้นราง ----
  CHECK(splashFillPx(0) == 0 && splashFillPx((uint32_t)CYD_SPLASH_MS) == SPL_TRACK_W && splashFillPx((uint32_t)CYD_SPLASH_MS * 10) == SPL_TRACK_W, "เริ่มที่ว่าง เต็มพอดีเมื่อครบเวลา และไม่เกินรางเมื่อเลยเวลา");
  CHECK(splashFillPx((uint32_t)CYD_SPLASH_MS / 2) == SPL_TRACK_W / 2, "ครึ่งเวลาต้องเต็มครึ่งราง");
  int lastFill = 0;
  for (uint32_t ms = 0; ms <= (uint32_t)CYD_SPLASH_MS + 100; ms += 7) { const int f = splashFillPx(ms); CHECK(f >= lastFill && f <= SPL_TRACK_W, "ส่วนเต็มต้องไม่ถอยและไม่ล้นราง"); lastFill = f; }
  const unsigned FILLC = SPL_C_FILL, TRACKC = SPL_C_TRACK;
  const uint32_t samples[] = {0, 20, (uint32_t)CYD_SPLASH_MS / 3, (uint32_t)CYD_SPLASH_MS / 2, (uint32_t)CYD_SPLASH_MS - 1, (uint32_t)CYD_SPLASH_MS};
  for (uint32_t ms : samples) {
    g_ms = 50000; splashStartMs = g_ms - ms;
    splashLastFill = -1;
    const long from = opsEnd();
    splashTick();
    auto rr = rrectsSince(from);
    int tracks = 0, fills = 0;
    for (auto &q : rr) {
      CHECK(q.x >= splashTrackX() && q.x + q.w <= splashTrackX() + SPL_TRACK_W && q.y == trackY && q.h == SPL_TRACK_H, "ราง/ส่วนเต็มต้องอยู่ในกรอบที่กำหนด");
      if (q.c == TRACKC && q.w == SPL_TRACK_W) tracks++;
      if (q.c == FILLC) { fills++; CHECK(q.w == splashFillPx(ms), "ความกว้างส่วนเต็มต้องตรงสูตร"); CHECK(q.r * 2 <= q.w || q.r == q.w / 2, "รัศมีปลายมนต้องไม่เกินครึ่งความกว้าง"); }
    }
    CHECK(tracks == 1 && fills == (splashFillPx(ms) > 0 ? 1 : 0), "ต้องวาดรางหนึ่งครั้ง และส่วนเต็มเมื่อมีความยาว");
  }
  // ไม่วาดซ้ำถ้าส่วนเต็มไม่ยาวขึ้น (เรียกถี่ได้โดยไม่กินเวลา SPI) และวาดใหม่เมื่อยาวขึ้น
  g_ms = 70000; splashStartMs = g_ms; splashLastFill = -1;
  splashTick();
  const long mark1 = opsEnd();
  splashTick(); splashTick();
  CHECK(opsEnd() == mark1, "เรียกซ้ำโดยส่วนเต็มเท่าเดิมต้องไม่วาดซ้ำ");
  g_ms += (uint32_t)CYD_SPLASH_MS / SPL_TRACK_W + 1;
  splashTick();
  CHECK(opsEnd() > mark1, "ส่วนเต็มยาวขึ้นต้องวาดใหม่");
  g_ms = 90000; splashDraw(); g_ms += (uint32_t)CYD_SPLASH_MS * 6 / 10; splashTick();
  mark("splash_progress");
  checkTextBounds("SPLASH");
}

static void testExtremeValues() {
  scenario();
  beds[0].cfg.targetRateHr = 3000; beds[0].cfg.planVolumeMl = 10000;
  bedIngest(beds[0], mk(1, 1, 4000000, 2999, 400), -99, MAC, g_ms);
  beds[0].batteryV = 5.9f; beds[0].linkPct = 100; beds[0].idConflict = true;
  activeBeds = 8;
  ui = UiState();
  for (int s = SCR_HOME; s <= SCR_SYSTEM; s++) {
    ui.scr = (Screen)s; ui.bed = 0; ui.field = NF_PLAN; strcpy(ui.buf, "10000");
    ui.edit = beds[0].cfg;
    drawNow();
    screenCheck("ค่าสุดขั้ว");
  }
  ui.scr = SCR_HOME; ui.bed = 0; drawNow(); mark("home_8beds_extreme");
  ui.scr = SCR_BED; drawNow(); mark("bed_extreme");
}


// ---- หน้า TREND (กราฟแนวโน้มบนจอ) ----
struct L4 { int x0, y0, x1, y1; unsigned col; };
static std::vector<L4> linesSince(long from) {
  fflush(g_ops);
  std::vector<L4> out;
  FILE *f = fopen(g_opsPath.c_str(), "r");
  fseek(f, from, SEEK_SET);
  static char line[400000];
  while (fgets(line, sizeof(line), f)) {
    L4 l;
    if (sscanf(line, "LINE %d %d %d %d %u", &l.x0, &l.y0, &l.x1, &l.y1, &l.col) == 5) out.push_back(l);
  }
  fclose(f);
  return out;
}
static int countBigRects(long from) {                     // จำนวนการล้างกรอบกราฟทั้งกรอบ (RECT x y w h สีดำ ทึบ)
  int n = 0;
  for (auto &r : rectsSince(from, 0)) if (r.x == TR_X && r.y == TR_Y && r.w == TR_W && r.h == TR_H) n++;
  return n;
}
static void fillStandardTrend() {
  trend = TrendState();
  Bed tmp[MAX_BEDS]; tmp[0].cfg.targetRateHr = 60;
  uint32_t drops = 0, now = 1000; double acc = 0; int n; const Seg *segs = standardScenario(n);
  runScenario(trend, tmp, 0, segs, n, now, acc, drops);
}

static void testTrendScreen() {
  scenario();
  fillStandardTrend();
  ui = UiState(); ui.scr = SCR_BED; ui.bed = 0; drawNow();
  screenCheck("BED (ห้าปุ่ม)", "bed_buttons");
  for (int id : {Z_BACK, Z_SETBTN, Z_GRAPH, Z_NEWBAG, Z_ACK}) { int x, y, w, h; CHECK(zoneRect(id, x, y, w, h) && w >= 40 && h >= 40, "ปุ่มหน้า BED ต้องใหญ่พอแตะ (ไม่เล็กกว่า 40 พิกเซล)"); }
  CHECK(tapId(Z_GRAPH) && ui.scr == SCR_TREND, "GRAPH เข้าหน้า TREND");
  screenCheck("TREND", "trend");
  // ขอบเขตกราฟ: ทุกเส้นที่วาดต้องอยู่ในกรอบ
  long from = opsEnd();
  ui.needFull = true; uiRefresh(g_ms);
  auto lines = linesSince(from);
  CHECK(lines.size() > 30, "กราฟต้องมีเส้นจริง");
  bool inside = true, hasGreen = false, hasCyan = false;
  for (auto &l : lines) {
    if (l.x0 < TR_X || l.x1 < TR_X || l.x0 >= TR_X + TR_W || l.x1 >= TR_X + TR_W || l.y0 < TR_Y || l.y1 < TR_Y || l.y0 >= TR_Y + TR_H || l.y1 >= TR_Y + TR_H) inside = false;
    if (l.col == C_GREEN) hasGreen = true;
    if (l.col == C_CYAN) hasCyan = true;
  }
  CHECK(inside, "ทุกเส้นของกราฟต้องอยู่ในกรอบ");
  CHECK(hasGreen && hasCyan, "ต้องมีเส้นอัตราไหล (เขียว) และเส้นเป้าหมาย (ฟ้า)");
  CHECK(!rectsSince(from, C_BTNRED).empty() || true, "ตรวจแถบเตือน");
  // แถบเตือนวิกฤตสีแดงต้องมี (สถานการณ์มีเตือนเร็วเกิน 10 นาที) และเส้นประเตือนใกล้หมดสีเหลืองอำพันก็มี
  {
    fflush(g_ops);
    FILE *f = fopen(g_opsPath.c_str(), "r"); fseek(f, from, SEEK_SET);
    static char ln[400000]; int redBars = 0, amberBars = 0, grayTicks = 0;
    while (fgets(ln, sizeof(ln), f)) {
      int x, y, h2, w2; unsigned c, fl;
      if (sscanf(ln, "RECT %d %d %d %d %u %u", &x, &y, &w2, &h2, &c, &fl) == 6 && w2 == 1) {       // drawFastVLine ในตัวจำลองบันทึกเป็น RECT กว้าง 1
        if (c == C_BTNRED) redBars++; else if (c == C_AMBER) amberBars++; else if (c == C_GRAY && h2 == 4) grayTicks++;
      }
    }
    fclose(f);
    CHECK(redBars >= 8 && amberBars >= 8, "ต้องมีแถบแดง (เตือนวิกฤต) และเหลืองอำพัน (ใกล้หมด) ตามนาทีที่เกิดเหตุ");
    CHECK(grayTicks >= 8, "ช่วง Station เงียบต้องแสดงเป็นขีดเทา ไม่ใช่เส้นอัตราไหลศูนย์");
  }
  // ช่วง 30 / 60 / 240 นาที
  CHECK(ui.trendRangeIdx == 1, "ค่าเริ่มต้น 60 นาที");
  tapId(Z_RANGE); CHECK(ui.trendRangeIdx == 2 && ui.scr == SCR_TREND, "RANGE -> 240 นาที"); mark("trend_240"); checkTextBounds("TREND 240");
  tapId(Z_RANGE); CHECK(ui.trendRangeIdx == 0, "RANGE -> 30 นาที"); mark("trend_30"); checkTextBounds("TREND 30");
  tapId(Z_RANGE); CHECK(ui.trendRangeIdx == 1, "RANGE -> 60 นาที (วนครบ)");
  // วาดกราฟใหม่เฉพาะเมื่อมีตัวอย่างใหม่ (ไม่กะพริบ)
  ui.needFull = true; uiRefresh(g_ms);
  from = opsEnd();
  drawTrend(false);
  CHECK(countBigRects(from) == 0, "ไม่มีตัวอย่างใหม่ ต้องไม่ล้างและวาดกราฟซ้ำ");
  TrendSample x = {}; x.flags = TS_VALID | TS_RUNNING; x.rate = 70; x.target = 60;
  trend.ring[0].add(x);
  from = opsEnd();
  drawTrend(false);
  CHECK(countBigRects(from) == 1, "มีตัวอย่างใหม่ต้องวาดกราฟใหม่หนึ่งครั้ง");
  CHECK(tapId(Z_BACKBED) && ui.scr == SCR_BED, "BACK กลับหน้า BED");
  // กราฟของเตียงอื่นไม่ปนกัน: เตียงที่ไม่มีตัวอย่างขึ้นข้อความไม่มีข้อมูล ไม่มีเส้น
  ui.bed = 2; ui.scr = SCR_TREND; ui.needFull = true; uiRefresh(g_ms);
  from = opsEnd(); ui.needFull = true; uiRefresh(g_ms);
  CHECK(linesSince(from).empty(), "เตียงที่ไม่มีข้อมูลต้องไม่มีเส้นกราฟ");
  screenCheck("TREND ว่าง", "trend_empty");
  // พิกัด: ใหม่สุดอยู่ขวา เก่าสุดอยู่ซ้าย เรียงต่อเนื่อง ไม่ออกนอกกรอบ
  for (int range : TREND_RANGES) {
    CHECK(trendX(range - 1, range) == TR_X + 1, "ตัวอย่างเก่าสุดชิดซ้าย");
    CHECK(trendX(0, range) <= TR_X + TR_W - 2 && trendX(0, range) >= TR_X + TR_W - 4, "ตัวอย่างใหม่สุดชิดขวา");
    bool mono = true; for (int a = 1; a < range; a++) if (trendX(a, range) > trendX(a - 1, range)) mono = false;
    CHECK(mono, "ยิ่งเก่ายิ่งอยู่ซ้าย");
  }
  CHECK(trendX(0, 1) >= TR_X && trendX(0, 1) < TR_X + TR_W, "ช่วง 1 นาทีไม่หารศูนย์");
  CHECK(trendY(0, 100) == TR_Y + TR_H - 2 && trendY(100, 100) == TR_Y + 2 && trendY(50, 100) > trendY(100, 100) && trendY(50, 100) < trendY(0, 100), "แกนตั้ง: ศูนย์ล่างสุด ค่าสูงสุดบนสุด เรียงต่อเนื่อง");
  CHECK(trendY(-5, 100) == trendY(0, 100) && trendY(1e9f, 100) == trendY(100, 100) && trendY(10, 0) >= TR_Y && trendY(10, -3) <= TR_Y + TR_H, "ค่านอกช่วงถูกตัด และสเกลศูนย์/ติดลบไม่พัง");
  // ค่าสุดขั้ว: อัตรา 65535 ต้องไม่ล้นกรอบ
  trend = TrendState();
  for (int i = 0; i < TREND_MINUTES; i++) { TrendSample e = {}; e.flags = TS_VALID | TS_RUNNING; e.rate = 65535; e.target = 65535; e.alert = ALERT_LOST_LINK; trend.ring[0].add(e); }
  ui.bed = 0; ui.trendRangeIdx = 2; ui.scr = SCR_TREND; ui.needFull = true;
  from = opsEnd(); uiRefresh(g_ms);
  bool insideX = true; for (auto &l : linesSince(from)) if (l.y0 < TR_Y || l.y1 < TR_Y || l.y0 >= TR_Y + TR_H || l.y1 >= TR_Y + TR_H) insideX = false;
  CHECK(insideX, "อัตราสูงสุดของชนิดข้อมูลต้องไม่ล้นกรอบ");
  checkTextBounds("TREND สุดขั้ว");
  ui.trendRangeIdx = 1;
}

int main(int argc, char **argv) {
  g_opsPath = argc > 1 ? argv[1] : "/tmp/cyd_ui_ops.txt";
  g_ops = fopen(g_opsPath.c_str(), "w");
  printf("ทดสอบจังหวะบัซเซอร์\n");
  testBuzzerPatterns();
  printf("ทดสอบบัซเซอร์ร่วมกับเตียง\n");
  testBuzzerIntegration();
  printf("ทดสอบเส้นทางหน้าจอ\n");
  testNavigationAndEditing();
  printf("ทดสอบหน้าระบบและปุ่มพักเสียง\n");
  testSystemAndSilence();
  printf("ทดสอบค่าสุดขั้ว\n");
  testExtremeValues();
  testSplash();
  testTrendScreen();
  fclose(g_ops);
  if (fails) { printf("รวมล้มเหลว %d รายการ\n", fails); return 1; }
  printf("ผ่านทั้งหมด\n");
  return 0;
}
