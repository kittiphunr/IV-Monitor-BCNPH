/**
 * @file      Host-CYD28.ino
 * @brief     Host บอร์ด CYD (ESP32-2432S028) เขียนใหม่ ด่านที่ 4 (ครบทุกด่าน): เว็บแดชบอร์ด นาฬิกา SD ประวัติเหตุการณ์
 * @version   2.0.3-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Hardware
 * ESP32-2432S028 (CYD) จอ ILI9341 หรือ ST7789 240x320 ทัช XPT2046 (ขาแยกจากบัสจอ)
 *
 * @par Description
 * ด่านที่ 2 ของการเขียน Host ใหม่ รับแพ็กเก็ตจาก Station เข้าคิว ตัดสินรหัสเตือนที่ Host จุดเดียว
 * แล้วส่ง Sync กลับเป็นช่องเวลาไม่บล็อก ค่าตั้งเตียงเก็บใน NVS ยังไม่มีหน้าจอสุดท้าย (ด่านที่ 3)
 * ใช้หน้าสถานะแบบตารางและคำสั่ง Serial แทนเพื่อตรวจกับ Station จริง
 * ตรรกะทั้งหมดอยู่ใน bed_model.h ซึ่งทดสอบบน PC ได้ทุกกรณี (tools/cyd-bed-test)
 *
 * @par คำสั่ง Serial (115200 จบบรรทัดด้วย Enter)
 *   table · beds <จำนวน> · rate <เตียง> <mL/h> · plan <เตียง> <mL> · df <เตียง> <10|15|20|60>
 *   near <เตียง> <%> · newbag <เตียง> · ack <เตียง> · m (สลับหน้าสถานะ/หน้าทดสอบจอ)
 *   i b r c + - d h (ตามด่านที่ 1: สลับสี BGR ทิศ คาลิเบรต ความสว่าง รหัสชิป ช่วยเหลือ)
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.3-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 4 ไม่นำโค้ดรุ่น 1.x มาปรับ: จุดเชื่อมต่อ Wi-Fi + เว็บแดชบอร์ดหน้าเดียว (ดูได้ทันที แก้ค่าต้องล็อกอิน), นาฬิกาตั้งจากเบราว์เซอร์ (RTC DS3231 ไม่บังคับ), ประวัติเหตุการณ์ในหน่วยความจำ 48 รายการ + บันทึก CSV ลง SD, คำสั่งแก้ค่าทุกทางผ่านฟังก์ชันเดียว (commitBedConfig/doNewBag/doAck) จึงลงประวัติเหมือนกัน |
 * | 2.0.2-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 3 ไม่นำโค้ดรุ่น 1.x มาปรับ: หน้า HOME/BED/BED SET/NUMPAD/SYSTEM แนวนอน, โซนแตะตารางเดียวกับที่วาด, บัซเซอร์ Active (เตือน/เสียงกด/ใกล้หมด), ปุ่มพักเสียง 2 นาที, ยืนยันก่อนเริ่มถุงใหม่ |
 * | 2.0.1-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 2 ไม่นำโค้ดรุ่น 1.x มาปรับ: คิวรับ ESP-NOW ผู้เขียน/ผู้อ่านคนเดียว, bed_model.h ตรรกะล้วนทดสอบบน PC, ตัวจัดช่องเวลาส่ง Sync, NVS เก็บ resetSeq ถาวร |
 * | 2.0.0-CYD | 2026-11-27 | เขียนใหม่ทั้งหมดด่านที่ 1 (ฐานบอร์ด: จอ ไฟหลังจอ ทัช) |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#include <Preferences.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "board.h"
#include "display.h"
#include "colors.h"
#include "touch_cal.h"
#include "touch_xpt2046.h"
#include "calibration_ui.h"
#include "protocol.h"
#include "bed_model.h"
#include "bed_store.h"
#include "espnow_link.h"
#include "diag_screen.h"
#include "status_screen.h"
#include "buzzer.h"
#include "sys_store.h"
#include "event_log.h"
#include "clock_sync.h"
#include "sd_log.h"

#define APP_VERSION "2.0.3-CYD"

enum AppMode : uint8_t { MODE_UI = 0, MODE_DIAG };

constexpr uint32_t SNOOZE_MS = 120000;       // พักเสียงเตือน 2 นาที

Bed beds[MAX_BEDS];
uint8_t activeBeds = DEFAULT_ACTIVE_BEDS;
SyncScheduler syncSched;
EventLog events;
static bool wasOnline[MAX_BEDS] = {false};
AppMode appMode = MODE_UI;
BuzzerEngine buzz;
SysSettings sysSet = {SYS_MAGIC, 100, 1};
uint32_t snoozeUntil = 0;
static uint32_t lastNearChime[MAX_BEDS] = {0};
uint32_t badPackets = 0, ignoredPackets = 0;
static uint32_t lastEvalMs = 0, lastLinkTickMs = 0;
static char cmdLine[64];
static uint8_t cmdLen = 0;

// ---- ประวัติเหตุการณ์ และการกระทำที่ทุกทาง (หน้าจอ เว็บ Serial) ใช้ร่วมกัน ----------------------------
static void logEvent(uint8_t bed, uint8_t type, uint8_t code = 0) {
  events.add(millis(), clockEpoch(), bed, type, code);
  sdLogEvent(events.newest(0));
}

// บันทึกค่าตั้งเตียง (resetSeq ใช้ค่าจริงของ Host เสมอ ห้ามรับจากภายนอก ไม่งั้น Station จะล้างตัวนับเอง)
static void commitBedConfig(int i, BedConfig c) {
  if (i < 0 || i >= MAX_BEDS) return;
  c.resetSeq = beds[i].cfg.resetSeq;
  beds[i].cfg = c;
  saveBeds(beds, activeBeds);
  syncSched.request(i);
  logEvent((uint8_t)(i + 1), EV_CONFIG);
}

static void doNewBag(int i) {
  if (i < 0 || i >= MAX_BEDS) return;
  bedNewBag(beds[i], millis());
  saveBeds(beds, activeBeds);
  syncSched.request(i);
  logEvent((uint8_t)(i + 1), EV_NEWBAG);
}

// รับทราบเตือนใกล้หมด คืน false ถ้าไม่มีอะไรให้รับทราบ
static bool doAck(int i) {
  if (i < 0 || i >= MAX_BEDS) return false;
  if (beds[i].alert != ALERT_NEAR_END || beds[i].nearEndAck) return false;
  beds[i].nearEndAck = true;
  syncSched.request(i);
  logEvent((uint8_t)(i + 1), EV_ACK);
  return true;
}

// ประกาศล่วงหน้า: นิยามอยู่ใน ui.h ซึ่งถูก include ท้ายไฟล์ (ห้ามใส่ค่าปริยายซ้ำที่นิยาม)
static void uiRequestFull();

static void setMode(AppMode m) {
  appMode = m;
  if (!cal.valid && !loadCal()) runCalibration();    // ทั้งหน้าจอใช้งานและหน้าทดสอบต้องใช้ทัช
  if (m == MODE_DIAG) drawTestScreen();
  else uiRequestFull();
}

// ---- เสียงเตือน ------------------------------------------------------------------------------
// รหัสเตือนที่ร้องแรงที่สุดในทุกเตียง (ไม่รวมใกล้หมด ซึ่งเป็นเสียงเตือนเบาแยกต่างหาก)
static uint8_t worstAlarm(uint32_t now) {
  (void)now;
  uint8_t best = ALERT_NONE;
  for (int i = 0; i < activeBeds; i++)
    if (isCriticalAlert(beds[i].alert) && buzzPriority(beds[i].alert) > buzzPriority(best)) best = beds[i].alert;
  return best;
}

static bool snoozed(uint32_t now) { return snoozeUntil != 0 && (int32_t)(snoozeUntil - now) > 0; }

static void serviceBuzzer(uint32_t now) {
  const uint8_t code = worstAlarm(now);
  if (code == ALERT_NONE) snoozeUntil = 0;                 // เหตุการณ์หายแล้ว ให้เหตุครั้งหน้าดังทันที
  buzz.setAlert(snoozed(now) ? (uint8_t)ALERT_NONE : code, now);
  if (code == ALERT_NONE || snoozed(now)) {
    for (int i = 0; i < activeBeds; i++) {                 // ใกล้หมดที่ยังไม่รับทราบ: 3 ครั้งสั้น ๆ ซ้ำทุก 5 นาที
      if (beds[i].alert != ALERT_NEAR_END || beds[i].nearEndAck) { lastNearChime[i] = 0; continue; }
      if (lastNearChime[i] == 0 || (uint32_t)(now - lastNearChime[i]) >= 300000UL) { lastNearChime[i] = now ? now : 1; buzz.chime(now); break; }
    }
  }
  if (CYD_BUZZER_PIN >= 0) {
    const bool on = buzz.update(now);
    digitalWrite(CYD_BUZZER_PIN, on ? CYD_BUZZER_ON_LEVEL : (CYD_BUZZER_ON_LEVEL == HIGH ? LOW : HIGH));
  }
}

static void printTable() {
  const uint32_t now = millis();
  Serial.println("bed state     rate  vol(mL) link rssi drops");
  for (int i = 0; i < activeBeds; i++) {
    char l[96];
    snprintf(l, sizeof(l), "%d   %-8s %5.0f %7.0f %3d%% %4d %lu", i + 1, bedStateText(beds[i], now), (double)beds[i].flowRateHr,
             (double)bedVolumeMl(beds[i]), (int)beds[i].linkPct, (int)beds[i].rssi, (unsigned long)beds[i].totalDrops);
    Serial.println(l);
  }
  char f[128];
  snprintf(f, sizeof(f), "ch %u active %u bad %lu ignored %lu rxDropped %lu wrongLen %lu syncFail %lu", (unsigned)linkChannel(),
           (unsigned)activeBeds, (unsigned long)badPackets, (unsigned long)ignoredPackets, (unsigned long)rxDropped,
           (unsigned long)rxWrongLen, (unsigned long)syncSendFail);
  Serial.println(f);
}

// คำสั่งที่มีเลขเตียง (1..activeBeds) คืนดัชนี 0-based หรือ -1
static int bedArg(int n) { return (n >= 1 && n <= activeBeds) ? n - 1 : -1; }

static void handleLine(char *line) {
  char cmd[12] = "";
  int a = 0;
  float v = 0;
  const int n = sscanf(line, "%11s %d %f", cmd, &a, &v);
  if (n < 1) return;
  const int i = bedArg(a);
  if (!strcmp(cmd, "table")) { printTable(); return; }
  if (!strcmp(cmd, "beds") && n >= 2) {
    if (a < 1 || a > MAX_BEDS) { Serial.println("beds 1-8"); return; }
    activeBeds = (uint8_t)a; saveBeds(beds, activeBeds); syncSched.requestAll(activeBeds);
    uiRequestFull();
    return;
  }
  if (!strcmp(cmd, "rate") && n >= 3 && i >= 0) {
    if (!(v >= 0 && v <= MAX_PLAUSIBLE_RATE_HR)) { Serial.println("rate 0-3000"); return; }
    { BedConfig c = beds[i].cfg; c.targetRateHr = v; commitBedConfig(i, c); } return;
  }
  if (!strcmp(cmd, "plan") && n >= 3 && i >= 0) {
    if (!(v >= 0 && v <= 10000)) { Serial.println("plan 0-10000"); return; }
    { BedConfig c = beds[i].cfg; c.planVolumeMl = v; commitBedConfig(i, c); } return;
  }
  if (!strcmp(cmd, "df") && n >= 3 && i >= 0) {
    const int d = (int)v;
    if (d != 10 && d != 15 && d != 20 && d != 60) { Serial.println("df 10|15|20|60"); return; }
    { BedConfig c = beds[i].cfg; c.dropFactor = (uint8_t)d; commitBedConfig(i, c); } return;
  }
  if (!strcmp(cmd, "near") && n >= 3 && i >= 0) {
    const int p = (int)v;
    if (p < 50 || p > 95) { Serial.println("near 50-95"); return; }
    { BedConfig c = beds[i].cfg; c.nearEndPct = (uint8_t)p; commitBedConfig(i, c); } return;
  }
  if (!strcmp(cmd, "newbag") && n >= 2 && i >= 0) {
    doNewBag(i); return;
  }
  if (!strcmp(cmd, "ack") && n >= 2 && i >= 0) {
    doAck(i); return;
  }
  if (!strcmp(cmd, "m")) { setMode(appMode == MODE_UI ? MODE_DIAG : MODE_UI); return; }
  if (!strcmp(cmd, "i")) { panelSetInvert(!g_invert); if (appMode == MODE_DIAG) drawTestScreen(); printSettings(); return; }
  if (!strcmp(cmd, "b")) { panelSetBgr(!g_bgr); if (appMode == MODE_DIAG) drawTestScreen(); printSettings(); return; }
  if (!strcmp(cmd, "r")) { panelSetRotation(g_rotation + 1); setMode(MODE_DIAG); runCalibration(); drawTestScreen(); printSettings(); return; }
  if (!strcmp(cmd, "c")) { setMode(MODE_DIAG); runCalibration(); drawTestScreen(); return; }
  if (!strcmp(cmd, "+")) { applyBrightness(+10); return; }
  if (!strcmp(cmd, "-")) { applyBrightness(-10); return; }
  if (!strcmp(cmd, "d")) { panelPrintId(); return; }
  Serial.println("table | beds N | rate B mL/h | plan B mL | df B 10|15|20|60 | near B % | newbag B | ack B | m | i b r c + - d");
}

static void handleSerial() {
  while (Serial.available()) {
    const int c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') { cmdLine[cmdLen] = 0; cmdLen = 0; handleLine(cmdLine); continue; }
    if (cmdLen < sizeof(cmdLine) - 1) cmdLine[cmdLen++] = (char)c;
  }
}

// ดึงแพ็กเก็ตจากคิวมาใส่เตียง
static void serviceRx(uint32_t now) {
  RxPacket p;
  while (rxPop(p)) {
    if (!packetPlausible(p.m, MAX_BEDS)) { badPackets++; continue; }
    const int idx = p.m.stationId - 1;
    if (idx >= activeBeds) { ignoredPackets++; continue; }
    const uint8_t beforeAck = beds[idx].nearEndAck;
    bedIngest(beds[idx], p.m, p.rssi, p.mac, now);
    if (!beforeAck && beds[idx].nearEndAck) syncSched.request(idx);   // ยืนยันรับทราบกลับให้ Station ทันที
  }
}

static void serviceAlerts(uint32_t now) {
  for (int i = 0; i < activeBeds; i++) {
    const uint8_t code = bedEvaluate(beds[i], now);
    const bool on = bedOnline(beds[i], now);
    if (beds[i].seen && on != wasOnline[i]) logEvent((uint8_t)(i + 1), on ? EV_ONLINE : EV_OFFLINE);
    wasOnline[i] = on;
    if (code != beds[i].alert) {
      syncSched.request(i);                                           // รหัสเปลี่ยน ส่งทันทีไม่รอรอบ
      if (code == ALERT_NONE) logEvent((uint8_t)(i + 1), EV_CLEAR, beds[i].alert);
      else logEvent((uint8_t)(i + 1), EV_ALERT, code);
    }
    beds[i].alert = code;
  }
}

#include "web_server.h"  // เว็บ (ใช้ commitBedConfig doNewBag doAck ด้านบน)
#include "ui.h"        // หน้าจอทั้งหมด (ต้องอยู่ท้ายไฟล์หลักก่อน setup() เพราะใช้ตัวแปรด้านบน)

void setup() {
  Serial.begin(115200);
  pinMode(cyd::BOOT_BTN, INPUT_PULLUP);
  if (CYD_BUZZER_PIN >= 0) {
    pinMode(CYD_BUZZER_PIN, OUTPUT);
    digitalWrite(CYD_BUZZER_PIN, CYD_BUZZER_ON_LEVEL == HIGH ? LOW : HIGH);   // เงียบไว้ก่อนเสมอ
  }
  panelBegin();
  sysSet = loadSys();
  backlightSet(sysSet.brightness);
  buzz.enabled = sysSet.buzzerOn;
  touchChip.begin();
  activeBeds = loadBeds(beds);
  const bool linkOk = linkBegin(true);   // เปิดจุดเชื่อมต่อ Wi-Fi ของ Host ด้วย (หน้าเว็บ)
  Serial.println("Host-CYD28 " APP_VERSION " (stage 4: web + clock + SD + events)");
  printSettings();
  Serial.println(linkOk ? "ESP-NOW ready" : "ESP-NOW INIT FAILED");
  clockBegin();
  sdLogBegin();
  webBegin();
  Serial.println("AP " CYD_AP_SSID " http://192.168.4.1/");
  logEvent(0, EV_BOOT);
  syncSched.requestAll(activeBeds);
  setMode(MODE_UI);
}

void loop() {
  const uint32_t now = millis();
  handleSerial();
  webService();
  serviceRx(now);
  if ((uint32_t)(now - lastEvalMs) >= 250) { lastEvalMs = now; serviceAlerts(now); }
  if ((uint32_t)(now - lastLinkTickMs) >= LINK_WINDOW_MS) { lastLinkTickMs = now; bedsLinkTick(beds, MAX_BEDS); }
  const int s = syncSched.next(now, activeBeds);
  if (s >= 0) linkSendSync(bedBuildSync(beds[s], s, linkChannel(), clockEpoch(), now));
  serviceBuzzer(now);
  if (appMode == MODE_UI) uiStep(now); else diagStep();
  if (digitalRead(cyd::BOOT_BTN) == LOW) {            // BOOT สั้น = สลับหน้า · ค้าง 1.5 วินาที = คาลิเบรตใหม่
    const uint32_t t0 = millis();
    while (digitalRead(cyd::BOOT_BTN) == LOW && millis() - t0 < 1500) delay(10);
    if (millis() - t0 >= 1500) { setMode(MODE_DIAG); runCalibration(); drawTestScreen(); }
    else setMode(appMode == MODE_UI ? MODE_DIAG : MODE_UI);
    while (digitalRead(cyd::BOOT_BTN) == LOW) delay(10);
  }
  delay(2);
}
