/**
 * @file      HostScreen.h
 * @brief     ทุกอย่างที่วาดลงจอ OLED 128x64 ของเครื่องส่วนกลาง
 * @version   4.9.0-OLED
 * @date      2026-09-23
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 * อาจารย์ที่ปรึกษา ดร.กรรณิการ์ กาศสมบูรณ์
 *
 * @par Description
 * แยกออกมาจาก `Host-OLED.ino` เพื่อให้ไฟล์หลักสั้นและหาของเจอเร็วขึ้น
 * มีหน้ารวมทุกเตียงแบบบล็อก หน้ารายเตียง หน้า LINK DIAGNOSTIC และหน้าพักจอ
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 4.9.0-OLED | 2026-09-28 | ออกแบบหน้าจอใหม่ตาม Station ทั้งหัวจอ การ์ด ป้ายเตียงแบบทึบ แถบบอกหน้า และหน้ารายเตียงแบบ IV BAG |
 * | 4.7.7 | 2026-09-23 | แยกออกมาจากไฟล์หลัก ย้ายที่อยู่ล้วน ๆ ไม่ได้แก้เนื้อในแม้แต่บรรทัดเดียว |
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
 *  @brief     ทุกอย่างที่วาดลงจอ OLED ของเครื่องส่วนกลาง
 *  @version   4.7.7-OLED
 *
 *  แยกออกมาจาก Host-OLED.ino เพื่อให้ไฟล์หลักสั้นและอ่านง่าย
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

// กระเปาะขนาดจิ๋ว กว้าง 6 px สำหรับการ์ดเตียงในหน้ารวม (h ตั้งแต่ 6 px ขึ้นไป)
void drawDripMini(int x, int y, int h, int i) {
  if (h < 6) h = 6;
  u8g2.drawFrame(x, y, 6, h);               // ตัวกระเปาะ
  u8g2.drawBox(x + 2, y + 1, 2, 1);         // หัวหยด

  switch (dripModeOf(i)) {
    case DRIP_OFFLINE:
      u8g2.drawLine(x + 1, y + 2, x + 4, y + h - 3);
      u8g2.drawLine(x + 4, y + 2, x + 1, y + h - 3);
      return;
    case DRIP_PAUSED:
      u8g2.drawVLine(x + 1, y + 2, h - 4);
      u8g2.drawVLine(x + 4, y + 2, h - 4);
      return;
    case DRIP_ALERT:
      u8g2.drawVLine(x + 2, y + 2, h - 5);
      u8g2.drawBox(x + 2, y + h - 2, 2, 1);
      return;
    default:
      break;
  }

  int poolY = y + h - 3;
  u8g2.drawBox(x + 1, poolY, 4, 2);         // ผิวน้ำในกระเปาะ

  int pct = dripAnimPhase(i);
  if (pct == -2) {                          // กระเพื่อมตอนหยดถึงผิวน้ำ
    u8g2.drawHLine(x + 1, poolY - 1, 4);
  } else if (pct >= 0) {
    int top = y + 2;
    int bot = poolY - 2;
    if (bot < top) bot = top;
    int dy  = top + ((bot - top) * pct) / 100;
    u8g2.drawBox(x + 2, dy, 2, 2);          // หยดที่กำลังตก
  }
}

// ============================================================================
// ชิ้นส่วนหน้าจอที่ใช้ภาษาการออกแบบเดียวกับ Station ([4.9.0-OLED] เพิ่มทั้งหมด)
//
// Station (จอ TFT สี 172x320) มีองค์ประกอบประจำอยู่สี่อย่าง
//   1) แถบบนสุด: ป้ายเตียงแบบทึบ | นาฬิกากลางจอ | แท่งสัญญาณ แล้วมีแถบสีขีดใต้
//   2) การ์ดขอบมน ป้ายเล็กตัวพิมพ์ใหญ่อยู่มุมบนซ้าย ตัวเลขใหญ่อยู่ใต้ป้าย
//   3) ป้ายสถานะแบบเม็ดยา (NORMAL) และถุงน้ำเกลือที่ระดับน้ำลดลงจริง
//   4) จุดบอกหน้าที่ขอบล่าง
// จอนี้ขาวดำ 128x64 จึงแปลงดังนี้
//   - สีของป้ายเตียงบอกสถานะ -> ป้ายกะพริบทึบ/โปร่งเมื่อมีเหตุวิกฤตที่ยังไม่พักเสียง
//   - จุดบอกหน้า -> ใช้แถบใต้หัวจอแทน ช่วงของหน้าปัจจุบันหนา 3 พิกเซล
//     เพราะจอสูงแค่ 64 พิกเซล ไม่มีแถวเหลือให้วางจุดที่ขอบล่าง
//   - สีของการ์ดเตือน -> การ์ดทึบทั้งใบ (เหมือนเดิม)
// ============================================================================

// แถบบนสุด: ป้าย (ALL / B03 / NET) | นาฬิกา | สัญญาณเราเตอร์ หรือ AP
void drawTopBar(const char *chip) {
  bool alarm  = globalAlarmTriggered && !isSnoozed();
  bool chipOn = !alarm || ((millis() / 400) % 2 == 0);
  u8g2.setFont(u8g2_font_5x8_tf);

  int cw = (int)strlen(chip) * 5 + 6;
  if (chipOn) {
    u8g2.drawRBox(0, 0, cw, 9, 2);
    u8g2.setDrawColor(0);
    u8g2.drawStr(3, 7, chip);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawRFrame(0, 0, cw, 9, 2);
    u8g2.drawStr(3, 7, chip);
  }

  String clk = getOledClockStr();
  u8g2.drawStr((128 - (int)clk.length() * 5) / 2, 7, clk.c_str());

  // ขวาสุด: เกาะเราเตอร์ = แท่งสัญญาณสี่ขีดแบบเดียวกับ Station / เปิด AP เอง = คำว่า AP
  if (netMode == NET_MODE_STA) {
    int bars = 0;
    if (lastStaRssi != 0) {
      if (lastStaRssi >= -60)      bars = 4;
      else if (lastStaRssi >= -70) bars = 3;
      else if (lastStaRssi >= -80) bars = 2;
      else                         bars = 1;
    }
    for (int b = 0; b < 4; b++) {
      int h = 2 + b * 2;
      int bx = 115 + b * 4;
      if (b < bars) u8g2.drawBox(bx, 8 - h, 3, h);
      else          u8g2.drawHLine(bx, 7, 3);   // ขีดที่ยังว่างเหลือแค่ฐาน
    }
  } else {
    u8g2.drawStr(118, 7, "AP");
  }

  // แถบใต้หัวจอ = จุดบอกหน้า: หน้าปัจจุบันหนาขึ้น
  int pages = activeStationCount + 2;               // หน้ารวม + รายเตียง + LINK DIAG
  int seg   = 128 / pages;
  int sx    = currentOledPage * seg;
  int sw    = (currentOledPage >= pages - 1) ? (128 - sx) : seg;
  u8g2.drawHLine(0, 10, 128);
  u8g2.drawBox(sx, 9, sw, 3);
}

// ป้ายเม็ดยาแบบ Station — เหตุผิดปกติ = ทึบ ตัวอักษรกลับสี / ปกติ = โปร่ง
void drawPill(int x, int y, int w, int h, const char *text, bool filled) {
  u8g2.setFont(u8g2_font_5x8_tf);
  int tx = x + (w - (int)strlen(text) * 5) / 2;
  int ty = y + h - 2;
  if (filled) {
    u8g2.drawRBox(x, y, w, h, 3);
    u8g2.setDrawColor(0);
    u8g2.drawStr(tx, ty, text);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawRFrame(x, y, w, h, 3);
    u8g2.drawStr(tx, ty, text);
  }
}

// คำในป้ายเม็ดยา — ถ้อยคำเดียวกับป้ายสถานะของ Station (NORMAL ฯลฯ) ไม่เกิน 7 ตัวอักษร
const char* pillWordOf(int i) {
  const StationData &s = stations[i];
  if (!isStationOnline(i))  return (s.alertCode == ALERT_LOST_LINK) ? "LOST" : "OFFLINE";
  if (s.idConflict)         return "DUP ID";
  if (!s.isRunning)         return "PAUSED";
  switch (s.alertCode) {
    case ALERT_TOO_FAST:  return "FAST";
    case ALERT_TOO_SLOW:  return "SLOW";
    case ALERT_OCCLUSION: return "NO FLOW";
    case ALERT_NO_SIGNAL: return "NO DRIP";
    case ALERT_COMPLETE:  return "DONE";
    case ALERT_NEAR_END:  return s.nearEndAck ? "NEAR" : "NEW BAG";
    default:              return "NORMAL";
  }
}

// ป้ายเล็กของการ์ด (RATE, LEFT ...) ตามแบบ Station ใช้ฟอนต์ 4x6
void drawCardLabel(int x, int y, const char *s) {
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(x, y, s);
}

// ถุงน้ำเกลือที่ระดับน้ำ = ปริมาตรที่เหลือตามแผน แบบเดียวกับหน้า IV BAG ของ Station
void drawIvBag(int x, int y, int w, int h, int i) {
  const StationData &s = stations[i];
  u8g2.drawBox(x + w / 2 - 2, y - 2, 4, 2);          // หูแขวน
  u8g2.drawRFrame(x, y, w, h, 3);
  for (int k = 1; k <= 3; k++) {                      // ขีดบอกระดับ 25/50/75%
    int ty = y + 2 + (h - 4) * k / 4;
    u8g2.drawHLine(x + w - 4, ty, 2);
  }
  if (s.cfg.planVolumeMl <= 0) {                      // ยังไม่มีแผน = ไม่รู้ว่าเหลือเท่าไร
    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.drawStr(x + w / 2 - 2, y + h / 2 + 3, "?");
    return;
  }
  float left = 1.0f - s.totalVolumeMl / s.cfg.planVolumeMl;
  if (left < 0.0f) left = 0.0f;
  if (left > 1.0f) left = 1.0f;
  int fh = (int)((h - 4) * left + 0.5f);
  if (fh > 0) u8g2.drawBox(x + 2, y + h - 2 - fh, w - 5, fh);
}

// เวลาที่เหลือของถุงนี้ เช่น 3h52m — ไม่มีแผนหรือไม่ไหล = "--"
String timeLeftStr(int i) {
  const StationData &s = stations[i];
  float rate = s.flowRate_ml_hr;
  float left = s.cfg.planVolumeMl - s.totalVolumeMl;
  if (s.cfg.planVolumeMl <= 0 || rate <= 0.5f || !isStationOnline(i) || !s.isRunning) return "--";
  if (left <= 0) return "0m";
  long mins = (long)(left / rate * 60.0f + 0.5f);
  char b[12];
  if (mins >= 60) snprintf(b, sizeof(b), "%ldh%02ldm", mins / 60, mins % 60);
  else            snprintf(b, sizeof(b), "%ldm", mins);
  return String(b);
}

// ----------------------------------------------------------------------------
// หน้ารายเตียง — ผังเดียวกับหน้า IV BAG ของ Station ([4.9.0-OLED] เพิ่ม)
//   ซ้าย  ถุงน้ำเกลือ + กระเปาะหยด (จังหวะหยดล็อกกับหยดจริงที่เตียง)
//   ขวา   RATE ตัวใหญ่ | ป้ายสถานะเม็ดยา | mL/h และ SET
//   ล่าง  LEFT ปริมาตรที่เหลือ และเวลาที่เหลือ
// ----------------------------------------------------------------------------
void renderBedDetail(int idx) {
  const StationData &s = stations[idx];
  bool isOnline = isStationOnline(idx);
  bool flowing  = isOnline && s.isRunning;

  drawIvBag(1, 14, 18, 28, idx);
  u8g2.drawVLine(9, 42, 3);                           // สายจากถุงลงกระเปาะ
  drawDripMini(7, 45, 12, idx);
  u8g2.drawVLine(9, 57, 5);                           // สายต่อลงไปหาผู้ป่วย

  drawCardLabel(24, 18, "RATE");
  drawPill(86, 12, 42, 10, pillWordOf(idx), bedNeedsAttention(idx) ||
           (s.alertCode == ALERT_NEAR_END && !s.nearEndAck));

  char b[16];
  if (flowing) {
    snprintf(b, sizeof(b), "%.0f", s.flowRate_ml_hr);
    u8g2.setFont(s.flowRate_ml_hr >= 999.5f ? u8g2_font_logisoso16_tf : u8g2_font_logisoso32_tf);
    u8g2.drawStr(24, 51, b);            // เว้นสี่แถวเหนือบรรทัด LEFT ไม่ให้ตัวเลขชนกัน
  } else {
    u8g2.setFont(u8g2_font_logisoso16_tf);
    u8g2.drawStr(24, 42, isOnline ? "PAUSED" : "OFFLINE");
  }

  drawCardLabel(92, 30, "mL/h");
  drawCardLabel(92, 39, "SET");
  snprintf(b, sizeof(b), "%.0f", s.cfg.targetRateHr);
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(92, 49, s.cfg.targetRateHr > 0 ? b : "--");

  // แถวล่าง: ปริมาตรที่เหลือ | เวลาที่เหลือ (ตามการ์ด LEFT / TIME LEFT ของ Station)
  if (s.cfg.planVolumeMl > 0) {
    float left = s.cfg.planVolumeMl - s.totalVolumeMl;
    if (left < 0) left = 0;
    drawCardLabel(24, 62, "LEFT");
    snprintf(b, sizeof(b), "%.0fmL", left);
    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.drawStr(42, 63, b);
    String tl = timeLeftStr(idx);
    u8g2.drawStr(128 - (int)tl.length() * 5, 63, tl.c_str());
  } else {
    u8g2.setFont(u8g2_font_5x8_tf);
    snprintf(b, sizeof(b), "%.0fmL  NO PLAN", s.totalVolumeMl);
    u8g2.drawStr(24, 63, b);
  }
}

// ----------------------------------------------------------------------------
// หน้ารวมทุกเตียง — เน้นอ่านจากระยะไกล
// ----------------------------------------------------------------------------
// v4.7.4 ตัดสิ่งที่มองไม่เห็นจริงบนจอ 128x64 ออก: ไม่มีกระเปาะหยด ไม่มีปริมาตรสะสม
// เหลือสามอย่างที่ตัดสินใจได้ทันที — เลขเตียง / อัตราไหล / คำสถานะ
// [4.9.0-OLED] แก้: การ์ดใช้ภาษาเดียวกับ Station — ป้ายเลขเตียงแบบทึบ ป้ายสถานะแบบเม็ดยา
void renderBlockStyleGrid() {
  int topY = 12;                        // [4.9.0-OLED] แก้: หัวจอใหม่สูงลงมาถึงแถว 11
  int bottomY = 53;
  int totalH = bottomY - topY;

  int onlineCount = 0;
  int alarmBed = 0;
  uint8_t alarmCode = ALERT_NONE;

  for (int i = 0; i < activeStationCount; i++) {
    if (isStationOnline(i)) onlineCount++;
    if (alarmBed == 0 && isCriticalAlert(stations[i].alertCode)) {
      alarmBed = i + 1;
      alarmCode = stations[i].alertCode;
    }
  }

  if (activeStationCount == 1) {
    // เตียงเดียว: การ์ดใหญ่ใบเดียว ตัวเลขใหญ่ซ้าย ป้ายเล็กและเม็ดยาขวา
    u8g2.drawRFrame(0, topY, 128, totalH, 3);
    bool flowing = isStationOnline(0) && stations[0].isRunning;
    char b[16];
    if (flowing) snprintf(b, sizeof(b), "%.0f", stations[0].flowRate_ml_hr);
    else         snprintf(b, sizeof(b), "--");
    u8g2.setFont(u8g2_font_logisoso32_tf);
    u8g2.drawStr(6, topY + 36, b);

    drawCardLabel(80, topY + 8, "BED 1  RATE");
    drawCardLabel(80, topY + 17, "mL/h");
    snprintf(b, sizeof(b), "SET %.0f", stations[0].cfg.targetRateHr);
    drawCardLabel(80, topY + 25, b);
    drawPill(78, topY + 28, 46, 10, pillWordOf(0), bedNeedsAttention(0));
  }
  else if (activeStationCount == 2) {
    int cardW = 62;
    for (int i = 0; i < 2; i++) {
      int x = (i == 0) ? 0 : 66;
      bool attn = bedNeedsAttention(i);
      u8g2.drawRFrame(x, topY, cardW, totalH, 3);

      // ป้ายเตียงแบบทึบ แบบเดียวกับ B01 บนหัวจอของ Station
      char idStr[8];
      snprintf(idStr, sizeof(idStr), "B%02d", i + 1);
      u8g2.setFont(u8g2_font_5x8_tf);
      u8g2.drawRBox(x + 3, topY + 3, 21, 9, 2);
      u8g2.setDrawColor(0);
      u8g2.drawStr(x + 6, topY + 10, idStr);
      u8g2.setDrawColor(1);
      drawCardLabel(x + 28, topY + 10, "RATE");

      char rBuf[16];
      if (isStationOnline(i) && stations[i].isRunning)
        snprintf(rBuf, sizeof(rBuf), "%.0f", stations[i].flowRate_ml_hr);
      else
        snprintf(rBuf, sizeof(rBuf), "--");
      u8g2.setFont(u8g2_font_logisoso16_tf);
      u8g2.drawStr(x + 4, topY + 28, rBuf);
      drawCardLabel(x + 40, topY + 28, "mL/h");

      drawPill(x + 3, topY + 30, cardW - 6, 9, pillWordOf(i), attn);
    }
  }
  else {
    int cardW = 62;
    int rows = (activeStationCount + 1) / 2;
    int rowH = (rows <= 3) ? 14 : 10;   // 3-6 เตียง = 3 แถว, 7-8 เตียง = 4 แถว
    bool bigFont = (rowH == 14);

    for (int i = 0; i < activeStationCount; i++) {
      int col = i % 2;
      int row = i / 2;
      int x = (col == 0) ? 0 : 65;
      int y = topY + row * rowH;
      int h = rowH - 1;
      int textY = y + (bigFont ? 10 : 7);

      bool attn = bedNeedsAttention(i);

      // เตียงที่ต้องไปดู = การ์ดทึบทั้งใบ มองเห็นได้จากท้ายห้อง
      if (attn) { u8g2.drawRBox(x, y, cardW, h, 2); u8g2.setDrawColor(0); }
      else      { u8g2.drawRFrame(x, y, cardW, h, 2); }

      u8g2.setFont(bigFont ? u8g2_font_6x10_tf : u8g2_font_5x8_tf);
      int adv  = bigFont ? 6 : 5;
      int chipW = bigFont ? 11 : 9;

      // [4.9.0-OLED] แก้: เลขเตียงอยู่ในป้ายทึบ แบบเดียวกับป้าย B01 ของ Station
      // การ์ดทึบ (ต้องไปดู) กลับเป็นป้ายโปร่งแทน เพื่อให้ยังแยกเลขเตียงออกได้
      char idStr[6];
      snprintf(idStr, sizeof(idStr), "%d", i + 1);
      if (!attn) {
        u8g2.drawRBox(x + 1, y + 1, chipW, h - 2, 1);
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 1 + (chipW - adv) / 2 + 1, textY, idStr);
        u8g2.setDrawColor(1);
      } else {
        u8g2.drawRFrame(x + 1, y + 1, chipW, h - 2, 1);
        u8g2.drawStr(x + 1 + (chipW - adv) / 2 + 1, textY, idStr);
      }

      if (isStationOnline(i) && stations[i].isRunning) {
        char rBuf[8];
        snprintf(rBuf, sizeof(rBuf), "%3.0f", stations[i].flowRate_ml_hr);
        u8g2.drawStr(x + chipW + 3, textY, rBuf);
      }

      // คำสถานะชิดขวาของการ์ดเสมอ ตาจึงกวาดหาคำผิดปกติได้เป็นแนวตั้ง
      const char *st = gridStatusOf(i);
      int stX = x + cardW - 3 - (int)strlen(st) * adv;
      u8g2.drawStr(stX, textY, st);

      u8g2.setDrawColor(1);
    }
  }

  // ---- แถบล่าง: เรียงตามความเร่งด่วน ----
  u8g2.drawHLine(0, 53, 128);
  u8g2.setFont(u8g2_font_5x8_tf);
  char buf[36];
  uint8_t beyond = heardBeyondBedCount();

  if (alarmBed > 0) {
    snprintf(buf, sizeof(buf), "%sB%d %s", isSnoozed() ? "(ZZ) " : "! ", alarmBed, alertTextEn(alarmCode));
    u8g2.drawBox(0, 54, 128, 10);
    u8g2.setDrawColor(0);
    u8g2.drawStr(2, 62, buf);
    u8g2.setDrawColor(1);
  }
  else if (anyIdConflict()) {
    // สำคัญกว่าทุกอย่างที่เหลือ เพราะทำให้ข้อมูลของเตียงอื่นหายไปทั้งเตียง
    u8g2.drawBox(0, 54, 128, 10);
    u8g2.setDrawColor(0);
    u8g2.drawStr(2, 62, "! 2 NODES SAME BED ID");
    u8g2.setDrawColor(1);
  }
  else if (beyond > 0) {
    snprintf(buf, sizeof(buf), "! HEARD BED %d - ADD BEDS", beyond);
    u8g2.drawFrame(0, 54, 128, 10);
    u8g2.drawStr(2, 62, buf);
  }
  else {
    int nearBed = 0;
    for (int i = 0; i < activeStationCount; i++) {
      if (stations[i].alertCode == ALERT_NEAR_END && !stations[i].nearEndAck) { nearBed = i + 1; break; }
    }
    if (nearBed > 0) {
      const StationData &ns = stations[nearBed - 1];
      int pct = (ns.cfg.planVolumeMl > 0) ? (int)(ns.totalVolumeMl * 100.0f / ns.cfg.planVolumeMl) : 0;
      snprintf(buf, sizeof(buf), "B%d NEW BAG SOON %d%%", nearBed, pct);
      u8g2.drawFrame(0, 54, 128, 10);
      u8g2.drawStr(2, 62, buf);
    } else {
      snprintf(buf, sizeof(buf), "ONLINE %d/%d", onlineCount, activeStationCount);
      u8g2.drawStr(0, 62, buf);
    }
  }
}

void updateHostOLED() {
  if (oledDisplaySleeping || isPowerOffProgressActive) return;
  u8g2.clearBuffer();

  bool nearPending = anyNearEndPending();
  if (!oledScreensaverActive && !globalAlarmTriggered && !nearPending && (millis() - lastUserActivityTime >= 300000)) {
    oledScreensaverActive = true;
  }
  // มีเหตุวิกฤต (ยังไม่พักเสียง) หรือเตือนใกล้หมดที่ยังไม่รับทราบ -> ออกจาก Screensaver เพื่อแสดงเตือน
  if (oledScreensaverActive && ((globalAlarmTriggered && !isSnoozed()) || nearPending)) {
    oledScreensaverActive = false;
    currentOledPage = 0;
  }

  if (oledScreensaverActive) {
    u8g2.setFont(u8g2_font_7x13_tf);
    u8g2.drawStr(20, 10, "SMART IV CLOCK");
    u8g2.drawHLine(10, 13, 108);

    u8g2.setFont(u8g2_font_logisoso32_tf);
    String clockStr = getOledClockStr();
    u8g2.drawStr(4, 50, clockStr.c_str());

    // ที่อยู่ที่พยาบาลต้องพิมพ์ลงเบราว์เซอร์ เปลี่ยนตามโหมดเครือข่าย
    u8g2.setFont(u8g2_font_4x6_tf);
    String addr = netModeLabel() + " " + activeIP().toString();
    u8g2.drawStr((128 - (int)addr.length() * 4) / 2, 62, addr.c_str());
  }
  else if (currentOledPage == 0) {
    drawTopBar("ALL");                // [4.9.0-OLED] แก้: หัวจอแบบเดียวกับ Station
    renderBlockStyleGrid();
  }
  else if (currentOledPage >= 1 && currentOledPage <= activeStationCount) {
    // [4.9.0-OLED] แก้: ผังเดียวกับหน้า IV BAG ของ Station
    char chip[6];
    snprintf(chip, sizeof(chip), "B%02d", (int)currentOledPage);
    drawTopBar(chip);
    renderBedDetail(currentOledPage - 1);
  }
  // ---- หน้าตรวจการเชื่อมต่อ (สำหรับช่างเท่านั้น อยู่ท้ายสุดของวง) ----
  else if (currentOledPage == activeStationCount + 1) {
    drawTopBar("NET");                // [4.9.0-OLED] แก้: หัวจอแบบเดียวกับ Station

    // ตารางเลขเตียง 1-8: ได้ยินไหม / MAC ท้าย 2 ไบต์ / คุณภาพลิงก์
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(0, 18, "BED MAC   LNK  BED MAC   LNK");
    for (int i = 0; i < MAX_SUPPORTED_STATIONS; i++) {
      int col = i / 4;
      int row = i % 4;
      int x = col * 65;
      int y = 26 + row * 7;
      const StationData &s = stations[i];
      char line[24];
      if (!heardStationId(i)) {
        snprintf(line, sizeof(line), "%d   --       -", i + 1);
      } else {
        snprintf(line, sizeof(line), "%d   %02X%02X%s %3d", i + 1, s.srcMac[4], s.srcMac[5],
                 s.idConflict ? "*" : " ", s.linkPct);
      }
      if (i >= activeStationCount && heardStationId(i)) {
        u8g2.drawBox(x, y - 5, 62, 7);      // ได้ยินแต่ยังไม่ได้เปิดใช้ = กลับสีให้สะดุดตา
        u8g2.setDrawColor(0);
        u8g2.drawStr(x + 1, y, line);
        u8g2.setDrawColor(1);
      } else {
        u8g2.drawStr(x + 1, y, line);
      }
    }

    u8g2.drawHLine(0, 54, 128);
    u8g2.setFont(u8g2_font_5x8_tf);
    // คำเตือนสำคัญมาก่อนเสมอ ถ้าไม่มีคำเตือนจึงแสดงที่อยู่ที่ต้องพิมพ์
    if (anyIdConflict())            u8g2.drawStr(0, 62, "* = 2 NODES SAME ID!");
    else if (heardBeyondBedCount()) u8g2.drawStr(0, 62, "INVERTED = ADD MORE BEDS");
    else {
      String line = activeIP().toString();
      if (netMode == NET_MODE_STA) line += "  CH" + String(currentWifiChannel());
      u8g2.drawStr(0, 62, line.c_str());
    }
  }

  u8g2.sendBuffer();
}
