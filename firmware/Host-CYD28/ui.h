/**
 * @file      ui.h
 * @brief     หน้าจอ Host (HOME / BED / BED SET / NUMPAD / SYSTEM) และการแตะ แนวนอน 320x240
 * @version   2.2.0-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * ทุกปุ่มลงทะเบียนโซนแตะตอนวาด (uiBtn) ตารางโซนจึงเป็นแหล่งเดียวทั้งการวาดและการแตะ ปุ่มไม่มีทางเลื่อนจากที่วาด
 * วาดเต็มจอเฉพาะตอนเปลี่ยนหน้า ระหว่างนั้นอัปเดตเฉพาะตัวเลขด้วยข้อความความยาวคงที่ทับพื้นเดิม จอจึงไม่กะพริบ
 * แนวนอนเท่านั้น (CYD_ROTATION 1 หรือ 3) ลบเตียง/เริ่มถุงใหม่ต้องยืนยันก่อนทุกครั้ง
 *
 * @warning  ถูก `#include` ท้ายไฟล์หลักก่อน `setup()` ห้ามย้ายขึ้นไปบนสุด เพราะใช้ตัวแปร beds activeBeds syncSched buzz ที่ประกาศไว้ด้านบน
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.2.0-CYD | 2026-11-27 | หน้า TREND (กราฟอัตราไหลรายนาที 30/60/240 นาที เส้นเป้าหมาย แถบเตือน ช่วงไม่มีข้อมูล และสรุปค่าเฉลี่ย/สูงสุด/ต่ำสุด/ปริมาตร) เปิดจากปุ่ม GRAPH ในหน้า BED |
 * | 2.1.0-CYD | 2026-11-27 | หน้า BED แสดงชื่อผู้ป่วยต่อท้ายเลขเตียงเมื่อเป็น ASCII ล้วน (ชื่อภาษาไทยแสดงบนเว็บ เพราะฟอนต์ของจอเป็น ASCII) หน้า SYSTEM แสดงชื่อ Wi-Fi ที่ตั้งไว้จริง |
 * | 2.0.3-CYD | 2026-11-27 | แก้ค่า/เริ่มถุงใหม่/รับทราบ/พักเสียงผ่านฟังก์ชันกลางของไฟล์หลักเพื่อลงประวัติ หน้า SYSTEM แสดงจุดเชื่อมต่อ Wi-Fi SD และนาฬิกา |
 * | 2.0.2-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 3 ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "board.h"
#include "display.h"
#include "colors.h"
#include "bed_model.h"
#include "status_screen.h"

#if ((CYD_ROTATION) & 1) == 0
#error "หน้าจอ v2.0 ออกแบบสำหรับแนวนอนเท่านั้น: ตั้ง CYD_ROTATION เป็น 1 หรือ 3"
#endif

enum Screen : uint8_t { SCR_HOME = 0, SCR_BED, SCR_SET, SCR_NUMPAD, SCR_SYSTEM, SCR_TREND };

enum ZoneId : uint8_t {
  Z_NONE = 0,
  Z_BED_ROW = 1,                       // 1..8 = แถวเตียง 1..8 ในรายการ
  Z_SYS = 20, Z_SILENCE, Z_BACK, Z_SETBTN, Z_NEWBAG, Z_ACK, Z_YES, Z_NO,
  Z_F_RATE = 30, Z_F_PLAN, Z_F_DF, Z_F_NEAR, Z_SAVE, Z_CANCEL,
  Z_KEY = 40,                          // 40..49 = ปุ่มเลข 0..9
  Z_KEY_DOT = 50, Z_KEY_DEL, Z_KEY_OK,
  Z_BEDS_DEC = 60, Z_BEDS_INC, Z_BRI_DEC, Z_BRI_INC, Z_BUZZ_TOGGLE, Z_BUZZ_TEST, Z_CAL,
  Z_GRAPH = 70, Z_RANGE, Z_BACKBED
};

struct Zone { int16_t x, y, w, h; uint8_t id; };

enum NumField : uint8_t { NF_RATE = 0, NF_PLAN, NF_NEAR };

struct UiState {
  Screen   scr = SCR_HOME;
  int      bed = 0;
  bool     confirmBag = false;
  BedConfig edit;
  NumField field = NF_RATE;
  char     buf[8] = "";
  bool     needFull = true;
  uint32_t lastRefresh = 0;
  uint8_t  trendRangeIdx = 1;          // 0 = 30 นาที 1 = 60 นาที 2 = 240 นาที
  uint32_t trendDrawnTotal = 0xFFFFFFFFu;   // จำนวนตัวอย่างทั้งหมดของวงแหวนตอนวาดกราฟล่าสุด (วาดใหม่เมื่อมีตัวอย่างใหม่)
};

static UiState ui;
static Zone zones[40];
static uint8_t zoneCount = 0;
static const uint16_t C_PANEL = 0x2104, C_DARK = 0x18C3, C_AMBER = 0xFD20, C_OKGREEN = 0x0600, C_BTNRED = 0xA000;

// ---- ตัวช่วยวาด ----------------------------------------------------------------------------
static void zoneClear() { zoneCount = 0; }

static void txt(int x, int y, int size, uint16_t fg, uint16_t bg, const char *s) {
  tft.setTextSize(size);
  tft.setTextColor(fg, bg);
  tft.setCursor(x, y);
  tft.print(s);
}

// ข้อความกึ่งกลางช่องกว้าง w (ฟอนต์ Adafruit กว้างคงที่ 6 พิกเซลต่อขนาด จึงคำนวณจากจำนวนตัวอักษรได้ตรง)
static void txtc(int x, int y, int w, int size, uint16_t fg, uint16_t bg, const char *s) {
  txt(x + (w - (int)strlen(s) * 6 * size) / 2, y, size, fg, bg, s);
}

static void uiZone(uint8_t id, int x, int y, int w, int h) {
  const Zone z = { (int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, id };
  for (uint8_t i = 0; i < zoneCount; i++) if (zones[i].id == id) { zones[i] = z; return; }   // วาดซ้ำไม่เพิ่มโซน
  if (zoneCount < sizeof(zones) / sizeof(zones[0])) zones[zoneCount++] = z;
}

static void uiBtn(uint8_t id, const char *label, int x, int y, int w, int h, uint16_t fill, uint16_t fg = C_WHITE, int size = 2) {
  uiZone(id, x, y, w, h);
  tft.fillRoundRect(x, y, w, h, 6, fill);
  tft.drawRoundRect(x, y, w, h, 6, C_GRAY);
  txtc(x, y + (h - 8 * size) / 2, w, size, fg, fill, label);
}

static void topBar(const char *title) {
  tft.fillRect(0, 0, tft.width(), 24, C_PANEL);
  txt(6, 4, 2, C_WHITE, C_PANEL, title);
}

// ปริมาตรที่แสดงต้องไม่ยาวเกินช่อง (Station ที่ส่งตัวนับหยดผิดปกติอาจให้ค่ามหาศาล)
static float dispVol(const Bed &b) { const float v = bedVolumeMl(b); return v > 99999.0f ? 99999.0f : v; }

static void fmtLeft(const Bed &b, char *o, size_t n) {
  const int m = bedRemainingMin(b);
  if (m < 0) snprintf(o, n, "--");
  else if (m >= 60) snprintf(o, n, "%dh%02dm", m / 60, m % 60);
  else snprintf(o, n, "%dm", m);
}

static void progressBar(int x, int y, int w, int h, const Bed &b) {
  float f = b.cfg.planVolumeMl > 0 ? bedVolumeMl(b) / b.cfg.planVolumeMl : 0;
  if (f < 0) f = 0;
  if (f > 1) f = 1;
  const int fillW = (int)((w - 2) * f);
  tft.drawRect(x, y, w, h, C_GRAY);
  tft.fillRect(x + 1, y + 1, fillW, h - 2, bedStateColor(b, millis()));
  tft.fillRect(x + 1 + fillW, y + 1, (w - 2) - fillW, h - 2, C_BLACK);
}

// เตียงที่ควรโชว์ใหญ่ที่หน้า HOME: เตียงที่มีเหตุวิกฤตก่อน แล้วใกล้หมด แล้วเตียงแรกที่ออนไลน์
static int focusBed(uint32_t now) {
  int best = -1, bestPri = -1;
  for (int i = 0; i < activeBeds; i++) {
    int pri = 0;
    if (isCriticalAlert(beds[i].alert)) pri = 100 + buzzPriority(beds[i].alert);
    else if (beds[i].alert == ALERT_NEAR_END && !beds[i].nearEndAck) pri = 50;
    else if (bedOnline(beds[i], now) && beds[i].running) pri = 10;
    else if (beds[i].seen) pri = 5;
    if (pri > bestPri) { bestPri = pri; best = i; }
  }
  return best < 0 ? 0 : best;
}

static const char *snoozeText(uint32_t now, char *o, size_t n) {
  const int32_t left = (int32_t)(snoozeUntil - now);
  snprintf(o, n, "SILENCED %d:%02d", (int)(left / 60000), (int)((left / 1000) % 60));
  return o;
}

// ---- หน้า HOME -----------------------------------------------------------------------------
static void drawHome(bool full) {
  const uint32_t now = millis();
  if (full) {
    zoneClear();
    tft.fillScreen(C_BLACK);
    topBar("SMART IV ALERT");
    for (int i = 0; i < activeBeds; i++) uiZone(Z_BED_ROW + i, 182, 28 + i * 19, 134, 18);
    uiBtn(Z_SYS, "SYS", 4, 186, 96, 48, C_PANEL);
    uiZone(Z_SILENCE, 108, 186, 208, 48);
  }
  const int f = focusBed(now);
  const Bed &b = beds[f];
  char s[40];
  const uint16_t col = bedStateColor(b, now);
  tft.drawRoundRect(4, 28, 172, 150, 6, col);
  snprintf(s, sizeof(s), "BED %d  ", f + 1);
  txt(12, 36, 2, C_WHITE, C_BLACK, s);
  snprintf(s, sizeof(s), "%-8s", bedStateText(b, now));
  txt(12, 56, 2, col, C_BLACK, s);
  snprintf(s, sizeof(s), "%4.0f", (double)b.flowRateHr);
  txt(12, 80, 4, C_WHITE, C_BLACK, s);
  txt(116, 104, 1, C_GRAY, C_BLACK, "mL/h");
  snprintf(s, sizeof(s), "TARGET %-5.0f", (double)b.cfg.targetRateHr);
  txt(12, 118, 1, C_GRAY, C_BLACK, s);
  snprintf(s, sizeof(s), "VOL %.0f/%.0f mL   ", (double)dispVol(b), (double)b.cfg.planVolumeMl);
  txt(12, 130, 1, C_GRAY, C_BLACK, s);
  progressBar(12, 144, 156, 10, b);
  char left[12]; fmtLeft(b, left, sizeof(left));
  snprintf(s, sizeof(s), "LEFT %-7s", left);
  txt(12, 160, 1, C_WHITE, C_BLACK, s);
  for (int i = 0; i < activeBeds; i++) {
    const Bed &r = beds[i];
    const uint16_t c = bedStateColor(r, now);
    snprintf(s, sizeof(s), "%d %-8s %5.0f ", i + 1, bedStateText(r, now), (double)r.flowRateHr);
    tft.drawRoundRect(182, 28 + i * 19, 134, 18, 3, i == f ? c : C_DARK);
    txt(188, 33 + i * 19, 1, c, C_BLACK, s);
  }
  // แถบเสียงเตือน / ปุ่มพัก (วาดทับทุกรอบเพราะข้อความเปลี่ยน)
  const uint8_t code = worstAlarm(now);
  uint16_t fill = C_PANEL; const char *label = "ALL CLEAR"; uint16_t fg = C_GREEN;
  char sn[24];
  if (code != ALERT_NONE && !snoozed(now)) { fill = C_BTNRED; label = "TAP TO SILENCE"; fg = C_WHITE; }
  else if (code != ALERT_NONE) { fill = C_DARK; label = snoozeText(now, sn, sizeof(sn)); fg = C_AMBER; }
  tft.fillRoundRect(108, 186, 208, 48, 6, fill);
  tft.drawRoundRect(108, 186, 208, 48, 6, C_GRAY);
  txtc(108, 202, 208, 2, fg, fill, label);
}

// ---- หน้า BED ------------------------------------------------------------------------------
static void drawBed(bool full) {
  const uint32_t now = millis();
  const Bed &b = beds[ui.bed];
  char s[48];
  if (full) {
    zoneClear();
    tft.fillScreen(C_BLACK);
    if (beds[ui.bed].name[0] && nameIsAscii(beds[ui.bed].name)) snprintf(s, sizeof(s), "BED %d %s", ui.bed + 1, beds[ui.bed].name);
    else snprintf(s, sizeof(s), "BED %d", ui.bed + 1);
    s[15] = 0;                                         // ชื่อตัวหนังสือขนาด 2 กว้าง 12 พิกเซล เหลือที่ให้ป้ายสถานะด้านขวา (เริ่มที่ x=200)
    topBar(s);
    uiBtn(Z_BACK, "BACK", 4, 190, 52, 46, C_PANEL, C_WHITE, 1);
    uiBtn(Z_SETBTN, "SET", 60, 190, 52, 46, C_PANEL, C_WHITE, 2);
    uiBtn(Z_GRAPH, "GRAPH", 116, 190, 60, 46, C_PANEL, C_WHITE, 1);
    uiBtn(Z_NEWBAG, "NEW BAG", 180, 190, 76, 46, C_PANEL, C_WHITE, 1);
    uiZone(Z_ACK, 260, 190, 56, 46);
  }
  if (ui.confirmBag) {
    zoneClear();
    tft.fillRoundRect(30, 60, 260, 120, 8, C_PANEL);
    tft.drawRoundRect(30, 60, 260, 120, 8, C_AMBER);
    txtc(30, 74, 260, 2, C_WHITE, C_PANEL, "START NEW BAG?");
    txtc(30, 98, 260, 1, C_GRAY, C_PANEL, "CLEARS DROP COUNT ON THE STATION");
    uiBtn(Z_YES, "YES", 48, 124, 100, 44, C_OKGREEN);
    uiBtn(Z_NO, "NO", 172, 124, 100, 44, C_PANEL);
    return;
  }
  const uint16_t col = bedStateColor(b, now);
  snprintf(s, sizeof(s), "%-8s", bedStateText(b, now));
  txt(200, 4, 2, col, C_PANEL, s);
  snprintf(s, sizeof(s), "%4.0f", (double)b.flowRateHr);
  txt(10, 34, 4, C_WHITE, C_BLACK, s);
  txt(112, 58, 1, C_GRAY, C_BLACK, "mL/h");
  snprintf(s, sizeof(s), "TARGET %-5.0f mL/h", (double)b.cfg.targetRateHr);
  txt(170, 36, 1, C_GRAY, C_BLACK, s);
  snprintf(s, sizeof(s), "PLAN   %-5.0f mL", (double)b.cfg.planVolumeMl);
  txt(170, 48, 1, C_GRAY, C_BLACK, s);
  snprintf(s, sizeof(s), "DROP   %-3d  NEAR %d%%  ", (int)b.cfg.dropFactor, (int)b.cfg.nearEndPct);
  txt(170, 60, 1, C_GRAY, C_BLACK, s);
  progressBar(10, 82, 300, 14, b);
  snprintf(s, sizeof(s), "VOL %.0f/%.0f mL        ", (double)dispVol(b), (double)b.cfg.planVolumeMl);   // ยาวสุด 23 ตัว x 12 px = 276 px
  s[25] = 0;
  txt(10, 104, 2, C_WHITE, C_BLACK, s);
  char left[12]; fmtLeft(b, left, sizeof(left));
  snprintf(s, sizeof(s), "LEFT   %-8s", left);
  txt(10, 128, 2, C_WHITE, C_BLACK, s);
  snprintf(s, sizeof(s), "LINK %3d%%  RSSI %4d  BAT %.1fV%s  ", (int)b.linkPct, (int)b.rssi, (double)b.batteryV, b.idConflict ? " ID!" : "");
  txt(10, 160, 1, b.linkPct < LINK_WEAK_PCT ? C_AMBER : C_GRAY, C_BLACK, s);
  const bool ackable = (b.alert == ALERT_NEAR_END && !b.nearEndAck);
  tft.fillRoundRect(260, 190, 56, 46, 6, ackable ? C_AMBER : C_DARK);
  tft.drawRoundRect(260, 190, 56, 46, 6, C_GRAY);
  txtc(260, 206, 56, 2, ackable ? C_BLACK : C_GRAY, ackable ? C_AMBER : C_DARK, "ACK");
}

// ---- หน้า TREND ------------------------------------------------------------------------------
constexpr int TR_X = 8, TR_Y = 30, TR_W = 304, TR_H = 112;                  // กรอบกราฟ
static const int TREND_RANGES[3] = {30, 60, 240};

// พิกัดของตัวอย่างที่เก่า ago นาที ในหน้าต่างกว้าง range นาที (ใหม่สุดอยู่ขวาสุด) และของค่า v บนแกนตั้ง 0..vmax (ฟังก์ชันล้วน ทดสอบได้)
static int trendX(int ago, int range) { return TR_X + 1 + ((range - 1 - ago) * (TR_W - 3)) / (range > 1 ? range - 1 : 1); }
static int trendY(float v, float vmax) {
  if (!(vmax > 0)) vmax = 1;
  if (v < 0) v = 0;
  if (v > vmax) v = vmax;
  return TR_Y + TR_H - 2 - (int)((v / vmax) * (TR_H - 4) + 0.5f);
}

static void drawTrendChart(const TrendRing &r, int range) {
  tft.fillRect(TR_X, TR_Y, TR_W, TR_H, C_BLACK);
  tft.drawRect(TR_X, TR_Y, TR_W, TR_H, C_GRAY);
  const int n = range > r.count ? r.count : range;
  float vmax = 10;
  for (int i = 0; i < n; i++) { const TrendSample &x = r.newest(i); if (x.flags & TS_VALID) { if (x.rate > vmax) vmax = x.rate; if (x.target > vmax) vmax = x.target; } }
  vmax *= 1.15f;
  for (int g = 1; g <= 3; g++) tft.drawFastHLine(TR_X + 1, TR_Y + (TR_H * g) / 4, TR_W - 2, C_DARK);   // เส้นกริดสามเส้น
  int prevX = -1, prevY = -1, prevTX = -1, prevTY = -1;
  for (int i = n - 1; i >= 0; i--) {                                         // เก่า -> ใหม่
    const TrendSample &x = r.newest(i);
    const int px = trendX(i, range);
    if (!(x.flags & TS_VALID)) {                                              // ช่วงไม่มีข้อมูล: แถบเทาที่ฐาน ไม่ลากเส้น (ไม่ใช่ศูนย์)
      tft.drawFastVLine(px, TR_Y + TR_H - 6, 4, C_GRAY);
      prevX = -1; prevTX = -1;
      continue;
    }
    if (x.target > 0) {                                                       // เส้นเป้าหมาย (เปลี่ยนตามที่ตั้งไว้ ณ ตอนนั้น)
      const int ty = trendY((float)x.target, vmax);
      if (prevTX >= 0) tft.drawLine(prevTX, prevTY, px, ty, C_CYAN);
      prevTX = px; prevTY = ty;
    } else prevTX = -1;
    if (trendAlertCritical(x.alert)) tft.drawFastVLine(px, TR_Y + 2, TR_H - 4, C_BTNRED);   // เหตุวิกฤต: เส้นแดงตลอดแนวตั้ง
    else if (x.alert == ALERT_NEAR_END) tft.drawFastVLine(px, TR_Y + TR_H - 10, 8, C_AMBER);
    const int py = trendY((float)x.rate, vmax);
    const uint16_t col = (x.flags & TS_RUNNING) ? C_GREEN : C_CYAN;          // กำลังนับ = เขียว หยุดชั่วคราว = ฟ้า
    if (prevX >= 0) tft.drawLine(prevX, prevY, px, py, col); else tft.drawPixel(px, py, col);
    prevX = px; prevY = py;
  }
  char s[24];
  snprintf(s, sizeof(s), "%.0f", (double)vmax);
  txt(TR_X + 3, TR_Y + 3, 1, C_GRAY, C_BLACK, s);
  if (n == 0) txtc(TR_X, TR_Y + TR_H / 2 - 4, TR_W, 1, C_GRAY, C_BLACK, "NO DATA YET (1 SAMPLE PER MINUTE)");
}

static void drawTrend(bool full) {
  const int range = TREND_RANGES[ui.trendRangeIdx % 3];
  const TrendRing &r = trend.ring[ui.bed];
  char s[64];
  if (full) {
    zoneClear();
    tft.fillScreen(C_BLACK);
    snprintf(s, sizeof(s), "BED %d TREND", ui.bed + 1);
    topBar(s);
    uiBtn(Z_BACKBED, "BACK", 4, 190, 96, 46, C_PANEL);
    uiBtn(Z_RANGE, "RANGE", 108, 190, 96, 46, C_PANEL);
    ui.trendDrawnTotal = 0xFFFFFFFFu;
  }
  snprintf(s, sizeof(s), "%d MIN", range);
  txt(228, 6, 2, C_AMBER, C_PANEL, s);                                        // "240 MIN" กว้าง 84 พิกเซล ชิดขวาไม่ล้น
  if (ui.trendDrawnTotal != r.totalMinutes || full) {                         // วาดกราฟใหม่เมื่อมีตัวอย่างใหม่เท่านั้น (ไม่กะพริบ)
    ui.trendDrawnTotal = r.totalMinutes;
    drawTrendChart(r, range);
  }
  const TrendSummary sm = trendSummarize(r, range, beds[ui.bed].cfg.dropFactor);
  char t[40];
  snprintf(t, sizeof(t), "AVG %.0f  MIN %u  MAX %u", (double)sm.avgRate, (unsigned)sm.minRate, (unsigned)sm.maxRate);
  snprintf(s, sizeof(s), "%-31.31s", t);                                       // ข้อความฝั่งซ้ายยาวไม่เกิน 31 ตัว (ไม่ชนคอลัมน์คำอธิบายที่ x=200)
  txt(8, 150, 1, C_WHITE, C_BLACK, s);
  snprintf(t, sizeof(t), "GIVEN %.0f mL  IN-RANGE %d%%", (double)sm.totalMl, sm.withinPct);
  snprintf(s, sizeof(s), "%-31.31s", t);
  txt(8, 162, 1, C_WHITE, C_BLACK, s);
  snprintf(t, sizeof(t), "ALERTS %d  NO DATA %d MIN", sm.alertEpisodes, sm.gapMinutes);
  snprintf(s, sizeof(s), "%-31.31s", t);
  txt(8, 174, 1, sm.alertEpisodes ? C_AMBER : C_GRAY, C_BLACK, s);
  if (full) {                                                                  // คำอธิบายสี (คงที่ จึงวาดครั้งเดียว)
    txt(200, 150, 1, C_GREEN, C_BLACK, "GREEN rate");
    txt(200, 162, 1, C_CYAN, C_BLACK, "CYAN target");
    txt(200, 174, 1, C_BTNRED, C_BLACK, "RED alert");
  }
}

// ---- หน้า BED SET ---------------------------------------------------------------------------
static void drawField(uint8_t id, int x, int y, const char *label, const char *value) {
  uiZone(id, x, y, 154, 70);
  tft.fillRoundRect(x, y, 154, 70, 6, C_PANEL);
  tft.drawRoundRect(x, y, 154, 70, 6, C_GRAY);
  txt(x + 8, y + 8, 1, C_GRAY, C_PANEL, label);
  txtc(x, y + 30, 154, 3, C_WHITE, C_PANEL, value);
}

static void drawSet() {
  char s[32];
  zoneClear();
  tft.fillScreen(C_BLACK);
  snprintf(s, sizeof(s), "BED %d SETTINGS", ui.bed + 1);
  topBar(s);
  snprintf(s, sizeof(s), "%.0f", (double)ui.edit.targetRateHr);  drawField(Z_F_RATE, 4, 30, "TARGET RATE (mL/h)", s);
  snprintf(s, sizeof(s), "%.0f", (double)ui.edit.planVolumeMl);  drawField(Z_F_PLAN, 162, 30, "BAG VOLUME (mL)", s);
  snprintf(s, sizeof(s), "%d", (int)ui.edit.dropFactor);         drawField(Z_F_DF, 4, 106, "DROP FACTOR (gtt/mL)", s);
  snprintf(s, sizeof(s), "%d%%", (int)ui.edit.nearEndPct);       drawField(Z_F_NEAR, 162, 106, "NEAR-END WARNING", s);
  uiBtn(Z_SAVE, "SAVE", 4, 190, 154, 46, C_OKGREEN);
  uiBtn(Z_CANCEL, "CANCEL", 162, 190, 154, 46, C_PANEL);
}

// ---- หน้า NUMPAD ----------------------------------------------------------------------------
static bool numpadValue(float &v) {
  if (ui.buf[0] == 0) return false;
  v = (float)atof(ui.buf);
  if (ui.field == NF_RATE) return v >= 0 && v <= MAX_PLAUSIBLE_RATE_HR;
  if (ui.field == NF_PLAN) return v >= 0 && v <= 10000;
  return v >= 50 && v <= 95 && !strchr(ui.buf, '.');
}

static void drawNumpadValue() {
  float v;
  const bool ok = numpadValue(v);
  char s[12];
  snprintf(s, sizeof(s), "%-7s", ui.buf[0] ? ui.buf : "0");
  tft.fillRoundRect(4, 30, 140, 50, 6, C_BLACK);
  tft.drawRoundRect(4, 30, 140, 50, 6, ok || !ui.buf[0] ? C_GRAY : C_RED);
  txt(14, 45, 3, ok || !ui.buf[0] ? C_WHITE : C_RED, C_BLACK, s);
}

static void drawNumpad() {
  zoneClear();
  tft.fillScreen(C_BLACK);
  static const char *names[3] = {"TARGET RATE (mL/h)", "BAG VOLUME (mL)", "NEAR-END (50-95 %)"};
  topBar(names[ui.field]);
  drawNumpadValue();
  uiBtn(Z_KEY_OK, "OK", 4, 100, 140, 46, C_OKGREEN);
  uiBtn(Z_CANCEL, "CANCEL", 4, 150, 140, 46, C_PANEL);
  static const char keys[12] = {'7', '8', '9', '4', '5', '6', '1', '2', '3', '.', '0', '<'};
  for (int k = 0; k < 12; k++) {
    const int col = k % 3, row = k / 3;
    char l[2] = {keys[k], 0};
    const uint8_t id = keys[k] == '.' ? (uint8_t)Z_KEY_DOT : keys[k] == '<' ? (uint8_t)Z_KEY_DEL : (uint8_t)(Z_KEY + (keys[k] - '0'));
    uiBtn(id, l, 152 + col * 56, 30 + row * 52, 52, 48, C_PANEL, C_WHITE, 3);
  }
}

// ---- หน้า SYSTEM ----------------------------------------------------------------------------
static void drawSystem(bool full) {
  char s[40];
  if (full) {
    zoneClear();
    tft.fillScreen(C_BLACK);
    topBar("SYSTEM");
    txt(8, 42, 2, C_WHITE, C_BLACK, "BEDS");
    uiBtn(Z_BEDS_DEC, "-", 176, 32, 40, 36, C_PANEL, C_WHITE, 3);
    uiBtn(Z_BEDS_INC, "+", 276, 32, 40, 36, C_PANEL, C_WHITE, 3);
    txt(8, 82, 2, C_WHITE, C_BLACK, "BRIGHT");
    uiBtn(Z_BRI_DEC, "-", 176, 72, 40, 36, C_PANEL, C_WHITE, 3);
    uiBtn(Z_BRI_INC, "+", 276, 72, 40, 36, C_PANEL, C_WHITE, 3);
    txt(8, 122, 2, C_WHITE, C_BLACK, "BUZZER");
    uiBtn(Z_BUZZ_TEST, "TEST", 254, 112, 62, 36, C_PANEL, C_WHITE, 2);
    uiBtn(Z_CAL, "CALIBRATE", 4, 156, 154, 32, C_PANEL, C_WHITE, 2);
    uiBtn(Z_BACK, "BACK", 162, 156, 154, 32, C_PANEL);
  }
  snprintf(s, sizeof(s), "%d", (int)activeBeds);
  txtc(218, 42, 56, 2, C_WHITE, C_BLACK, s);
  snprintf(s, sizeof(s), "%d%%", (int)sysSet.brightness);
  txtc(218, 82, 56, 2, C_WHITE, C_BLACK, "     ");
  txtc(218, 82, 56, 2, C_WHITE, C_BLACK, s);
  uiBtn(Z_BUZZ_TOGGLE, sysSet.buzzerOn ? "ON" : "OFF", 176, 112, 72, 36, sysSet.buzzerOn ? C_OKGREEN : C_BTNRED, C_WHITE, 2);
  snprintf(s, sizeof(s), "CH %u  RX DROP %lu  BAD %lu  WRONG %lu   ", (unsigned)linkChannel(), (unsigned long)rxDropped, (unsigned long)badPackets, (unsigned long)rxWrongLen);
  txt(8, 196, 1, C_GRAY, C_BLACK, s);
  const uint32_t up = millis() / 1000;
  snprintf(s, sizeof(s), "v%s  UP %lu:%02lu:%02lu  PANEL %d  ", APP_VERSION, (unsigned long)(up / 3600), (unsigned long)((up / 60) % 60), (unsigned long)(up % 60), (int)CYD_PANEL);
  txt(8, 208, 1, C_GRAY, C_BLACK, s);
  snprintf(s, sizeof(s), "WIFI %s  %s  SD %s  CLK %s  ", netCfg.ssid, "192.168.4.1", sdOk ? "OK" : "NO", clockSynced ? "SET" : "NOT SET");
  s[52] = 0;
  txt(8, 220, 1, C_GRAY, C_BLACK, s);
  txt(8, 231, 1, C_GRAY, C_BLACK, "ESP-NOW PROTOCOL V3 - NOT A CERTIFIED ALARM SYSTEM");
}

static void uiRequestFull() { ui.needFull = true; }

// ---- นำทางและการแตะ ---------------------------------------------------------------------------
static void uiGo(Screen sc) { ui.scr = sc; ui.confirmBag = false; ui.needFull = true; }

static void uiDraw(bool full) {
  switch (ui.scr) {
    case SCR_HOME:   drawHome(full); break;
    case SCR_BED:    drawBed(full); break;
    case SCR_SET:    if (full) drawSet(); break;
    case SCR_NUMPAD: if (full) drawNumpad(); break;
    case SCR_SYSTEM: drawSystem(full); break;
    case SCR_TREND:  drawTrend(full); break;
  }
}

static void uiRefresh(uint32_t now) {
  if (ui.needFull) { ui.needFull = false; ui.lastRefresh = now; uiDraw(true); return; }
  if ((uint32_t)(now - ui.lastRefresh) >= 500) { ui.lastRefresh = now; uiDraw(false); }
}

static int zoneAt(int x, int y) {
  for (uint8_t i = 0; i < zoneCount; i++)
    if (x >= zones[i].x && x < zones[i].x + zones[i].w && y >= zones[i].y && y < zones[i].y + zones[i].h) return zones[i].id;
  return Z_NONE;
}

static void uiCommitBeds() { saveBeds(beds, activeBeds); }

static void numpadStart(NumField f) { ui.field = f; ui.buf[0] = 0; uiGo(SCR_NUMPAD); }

static void numpadKey(char c) {
  const size_t n = strlen(ui.buf);
  if (n >= 6) return;
  if (c == '.') {
    if (strchr(ui.buf, '.') || ui.field == NF_NEAR) return;
    if (n == 0) { strcpy(ui.buf, "0."); return; }
  } else if (n == 1 && ui.buf[0] == '0') { ui.buf[0] = c; return; }
  ui.buf[n] = c; ui.buf[n + 1] = 0;
}

// แตะที่พิกัดหน้าจอ (หลังแปลงด้วยค่าคาลิเบรตแล้ว)
static void uiTap(int x, int y, uint32_t now) {
  const int id = zoneAt(x, y);
  if (id == Z_NONE) return;
  buzz.click(now);
  if (id >= Z_BED_ROW && id < Z_BED_ROW + MAX_BEDS) { ui.bed = id - Z_BED_ROW; uiGo(SCR_BED); return; }
  if (id >= Z_KEY && id <= Z_KEY + 9) { numpadKey((char)('0' + id - Z_KEY)); drawNumpadValue(); return; }
  switch (id) {
    case Z_SYS:     uiGo(SCR_SYSTEM); break;
    case Z_BACK:    uiGo(SCR_HOME); break;
    case Z_SILENCE:
      if (worstAlarm(now) != ALERT_NONE) {
        const bool wasSnoozed = snoozed(now);
        snoozeUntil = wasSnoozed ? 0 : now + SNOOZE_MS;
        if (!wasSnoozed) logEvent(0, EV_SILENCE);
      }
      uiDraw(false); break;
    case Z_SETBTN:  ui.edit = beds[ui.bed].cfg; uiGo(SCR_SET); break;
    case Z_GRAPH:   uiGo(SCR_TREND); break;
    case Z_BACKBED: uiGo(SCR_BED); break;
    case Z_RANGE:   ui.trendRangeIdx = (uint8_t)((ui.trendRangeIdx + 1) % 3); ui.needFull = true; break;
    case Z_NEWBAG:  ui.confirmBag = true; uiDraw(false); break;
    case Z_NO:      ui.confirmBag = false; ui.needFull = true; break;
    case Z_YES:
      doNewBag(ui.bed);
      ui.confirmBag = false; ui.needFull = true; break;
    case Z_ACK:
      doAck(ui.bed);
      break;
    case Z_F_RATE:  numpadStart(NF_RATE); break;
    case Z_F_PLAN:  numpadStart(NF_PLAN); break;
    case Z_F_NEAR:  numpadStart(NF_NEAR); break;
    case Z_F_DF: {
      const uint8_t d = ui.edit.dropFactor;
      ui.edit.dropFactor = d == 10 ? 15 : d == 15 ? 20 : d == 20 ? 60 : 10;
      uiGo(SCR_SET); break;
    }
    case Z_SAVE: {
      commitBedConfig(ui.bed, ui.edit);                 // ฟังก์ชันกลางคง resetSeq จริงของ Host ไว้เอง (ไม่งั้น Station จะล้างตัวนับ)
      uiGo(SCR_BED); break;
    }
    case Z_CANCEL:  uiGo(ui.scr == SCR_NUMPAD ? SCR_SET : SCR_BED); break;
    case Z_KEY_DOT: numpadKey('.'); drawNumpadValue(); break;
    case Z_KEY_DEL: { const size_t n = strlen(ui.buf); if (n) ui.buf[n - 1] = 0; drawNumpadValue(); break; }
    case Z_KEY_OK: {
      float v;
      if (!numpadValue(v)) break;
      if (ui.field == NF_RATE) ui.edit.targetRateHr = v;
      else if (ui.field == NF_PLAN) ui.edit.planVolumeMl = v;
      else ui.edit.nearEndPct = (uint8_t)(v + 0.5f);
      uiGo(SCR_SET); break;
    }
    case Z_BEDS_DEC: if (activeBeds > 1) { activeBeds--; uiCommitBeds(); } uiDraw(false); break;
    case Z_BEDS_INC: if (activeBeds < MAX_BEDS) { activeBeds++; uiCommitBeds(); syncSched.requestAll(activeBeds); } uiDraw(false); break;
    case Z_BRI_DEC: if (sysSet.brightness > 10) sysSet.brightness -= 10; backlightSet(sysSet.brightness); saveSys(sysSet); uiDraw(false); break;
    case Z_BRI_INC: if (sysSet.brightness < 100) sysSet.brightness += 10; backlightSet(sysSet.brightness); saveSys(sysSet); uiDraw(false); break;
    case Z_BUZZ_TOGGLE: sysSet.buzzerOn = !sysSet.buzzerOn; buzz.enabled = sysSet.buzzerOn; saveSys(sysSet); uiDraw(false); break;
    case Z_BUZZ_TEST: buzz.test(now); break;
    case Z_CAL: runCalibration(); ui.needFull = true; break;
    default: break;
  }
}

// เรียกทุกรอบของ loop() ขณะอยู่หน้าจอใช้งาน: อ่านทัช แล้ววาดส่วนที่เปลี่ยน
static void uiStep(uint32_t now) {
  int16_t rx = 0, ry = 0, z = 0, sx = 0, sy = 0;
  const bool hit = touchChip.read(rx, ry, z);
  const TouchEvt ev = tracker.update(hit, now);
  if (hit) touchCalApply(cal, rx, ry, tft.width(), tft.height(), sx, sy);
  if (ev == TOUCH_DOWN) uiTap(sx, sy, now);
  uiRefresh(now);
}
