/**
 * @file      HostScreen.h
 * @brief     ทุกอย่างที่วาดลงจอสัมผัส 2.8 นิ้ว ของเครื่องส่วนกลาง บอร์ด CYD
 * @version   1.1.0-CYD
 * @date      2026-11-23
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 * อาจารย์ที่ปรึกษา ดร.กรรณิการ์ กาศสมบูรณ์
 *
 * @par Description
 * สำเนาจาก `Host-Touch24/HostScreen.h` v4.9.3 ส่วนผังแนวตั้งเหมือนกันทุกบรรทัด ผังแนวนอนเพิ่มใหม่ในรุ่นนี้
 * แยกออกมาจากไฟล์หลักเพื่อให้ไฟล์หลักสั้นและหาของเจอเร็วขึ้น
 * มีหน้ารวม หน้ารายเตียง หน้าตั้งค่า แป้นตัวเลข หน้าระบบ และหน้าคาลิเบรตจอสัมผัส
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 1.1.0-CYD | 2026-11-23 | เพิ่มผังแนวนอน 320x240 ทุกหน้า (เลือกด้วย `UI_LANDSCAPE`) ผังแนวตั้งเดิมไม่เปลี่ยนแม้แต่พิกเซล |
 * | 1.0.0-CYD | 2026-11-22 | คัดลอกจาก Host จอสัมผัส 2.4" v4.9.3 มาใช้กับบอร์ด ESP32-2432S028 ไม่ได้แก้โค้ด |
 *
 * @warning  ถูก `#include` ท้ายไฟล์หลักก่อน `setup()` ห้ามย้ายขึ้นไปบนสุด
 *           เพราะโค้ดในนี้ใช้ตัวแปรและฟังก์ชันช่วยที่ประกาศไว้เหนือบรรทัด include
 *           ถ้าโค้ดเหนือบรรทัดนั้นเรียกฟังก์ชันในไฟล์นี้ ต้องประกาศล่วงหน้าในไฟล์หลักด้วย
 *           ตรวจด้วย `bash tools/split-check/run.sh`
 *
 * @warning  ข้อความที่วาดลงจอต้องเป็น ASCII ล้วน ฟอนต์ในตัวของไลบรารีไม่มีอักษรไทย
 *           อักษรไทยหนึ่งตัวกินสามไบต์ จะกลายเป็นสัญลักษณ์มั่วสามตัวและกินที่กว้างกว่าที่นับไว้
 *
 * @note     Arduino IDE แสดงไฟล์นี้เป็นแท็บของสเก็ตช์เดียวกัน เปิดคู่กับไฟล์หลักได้เลย
 *
 * @par บันทึกการเปลี่ยนแปลงโดยละเอียด
 * เก็บข้อความเดิมไว้ทั้งหมด เพราะเหตุผลเชิงเทคนิคในนั้นหาจากที่อื่นไม่ได้
 *  @file      HostScreen.h
 *  @brief     ทุกอย่างที่วาดลงจอสัมผัส 2.4" ของเครื่องส่วนกลาง (ข้อความเดิมจากรุ่น 2.4")
 *  @version   4.9.3
 *
 *  แยกออกมาจาก Host-Touch24.ino เพื่อให้ไฟล์หลักสั้นและอ่านง่าย
 *  คนที่มารับช่วงดูแลต่อจะได้หาของเจอเร็วขึ้น
 *
 *  เป็นการ **ย้ายที่อยู่ล้วน ๆ** ไม่ได้แก้เนื้อในแม้แต่บรรทัดเดียว
 *  พิสูจน์ด้วยการเรนเดอร์ภาพหน้าจอก่อนและหลังแยก แล้วเทียบทีละพิกเซล
 *
 *  @warning ถูก #include ท้ายไฟล์หลักก่อน setup() ห้ามย้ายขึ้นไปบนสุด
 *           เพราะโค้ดในนี้ใช้ตัวแปรและฟังก์ชันช่วยที่ประกาศไว้เหนือบรรทัด include
 *  @note    Arduino IDE แสดงไฟล์นี้เป็นแท็บของสเก็ตช์เดียวกัน เปิดคู่กันได้เลย
 */

#pragma once

// ---- ตัวช่วยวางข้อความ (ฟอนต์มาตรฐาน 6xsize กว้าง, 8xsize สูง) ----
void textAt(const String &s, int x, int y, uint8_t size, uint16_t col, uint16_t bg) {
  tft.setTextSize(size);
  tft.setTextColor(col, bg);
  tft.setCursor(x, y);
  tft.print(s);
}

// กระเปาะหยดขนาดเล็กสำหรับหน้ารายละเอียดเตียง
void drawDripChamber(int x, int y, int w, int h, int idx) {
  tft.drawRoundRect(x, y, w, h, 4, C_DIM);
  int cx = x + w / 2;
  tft.fillRect(cx - 2, y + 2, 4, 5, C_SKY);            // หัวหยด
  int poolY = y + h - 7;
  tft.fillRoundRect(x + 3, poolY, w - 6, 5, 2, C_SKY); // ผิวน้ำด้านล่าง

  int ph = dripAnimPhase(idx);
  if (ph >= 0) {
    int top = y + 8, bot = poolY - 3;
    int dy = top + ((bot - top) * ph) / 100;
    tft.fillCircle(cx, dy, 2, C_CYAN);
  }
}

void drawProgressBar(int x, int y, int w, int h, int pct, uint16_t fill) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  tft.fillRoundRect(x, y, w, h, h / 2, C_BAR);
  int fw = w * pct / 100;
  if (fw >= h) tft.fillRoundRect(x, y, fw, h, h / 2, fill);
  else if (fw > 0) tft.fillRect(x, y, fw, h, fill);
}

void drawWifiIcon(int x, int y, int clients) {
  int h[4] = {3, 6, 9, 12};
  for (int i = 0; i < 4; i++) tft.fillRect(x + i * 4, y + (12 - h[i]), 3, h[i], (clients > i) ? C_GREEN : C_LINE);
}

void drawBatteryIcon(int x, int y, int pct) {
  tft.drawRoundRect(x, y, 20, 10, 2, C_DIM);
  tft.fillRect(x + 20, y + 3, 2, 4, C_DIM);
  tft.fillRect(x + 2, y + 2, 16, 6, C_BG);
  uint16_t c = (pct <= 20) ? C_RED : (pct <= 50 ? C_YELLOW : C_GREEN);
  int w = (16 * pct) / 100;
  if (w > 0) tft.fillRect(x + 2, y + 2, w, 6, c);
}

void drawButton(const TouchZone &z, const String &label, uint16_t bg, uint16_t fg, uint8_t size = 1) {
  tft.fillRoundRect(z.x, z.y, z.w, z.h, 6, bg);
  textCenterIn(label, z.x, z.w, z.y + (z.h - 8 * size) / 2, size, fg, bg);
}

// ============================================================================
// แถบบนสุดและแถบล่างที่ใช้ร่วมกันทุกหน้า
// ============================================================================
void drawStatusBar(bool force) {
  if (force) {
    tft.fillRect(0, 0, SCR_W, TOPBAR_H, C_TOPBAR);
    cacheClock = ""; cacheTopRight = "";
  }
  String clockStr = getDisplayClockStr();
  if (clockStr != cacheClock) {
    cacheClock = clockStr;
    textAt(clockStr, 6, 7, 1, isTimeApprox ? C_YELLOW : C_WHITE, C_TOPBAR);
  }
  String right = String(WiFi.softAPgetStationNum()) + ":" + String(hostBatteryPct / 5);
  if (right != cacheTopRight) {
    cacheTopRight = right;
    drawWifiIcon(SCR_W - 50, 5, (int)WiFi.softAPgetStationNum());
    drawBatteryIcon(SCR_W - 28, 6, hostBatteryPct);
  }
}

void drawTitleBar(const String &title) {
  tft.fillRect(0, TOPBAR_H, SCR_W, 36, C_CARD);
  drawButton(ZONE_BACK, "<", C_BTN, C_WHITE, 2);
  textCenterIn(title, 52, SCR_W - 104, 32, 2, C_WHITE, C_CARD);
}

void drawHomeFocusPanel(int focus, bool force) {
  BedUiStatus st = bedUiStatus(focus);
  const StationData &s = stations[focus];
  uint16_t sc = bedColor(st);

  char head[16];
  snprintf(head, sizeof(head), "BED %02d", focus + 1);
  String key = String(head) + "|" + String((int)st);
  if (force || key != cacheFocusKey) {
    cacheFocusKey = key;
    tft.fillRoundRect(FOCUS_X, FOCUS_Y, FOCUS_W, FOCUS_H, 8, C_CARD);
    tft.fillRoundRect(FOCUS_X, FOCUS_Y, FOCUS_W, 26, 8, sc);
    tft.fillRect(FOCUS_X, FOCUS_Y + 18, FOCUS_W, 8, sc);
    textAt(String(head), FOCUS_X + 8, FOCUS_Y + 6, 2, C_BG, sc);
    textRight(String(bedWord(st)), FOCUS_X + FOCUS_W - 8, FOCUS_Y + 9, 1, C_BG, sc);
    cacheRate = ""; cacheSetRate = ""; cacheSummary = ""; cachePct = -999;
  }

  String rateStr = (st == BU_OFFLINE || st == BU_PAUSED) ? String("--")
                                                         : String((int)(s.flowRate_ml_hr + 0.5f));
  if (rateStr != cacheRate) {
    cacheRate = rateStr;
#if UI_LANDSCAPE
    tft.fillRect(FOCUS_X + 8, 66, 120, 48, C_CARD);                       // [1.1.0-CYD] เพิ่ม: ตัวเลขใหญ่ขึ้นเป็นขนาด 6 ในแนวนอน
    textAt(rateStr, FOCUS_X + 8, 66, 6, bedIsAlarm(st) ? sc : C_WHITE, C_CARD);
#else
    tft.fillRect(FOCUS_X + 8, 58, 120, 40, C_CARD);
    textAt(rateStr, FOCUS_X + 8, 58, 5, bedIsAlarm(st) ? sc : C_WHITE, C_CARD);
#endif
  }

  String setStr = (s.cfg.targetRateHr > 0) ? ("SET " + String((int)(s.cfg.targetRateHr + 0.5f))) : String("SET --");
  if (setStr != cacheSetRate) {
    cacheSetRate = setStr;
#if UI_LANDSCAPE
    textAt("mL/h", FOCUS_X + 8, 120, 1, C_DIM, C_CARD);
    textRight(padTo(setStr, 9), FOCUS_X + FOCUS_W - 8, 120, 1, C_SKY, C_CARD);
#else
    textAt("mL/h", FOCUS_X + 132, 80, 1, C_DIM, C_CARD);
    textRight(padTo(setStr, 9), FOCUS_X + FOCUS_W - 8, 62, 1, C_SKY, C_CARD);
#endif
  }

  int pct = bedPct(focus);
  if (pct != cachePct) {
    cachePct = pct;
#if UI_LANDSCAPE
    drawProgressBar(FOCUS_X + 8, 140, FOCUS_W - 16, 12, (pct < 0) ? 0 : pct,
                    (st == BU_NEAREND) ? C_ORANGE : C_CYAN);
#else
    drawProgressBar(FOCUS_X + 8, 104, FOCUS_W - 16, 10, (pct < 0) ? 0 : pct,
                    (st == BU_NEAREND) ? C_ORANGE : C_CYAN);
#endif
  }

  int mins = bedMinutesLeft(focus);
  char tl[16];
  if (mins >= 0) snprintf(tl, sizeof(tl), "%dh %02dm", mins / 60, mins % 60);
  else           snprintf(tl, sizeof(tl), "--h --m");
  char vol[28];
  if (s.cfg.planVolumeMl > 0) snprintf(vol, sizeof(vol), "%d/%d mL", (int)s.totalVolumeMl, (int)s.cfg.planVolumeMl);
  else                        snprintf(vol, sizeof(vol), "%d mL", (int)s.totalVolumeMl);
  String sum = String(vol) + "|" + String(tl);
  if (sum != cacheSummary) {
    cacheSummary = sum;
#if UI_LANDSCAPE
    tft.fillRect(FOCUS_X + 6, 160, FOCUS_W - 12, 40, C_CARD);            // [1.1.0-CYD] เพิ่ม: ปริมาตรบนหนึ่งบรรทัด เวลาที่เหลือใต้ลงมาตัวใหญ่
    textAt(String(vol), FOCUS_X + 8, 162, 1, C_DIM, C_CARD);
    textAt(String(tl), FOCUS_X + 8, 178, 3, bedIsAlarm(st) ? sc : C_WHITE, C_CARD);
#else
    tft.fillRect(FOCUS_X + 6, 120, FOCUS_W - 12, 10, C_CARD);
    textAt(String(vol), FOCUS_X + 8, 120, 1, C_DIM, C_CARD);
    textRight(String(tl), FOCUS_X + FOCUS_W - 8, 120, 1, bedIsAlarm(st) ? sc : C_WHITE, C_CARD);
#endif
  }
}

void drawHomeRow(int idx, int y, int rowH) {
  BedUiStatus st = bedUiStatus(idx);
  const StationData &s = stations[idx];
  bool dim = (st == BU_OFFLINE);
  uint16_t card = dim ? C_CARD2 : C_CARD;
  uint16_t sc = bedColor(st);

  tft.fillRoundRect(LIST_X, y, LIST_W, rowH, 5, card);
  tft.fillRect(LIST_X, y, 5, rowH, sc);

  char bed[8];
  snprintf(bed, sizeof(bed), "B%02d", idx + 1);
  String rateStr;
  if (st == BU_OFFLINE)     rateStr = "OFF";
  else if (st == BU_PAUSED) rateStr = "||";
  else                      rateStr = String((int)(s.flowRate_ml_hr + 0.5f));

  int ty = y + (rowH - 16) / 2;
  textAt(String(bed), LIST_X + 12, ty, 2, dim ? C_DIM : C_WHITE, card);
#if !UI_LANDSCAPE   // [1.1.0-CYD] แก้: แนวนอนรายการแคบ ไม่มีที่พอสำหรับชื่อผู้ป่วย (ดูชื่อได้ที่หน้ารายเตียง)
  String name = String(s.cfg.patientName);
  if (name.length() > 10) name = name.substring(0, 10);
  if (rowH >= 28 && name.length() > 0) textAt(name, LIST_X + 56, ty + 4, 1, C_DIM, card);
#endif
  textRight(rateStr, LIST_X + LIST_W - 40, ty, 2, bedIsAlarm(st) ? sc : (dim ? C_DIM : C_WHITE), card);
  textRight(">", LIST_X + LIST_W - 10, ty, 2, C_DIM, card);
}

void drawHomeList(int focus, bool force) {
  int n = activeStationCount - 1;
  if (n <= 0) return;
  int gap = 4;
  int rowH = (LIST_H - gap * (n - 1)) / n;
  if (rowH > 40) rowH = 40;
  if (rowH < 22) rowH = 22;

  if (force || rowH != homeRowH) {
    homeRowH = rowH;
    tft.fillRect(LIST_X, LIST_Y, LIST_W, LIST_H, C_BG);
    for (int i = 0; i < MAX_SUPPORTED_STATIONS; i++) cacheSide[i] = "";
  }

  int slot = 0;
  homeRowCount = 0;
  for (int i = 0; i < activeStationCount; i++) {
    if (i == focus) continue;
    int y = LIST_Y + slot * (rowH + gap);
    if (y + rowH > LIST_Y + LIST_H) break;
    homeRowBed[homeRowCount++] = i;
    String key = String((int)bedUiStatus(i)) + ":" + String((int)(stations[i].flowRate_ml_hr + 0.5f));
    if (force || key != cacheSide[i]) {
      cacheSide[i] = key;
      drawHomeRow(i, y, rowH);
    }
    slot++;
  }
}

void drawHomeBottomBar() {
  int alarm = -1, near = -1, online = 0;
  for (int i = 0; i < activeStationCount; i++) {
    if (alarm < 0 && bedIsAlarm(bedUiStatus(i))) alarm = i;
    if (near < 0 && stations[i].alertCode == ALERT_NEAR_END && !stations[i].nearEndAck) near = i;
    if (isStationOnline(i)) online++;
  }
  char buf[40];
  uint16_t bg, fg;
  if (alarm >= 0) {
    snprintf(buf, sizeof(buf), "! BED %02d %s", alarm + 1, alertTextEn(stations[alarm].alertCode));
    bg = C_RED; fg = C_WHITE;
  } else if (anyIdConflict()) {
    // เลขเตียงซ้ำทำให้เตียงที่เหลือไม่ถูกเฝ้าเลย จึงสำคัญกว่าการเตือนใกล้หมด
    int dup = -1;
    for (int i = 0; i < activeStationCount; i++) if (stations[i].idConflict) { dup = i; break; }
    snprintf(buf, sizeof(buf), "! BED %02d ID CLASH", dup + 1);
    bg = C_RED; fg = C_WHITE;
  } else if (heardBeyondBedCount() > 0) {
    snprintf(buf, sizeof(buf), "HEARD BED %02d NOT ON", heardBeyondBedCount());
    bg = C_ORANGE; fg = C_BG;
  } else if (near >= 0) {
    snprintf(buf, sizeof(buf), "BED %02d NEXT BAG", near + 1);
    bg = C_ORANGE; fg = C_BG;
  } else {
    snprintf(buf, sizeof(buf), "ALL NORMAL  %d/%d", online, activeStationCount);
    bg = C_CARD; fg = C_GREEN;
  }
  String line = String(buf);
  if (line == cacheBottom) return;
  cacheBottom = line;
  tft.fillRoundRect(ZONE_ALERT.x, ZONE_ALERT.y, ZONE_ALERT.w, ZONE_ALERT.h, 6, bg);
  textCenterIn(line, ZONE_ALERT.x, ZONE_ALERT.w, ZONE_ALERT.y + (ZONE_ALERT.h - 8) / 2, 1, fg, bg);   // [1.1.0-CYD] แก้: คำนวณจากความสูงปุ่ม (แนวตั้งยังได้ +10 เท่าเดิม)
}

void drawHomeScreen(bool force) {
  if (force) {
    tft.fillScreen(C_BG);
    drawStatusBar(true);
    drawButton(ZONE_GEAR, "SET", C_BTN, C_WHITE);
    cacheFocusKey = ""; cacheBottom = "";
    for (int i = 0; i < MAX_SUPPORTED_STATIONS; i++) cacheSide[i] = "";
  } else {
    drawStatusBar(false);
  }
  int focus = currentFocusBed();
  drawHomeFocusPanel(focus, force);
  drawHomeList(focus, force);
  drawHomeBottomBar();
}

void drawBedScreen(bool force) {
  int i = uiSelectedBed;
  BedUiStatus st = bedUiStatus(i);
  const StationData &s = stations[i];
  uint16_t sc = bedColor(st);

  if (force) {
    tft.fillScreen(C_BG);
    drawStatusBar(true);
    char title[16];
    snprintf(title, sizeof(title), "BED %02d", i + 1);
    drawTitleBar(String(title));
#if UI_LANDSCAPE
    tft.fillRoundRect(6, 62, 308, 96, 8, C_CARD);     // [1.1.0-CYD] เพิ่ม: แนวนอนกว้าง การ์ดบนกินเต็มความกว้าง
    tft.fillRoundRect(6, 164, 308, 46, 8, C_CARD);
    textAt("INFUSED", 14, 170, 1, C_DIM, C_CARD);
#else
    tft.fillRoundRect(6, 62, 228, 100, 8, C_CARD);
    tft.fillRoundRect(6, 168, 228, 64, 8, C_CARD);
    textAt("INFUSED", 14, 176, 1, C_DIM, C_CARD);
#endif
    drawButton(ZONE_BED_NEWBAG, "NEW BAG", C_BTN, C_WHITE);
    drawButton(ZONE_BED_ACK, "ACK", C_BTN, C_WHITE);
    drawButton(ZONE_BED_SET, "SET", C_BTN, C_WHITE);
#if !UI_LANDSCAPE
    textCenterIn("tap SET to change target / plan", 0, SCR_W, 300, 1, C_DIM, C_BG);
#endif
    cacheBedKey = "";
  } else {
    drawStatusBar(false);
  }

  int pct = bedPct(i);
  int mins = bedMinutesLeft(i);
  String key = String((int)st) + ":" + String((int)(s.flowRate_ml_hr + 0.5f)) + ":" +
               String((int)s.cfg.targetRateHr) + ":" + String(pct) + ":" + String(mins);
  // กระเปาะหยดต้องวาดทุกเฟรม ไม่งั้นจะไม่ขยับ จึงวาดก่อนการตรวจแคชของตัวเลข
#if UI_LANDSCAPE
  drawDripChamber(138, 90, 42, 46, i);
#else
  drawDripChamber(180, 86, 42, 46, i);
#endif

  if (key == cacheBedKey) return;
  cacheBedKey = key;

  String name = String(s.cfg.patientName);
  if (name.length() > 14) name = name.substring(0, 14);
  textAt(padTo(name, 14), 14, 70, 1, C_DIM, C_CARD);
#if UI_LANDSCAPE
  tft.fillRoundRect(222, 66, 86, 20, 5, sc);
  textCenterIn(String(bedWord(st)), 222, 86, 72, 1, C_BG, sc);
#else
  tft.fillRoundRect(146, 66, 82, 20, 5, sc);
  textCenterIn(String(bedWord(st)), 146, 82, 72, 1, C_BG, sc);
#endif

#if UI_LANDSCAPE
  tft.fillRect(12, 90, 120, 48, C_CARD);
#else
  tft.fillRect(12, 90, 140, 48, C_CARD);
#endif
  String rateStr = (st == BU_OFFLINE || st == BU_PAUSED) ? String("--")
                                                         : String((int)(s.flowRate_ml_hr + 0.5f));
  textAt(rateStr, 14, 90, 6, bedIsAlarm(st) ? sc : C_WHITE, C_CARD);
  textAt("mL/h", 14, 142, 1, C_DIM, C_CARD);
  textRight(padTo(s.cfg.targetRateHr > 0 ? ("SET " + String((int)s.cfg.targetRateHr) + " mL/h")
                                         : String("SET --"), 14),
#if UI_LANDSCAPE
            306, 142, 1, C_SKY, C_CARD);
#else
            226, 142, 1, C_SKY, C_CARD);
#endif

  // ---- บรรทัดวินิจฉัยการเชื่อมต่อ (ใหม่ใน 4.9.2) ----
  // คุณภาพลิงก์ต่างจากความแรงสัญญาณ: RSSI บอกว่าสัญญาณแรงแค่ไหน
  // ส่วนค่านี้บอกว่าข้อมูลหายจริงหรือเปล่า เตียงที่ RSSI ดีแต่ลิงก์ตก = แพ็กเก็ตชนกัน
  String diag;
  uint16_t diagCol = C_DIM;
  if (!isStationOnline(i)) {
    diag = "LINK --";
  } else if (s.idConflict) {
    diag = "BED ID CLASH " + macTail(s);     // มีสองเครื่องตั้งเลขเตียงเดียวกัน
    diagCol = C_RED;
  } else {
    diag = "LINK " + String(s.linkPct) + "%  " + macTail(s);
    diagCol = (s.linkPct >= 90) ? C_GREEN : (s.linkPct >= LINK_WEAK_PCT ? C_SKY : C_RED);
  }
#if UI_LANDSCAPE
  textAt(padTo(diag, 21), 186, 112, 1, diagCol, C_CARD);   // [1.1.0-CYD] แก้: ย้ายไปด้านขวาของตัวเลขอัตราไหล
#else
  textAt(padTo(diag, 24), 14, 152, 1, diagCol, C_CARD);
#endif

#if UI_LANDSCAPE
  textRight(padTo(pct >= 0 ? (String(pct) + "%") : String("--"), 5), 306, 170, 1, C_WHITE, C_CARD);
  drawProgressBar(14, 182, 292, 10, (pct < 0) ? 0 : pct, (st == BU_NEAREND) ? C_ORANGE : C_CYAN);
#else
  textRight(padTo(pct >= 0 ? (String(pct) + "%") : String("--"), 5), 226, 176, 1, C_WHITE, C_CARD);
  drawProgressBar(14, 190, 212, 12, (pct < 0) ? 0 : pct, (st == BU_NEAREND) ? C_ORANGE : C_CYAN);
#endif
  char sub[44];
  if (mins >= 0) snprintf(sub, sizeof(sub), "%d/%d mL   %dh %02dm left",
                          (int)s.totalVolumeMl, (int)s.cfg.planVolumeMl, mins / 60, mins % 60);
  else           snprintf(sub, sizeof(sub), "%d/%d mL   --h --m left",
                          (int)s.totalVolumeMl, (int)s.cfg.planVolumeMl);
#if UI_LANDSCAPE
  textAt(padTo(String(sub), 30), 14, 196, 1, C_DIM, C_CARD);
#else
  textAt(padTo(String(sub), 30), 14, 212, 1, C_DIM, C_CARD);
#endif
}

void drawSettingRow(int x, int y, const char* label, const String &value, const char* unit, bool force) {
#if UI_LANDSCAPE
  // [1.1.0-CYD] เพิ่ม: ช่องแนวนอนแคบ หน่วยจึงลงไปอยู่ใต้ค่า ไม่ชนปุ่ม - +
  if (force) {
    tft.fillRoundRect(x, y, SET_ROW_W, SET_ROW_H, 7, C_CARD);
    textAt(String(label), x + 8, y + 5, 1, C_DIM, C_CARD);
    tft.fillRoundRect(x + SET_M_X, y + SET_BTN_Y, SET_M_W, SET_BTN_H, 6, C_BTN);
    textCenterIn("-", x + SET_M_X, SET_M_W, y + SET_BTN_Y + 7, 2, C_WHITE, C_BTN);
    tft.fillRoundRect(x + SET_P_X, y + SET_BTN_Y, SET_P_W, SET_BTN_H, 6, C_BTN);
    textCenterIn("+", x + SET_P_X, SET_P_W, y + SET_BTN_Y + 7, 2, C_WHITE, C_BTN);
    textAt(String(unit), x + 8, y + 38, 1, C_DIM, C_CARD);
  }
  tft.fillRect(x + 6, y + 16, SET_M_X - 10, 20, C_CARD);
  textAt(value, x + 8, y + 18, 2, C_WHITE, C_CARD);
#else
  if (force) {
    tft.fillRoundRect(x, y, 228, 44, 7, C_CARD);
    textAt(String(label), x + 8, y + 5, 1, C_DIM, C_CARD);
    tft.fillRoundRect(x + 134, y + 4, 44, 36, 6, C_BTN);
    textCenterIn("-", x + 134, 44, y + 14, 2, C_WHITE, C_BTN);
    tft.fillRoundRect(x + 182, y + 4, 42, 36, 6, C_BTN);
    textCenterIn("+", x + 182, 42, y + 14, 2, C_WHITE, C_BTN);
  }
  tft.fillRect(x + 6, y + 18, 124, 20, C_CARD);
  textAt(value, x + 8, y + 20, 2, C_WHITE, C_CARD);
  textAt(String(unit), x + 8 + (int)value.length() * 12 + 6, y + 26, 1, C_DIM, C_CARD);
#endif
}

void drawBedSetScreen(bool force) {
  if (force) {
    tft.fillScreen(C_BG);
    drawStatusBar(true);
    char title[16];
    snprintf(title, sizeof(title), "SET B%02d", uiSelectedBed + 1);
    drawTitleBar(String(title));
    drawButton(ZONE_SET_SAVE, "SAVE", C_GREEN, C_BG, 2);
    drawButton(ZONE_SET_CANCEL, "CANCEL", C_BTN, C_WHITE, 2);
  } else {
    drawStatusBar(false);
  }
  drawSettingRow(SET_ROW_X[0], SET_ROW_Y[0], "TARGET RATE", String((int)editTargetRate),  "mL/h",   force);
  drawSettingRow(SET_ROW_X[1], SET_ROW_Y[1], "PLAN VOLUME", String((int)editPlanVolume),  "mL",     force);
  drawSettingRow(SET_ROW_X[2], SET_ROW_Y[2], "DROP FACTOR", String((int)editDropFactor),  "gtt/mL", force);
  drawSettingRow(SET_ROW_X[3], SET_ROW_Y[3], "NEXT BAG AT", String((int)editNearPct),     "%",      force);
}

// ============================================================================
// หน้า NUMPAD — แป้นตัวเลขบนจอ
// ============================================================================
void drawNumpadScreen(bool force) {
  if (force) {
    tft.fillScreen(C_BG);
    drawStatusBar(true);
    drawTitleBar("ENTER");
#if UI_LANDSCAPE
    tft.fillRoundRect(6, 62, 102, 72, 7, C_CARD);      // [1.1.0-CYD] เพิ่ม: แนวนอน ช่องค่าอยู่ซ้าย แป้นอยู่ขวา
    textAt(String(editNumLabel), 14, 68, 1, C_DIM, C_CARD);
    textAt(String(editNumUnit), 14, 120, 1, C_DIM, C_CARD);
#else
    tft.fillRoundRect(6, 62, 228, 46, 7, C_CARD);
    textAt(String(editNumLabel), 14, 68, 1, C_DIM, C_CARD);
    textRight(String(editNumUnit), 226, 88, 1, C_DIM, C_CARD);
#endif

    const char* keys[12] = { "1","2","3","4","5","6","7","8","9","C","0","OK" };
    for (int i = 0; i < 12; i++) {
      TouchZone z = numpadKeyZone(i);
      bool ok = (i == 11), clr = (i == 9);
      drawButton(z, String(keys[i]), ok ? C_GREEN : (clr ? C_ORANGE : C_BTN),
                 (ok || clr) ? C_BG : C_WHITE, 2);
    }
    cacheNumValue = "";
  } else {
    drawStatusBar(false);
  }
  String v = (editNumBuf[0] == '\0') ? String("_") : String(editNumBuf);
  if (v != cacheNumValue) {
    cacheNumValue = v;
#if UI_LANDSCAPE
    tft.fillRect(12, 88, 92, 26, C_CARD);
    textAt(v, 14, 90, 3, C_WHITE, C_CARD);
#else
    tft.fillRect(12, 80, 120, 24, C_CARD);
    textAt(v, 14, 82, 3, C_WHITE, C_CARD);
#endif
  }
}

void drawSysScreen(bool force) {
  if (force) {
    tft.fillScreen(C_BG);
    drawStatusBar(true);
    drawTitleBar("SETTINGS");
    drawButton(ZONE_SYS_CALIB, "TOUCH CALIBRATION", C_BTN, C_WHITE);

#if UI_LANDSCAPE
    // [1.1.0-CYD] เพิ่ม: แนวนอนแบ่งการ์ดข้อมูลเป็นสองคอลัมน์ ซ้ายเครือข่าย ขวานาฬิกากับ SD
    tft.fillRoundRect(6, 154, 308, 82, 7, C_CARD);
    textAt("WI-FI / WEB", 14, 160, 1, C_DIM, C_CARD);
    textAt("CLOCK / SD", 164, 160, 1, C_DIM, C_CARD);
    textRight("v" APP_VERSION, 306, 160, 1, C_DIM, C_CARD);
    textAt(String(default_ap_ssid), 14, 178, 1, C_WHITE, C_CARD);
    textAt("pass " + String(default_ap_pass), 14, 194, 1, C_DIM, C_CARD);
    textAt("http://" + WiFi.softAPIP().toString(), 14, 210, 1, C_SKY, C_CARD);
    tft.drawFastVLine(156, 160, 70, C_DIM);
#else
    tft.fillRoundRect(6, 204, 228, 112, 7, C_CARD);
    textAt("WI-FI / WEB", 14, 210, 1, C_DIM, C_CARD);
    textRight("v" APP_VERSION, 226, 210, 1, C_DIM, C_CARD);
    textAt(String(default_ap_ssid), 14, 224, 1, C_WHITE, C_CARD);
    textAt("pass " + String(default_ap_pass), 14, 238, 1, C_DIM, C_CARD);
    textAt("http://" + WiFi.softAPIP().toString(), 14, 252, 1, C_SKY, C_CARD);
    tft.drawFastHLine(14, 267, 212, C_DIM);
    textAt("CLOCK / SD CARD", 14, 274, 1, C_DIM, C_CARD);
#endif
    cacheSummary = "";
  } else {
    drawStatusBar(false);
  }

  drawSettingRow(SYS_CELL_X[0], SYS_CELL_Y[0], "ACTIVE BEDS",       String(activeStationCount), "beds", force);
  drawSettingRow(SYS_CELL_X[1], SYS_CELL_Y[1], UI_LANDSCAPE ? "BRIGHTNESS" : "SCREEN BRIGHTNESS",
                 String(screenBrightness), "%", force);

  // แถวสถานะแบบเปลี่ยนแปลงได้: จำนวนอุปกรณ์ที่ต่ออยู่ + สถานะนาฬิกา DS3231 + SD card
  String devTxt = String((int)WiFi.softAPgetStationNum()) + " dev";
  String rtcTxt = rtcStatusText();
  String sdTxt  = sdStatusText();
  String key = devTxt + "|" + rtcTxt + "|" + sdTxt;
  if (key != cacheSummary) {
    cacheSummary = key;
#if UI_LANDSCAPE
    textAt(padTo("RTC " + rtcTxt, 24), 164, 178, 1,
           (rtcPresent && rtcTimeOk) ? C_GREEN : C_YELLOW, C_CARD);
    textAt(padTo("SD  " + sdTxt, 24), 164, 194, 1,
           sdPresent ? C_GREEN : C_YELLOW, C_CARD);
    textAt(padTo(devTxt, 24), 164, 210, 1, C_DIM, C_CARD);
#else
    textRight(padTo(devTxt, 8), 226, 224, 1, C_DIM, C_CARD);
    textAt(padTo("RTC " + rtcTxt, 24), 14, 288, 1,
           (rtcPresent && rtcTimeOk) ? C_GREEN : C_YELLOW, C_CARD);
    textAt(padTo("SD  " + sdTxt, 24), 14, 302, 1,
           sdPresent ? C_GREEN : C_YELLOW, C_CARD);
#endif
  }
}

// ============================================================================
// หน้า ALARM — เตือนเต็มจอ แตะที่ไหนก็ได้เพื่อพักเสียง 2 นาที
// ============================================================================
void drawAlarmScreen(bool force) {
  int bed = autoFocusBed();
  if (bed < 0) bed = 0;
  const StationData &s = stations[bed];
  uint8_t code = s.alertCode;

  if (force) {
    tft.fillScreen(C_RED);
#if UI_LANDSCAPE
    tft.fillRoundRect(10, 10, 300, 42, 8, C_WHITE);   // [1.1.0-CYD] เพิ่ม: แนวนอนจัดผังใหม่ให้พอดีความสูง 240
    char head[16];
    snprintf(head, sizeof(head), "BED %02d", bed + 1);
    textCenterIn(String(head), 10, 300, 21, 3, C_RED, C_WHITE);
#else
    tft.fillRoundRect(10, 14, 220, 44, 8, C_WHITE);
    char head[16];
    snprintf(head, sizeof(head), "BED %02d", bed + 1);
    textCenterIn(String(head), 10, 220, 28, 3, C_RED, C_WHITE);
#endif

    const char* sub = "CHECK IV SYSTEM";
    if (code == ALERT_OCCLUSION)     sub = "LINE BLOCKED - CHECK TUBING";
    else if (code == ALERT_TOO_FAST) sub = "FLOW ABOVE TARGET";
    else if (code == ALERT_TOO_SLOW) sub = "FLOW BELOW TARGET";
    else if (code == ALERT_COMPLETE) sub = "INFUSION FINISHED";
#if UI_LANDSCAPE
    textCenterIn(String(alertTextEn(code)), 0, SCR_W, 58, 3, C_WHITE, C_RED);
    textCenterIn(String(sub), 0, SCR_W, 90, 1, C_YELLOW, C_RED);

    tft.fillRoundRect(16, 104, 288, 70, 8, C_BG);
    textAt("RATE", 28, 110, 1, C_DIM, C_BG);
    textAt("mL/h", 28, 160, 1, C_DIM, C_BG);
    textRight(s.cfg.targetRateHr > 0 ? ("SET " + String((int)s.cfg.targetRateHr) + " mL/h") : String("SET --"),
              292, 160, 1, C_SKY, C_BG);

    tft.fillRoundRect(16, 182, 288, 48, 10, C_WHITE);
    textCenterIn("TAP = SNOOZE 2 MIN", 16, 288, 194, 1, C_RED, C_WHITE);
    textCenterIn("BUTTON = SILENCE", 16, 288, 210, 1, C_RED, C_WHITE);
#else
    textCenterIn(String(alertTextEn(code)), 0, SCR_W, 74, 3, C_WHITE, C_RED);
    textCenterIn(String(sub), 0, SCR_W, 112, 1, C_YELLOW, C_RED);

    tft.fillRoundRect(16, 136, 208, 80, 8, C_BG);
    textAt("RATE", 28, 144, 1, C_DIM, C_BG);
    textAt("mL/h", 28, 196, 1, C_DIM, C_BG);
    textRight(s.cfg.targetRateHr > 0 ? ("SET " + String((int)s.cfg.targetRateHr) + " mL/h") : String("SET --"),
              212, 196, 1, C_SKY, C_BG);

    tft.fillRoundRect(16, 232, 208, 72, 10, C_WHITE);
    textCenterIn("TAP = SNOOZE 2 MIN", 16, 208, 252, 1, C_RED, C_WHITE);
    textCenterIn("BUTTON = SILENCE", 16, 208, 274, 1, C_RED, C_WHITE);
#endif
    cacheAlarmRate = "";
  }

  String rate = String((int)(s.flowRate_ml_hr + 0.5f));
  if (rate != cacheAlarmRate) {
    cacheAlarmRate = rate;
#if UI_LANDSCAPE
    tft.fillRect(26, 122, 150, 34, C_BG);
    textAt(rate, 28, 122, 4, C_WHITE, C_BG);
#else
    tft.fillRect(26, 158, 150, 34, C_BG);
    textAt(rate, 28, 158, 4, C_WHITE, C_BG);
#endif
  }
}

void drawCalibCross(int x, int y, uint16_t col) {
  tft.drawFastHLine(x - 12, y, 25, col);
  tft.drawFastVLine(x, y - 12, 25, col);
  tft.drawCircle(x, y, 8, col);
}

void drawCalibScreen(bool force) {
  if (!force) return;
  tft.fillScreen(C_BG);
#if UI_LANDSCAPE
  const int calY = 90;      // [1.1.0-CYD] เพิ่ม: แนวนอนขยับข้อความขึ้นให้อยู่กลางจอ (ห่างจากกากบาทมุมจอ)
#else
  const int calY = 120;
#endif
  textCenterIn("TOUCH CALIBRATION", 0, SCR_W, calY, 2, C_WHITE, C_BG);
  textCenterIn(calibStep == 0 ? "tap the cross (1 of 2)" : "tap the cross (2 of 2)",
               0, SCR_W, calY + 30, 1, C_DIM, C_BG);
  textCenterIn("use a stylus for best accuracy", 0, SCR_W, calY + 50, 1, C_DIM, C_BG);
  drawCalibCross(CALIB_PT[calibStep][0], CALIB_PT[calibStep][1], C_YELLOW);
}

// ---- หน้าจอเฉพาะกิจ: ปิดเครื่อง / หน้าจอต้อนรับ ----
void drawPowerOffProgress(int pct) {
  if (uiScreenDrawn != SCR_POWER) {
    uiScreenDrawn = SCR_POWER;
    tft.fillScreen(C_BG);
#if UI_LANDSCAPE
    const int poY = 60;     // [1.1.0-CYD] เพิ่ม: แนวนอนขยับขึ้น 50 พิกเซลให้อยู่กลางจอ
#else
    const int poY = 110;
#endif
    textCenterIn("POWER OFF ?", 0, SCR_W, poY, 3, C_WHITE, C_BG);
    textCenterIn("KEEP HOLDING TO TURN OFF", 0, SCR_W, poY + 40, 1, C_DIM, C_BG);
    tft.drawRoundRect((SCR_W - 204) / 2, poY + 68, 204, 24, 6, C_WHITE);
  }
#if UI_LANDSCAPE
  const int poBarY = 60 + 70;
#else
  const int poBarY = 110 + 70;
#endif
  const int poBarX = (SCR_W - 204) / 2 + 2;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  int w = (200 * pct) / 100;
  tft.fillRect(poBarX, poBarY, w, 20, C_RED);
  tft.fillRect(poBarX + w, poBarY, 200 - w, 20, C_BG);
}

void drawGoodbyeScreen() {
  uiScreenDrawn = SCR_POWER;
  tft.fillScreen(C_BG);
#if UI_LANDSCAPE
  const int gbY = 70;       // [1.1.0-CYD] เพิ่ม: แนวนอนขยับข้อความขึ้น
#else
  const int gbY = 120;
#endif
  textCenterIn("GOODBYE", 0, SCR_W, gbY, 4, C_SKY, C_BG);
  textCenterIn("RELEASE BUTTON TO TURN OFF", 0, SCR_W, gbY + 50, 1, C_DIM, C_BG);
  textCenterIn("HOLD 2s TO POWER ON AGAIN", 0, SCR_W, gbY + 70, 1, C_DIM, C_BG);
}

void setDisplaySleep(bool sleep) {
  displaySleeping = sleep;
  if (sleep) {
    setBacklightRaw(0);
    tft.fillScreen(C_BG);
  } else {
    setBacklightRaw(screenBrightness);
    uiScreenDrawn = (UiScreen)-1;
    uiNeedFramework = true;
    resetUiCaches();
  }
}

void drawSplashScreen() {
  tft.fillScreen(C_BG);
#if UI_LANDSCAPE
  // [1.1.0-CYD] เพิ่ม: แนวนอนเลื่อนทั้งชุดขึ้นและกางข้อความท้ายให้พอดีความสูง 240
  tft.fillRoundRect(60, 20, 200, 96, 10, C_TOPBAR);
  textCenterIn("SMART IV", 0, SCR_W, 38, 4, C_WHITE, C_TOPBAR);
  textCenterIn("CENTRAL HOST", 0, SCR_W, 80, 2, C_SKY, C_TOPBAR);
  textCenterIn("Host v" APP_VERSION, 0, SCR_W, 132, 1, C_DIM, C_BG);
  char buf[40];
  snprintf(buf, sizeof(buf), "BEDS %d   TOUCH SCREEN", activeStationCount);
  textCenterIn(String(buf), 0, SCR_W, 150, 1, C_DIM, C_BG);
  textCenterIn(String(default_ap_ssid), 0, SCR_W, 176, 1, C_WHITE, C_BG);
  textCenterIn("BCN Phrae Innovation", 0, SCR_W, 204, 1, C_GREEN, C_BG);
#else
  tft.fillRoundRect(20, 70, 200, 96, 10, C_TOPBAR);
  textCenterIn("SMART IV", 0, SCR_W, 88, 4, C_WHITE, C_TOPBAR);
  textCenterIn("CENTRAL HOST", 0, SCR_W, 130, 2, C_SKY, C_TOPBAR);
  textCenterIn("Host v" APP_VERSION, 0, SCR_W, 188, 1, C_DIM, C_BG);
  char buf[40];
  snprintf(buf, sizeof(buf), "BEDS %d   TOUCH SCREEN", activeStationCount);
  textCenterIn(String(buf), 0, SCR_W, 208, 1, C_DIM, C_BG);
  textCenterIn(String(default_ap_ssid), 0, SCR_W, 232, 1, C_WHITE, C_BG);
  textCenterIn("BCN Phrae Innovation", 0, SCR_W, 258, 1, C_GREEN, C_BG);
#endif
}
