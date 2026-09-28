/**
 * @file      StationScreen.h
 * @brief     ทุกอย่างที่วาดลงจอ TFT ของเครื่องประจำเตียง รุ่นพิเศษ Fallout
 * @version   1.0.0-FO
 * @date      2026-09-28
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 * อาจารย์ที่ปรึกษา ดร.กรรณิการ์ กาศสมบูรณ์
 *
 * @par Description
 * แยกออกมาจาก `Station.ino` เพื่อให้ไฟล์หลักสั้นและหาของเจอเร็วขึ้น
 * มีหน้าจอหลัก 4 หน้า หน้าเตือน หน้าพักจอ และหน้าตั้งเลขเตียง
 *
 * @par Fallout Edition
 * คัดลอกมาจาก `firmware/Station/StationScreen.h` แล้วเปลี่ยนเฉพาะหน้า 1
 * ภาพถุงน้ำเกลือกับกระเปาะหยดถูกแทนด้วยนักเดินทางในจอแบบหลอดภาพสีเขียว
 * ที่ **ก้าวหนึ่งจังหวะต่อหนึ่งหยดที่ตรวจจับได้จริง** พื้นหลังซากเมืองเลื่อนตามก้าว
 * ไม่มีหยดก็ยืนนิ่ง หยุดให้ยาขึ้น || สีเหลือง ไม่ไหลนั่งคุกเข่าขึ้น ! สีแดง
 * ระดับน้ำในถุงย้ายไปเป็นมาตรวัด 10 ช่องใต้ฉาก ตัวเลขและการแจ้งเตือนทุกอย่างเหมือนสายหลัก
 * หน้า 2 ถึง 4 หน้าเตือน และหน้าอื่นทั้งหมดไม่ได้แตะ นอกจากสีที่ไฟล์หลักกำหนด
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 1.0.0-FO | 2026-09-28 | หน้า 1 แทนถุงน้ำเกลือด้วยนักเดินทางที่ก้าวตามหยด และมาตรวัดระดับน้ำ 10 ช่อง |
 * | 7.10.0 | 2026-09-28 | (สืบทอดจากสายหลัก) การ์ดและแท่งสัญญาณแสดงสถานะ ONE-WAY |
 * | 7.8.0 | 2026-09-23 | (สืบทอดจากสายหลัก) แยกออกมาจากไฟล์หลัก ย้ายที่อยู่ล้วน ๆ ไม่ได้แก้เนื้อในแม้แต่บรรทัดเดียว |
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
 *  @file      StationScreen.h
 *  @brief     ทุกอย่างที่วาดลงจอ TFT ของเครื่องประจำเตียง สายหลัก
 *  @version   7.8.0
 *
 *  แยกออกมาจาก Station.ino เพื่อให้ไฟล์หลักสั้นและอ่านง่าย
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

// ---- ตัวช่วยวางข้อความ (ฟอนต์มาตรฐาน: 1 ตัวอักษร = 6xsize กว้าง, 8xsize สูง) ----
void textAt(const String &s, int x, int y, uint8_t size, uint16_t col, uint16_t bg) {
  tft.setTextSize(size);
  tft.setTextColor(col, bg);
  tft.setCursor(x, y);
  tft.print(s);
}

// ---- แถบสัญญาณ Host ----
void drawLinkBars(int x, int y, int rssi, bool online) {
  int bars = 0;
  uint16_t col = UI_GREEN;
  if (online) {
    if (rssi >= -65)      { bars = 4; col = UI_GREEN; }
    else if (rssi >= -75) { bars = 3; col = THEME_CYAN; }
    else if (rssi >= -85) { bars = 2; col = COLOR_ORANGE; }
    else                  { bars = 1; col = COLOR_RED; }
    if (linkState() != LINK_OK) col = COLOR_RED;   // [7.10.0] เพิ่ม: ได้ยิน Host แต่ Host ไม่ได้ยินเรา
  }
  int h[4] = {4, 7, 10, 13};
  for (int i = 0; i < 4; i++) {
    int bx = x + i * 4;
    tft.fillRect(bx, y + (13 - h[i]), 3, h[i], (i < bars) ? col : UI_LINE);
  }
  if (!online) {
    tft.drawLine(x, y, x + 14, y + 13, COLOR_RED);
    tft.drawLine(x, y + 13, x + 14, y, COLOR_RED);
  }
}

// ---- แถบบนสุด: ป้ายเตียงสีตามสถานะ | นาฬิกา | สัญญาณ ----
void drawTopBar(bool force) {
  UiStatus st = uiStatusOf(uiAlertCode);
  String clockStr = getStationClockStr();
  int bars = isHostOnline ? lastHostRssi : -999;

  if (force) {
    tft.fillRect(0, 0, 172, 22, UI_BAR);
    tft.fillRect(0, 22, 172, 2, uiStatusColor(st));
    cacheStatusBar = (int)st;
    cacheClock = "";
    cacheBars = -9999;
  } else if ((int)st != cacheStatusBar) {
    cacheStatusBar = (int)st;
    tft.fillRect(0, 22, 172, 2, uiStatusColor(st));
    cacheBedChip = false;
  }

  if (!cacheBedChip || force) {
    cacheBedChip = true;
    char bed[8];
    snprintf(bed, sizeof(bed), "B%02d", currentStationId);
    tft.fillRoundRect(4, 3, 30, 16, 4, uiStatusColor(st));
    textCenterIn(String(bed), 4, 30, 7, 1, THEME_BG, uiStatusColor(st));
  }

  if (clockStr != cacheClock) {
    cacheClock = clockStr;
    textCenterIn(clockStr, 46, 80, 7, 1, COLOR_WHITE, UI_BAR);
  }

  int barKey = ((bars == -999) ? -999 : (bars / 5)) * 4 + (int)linkState();   // [7.10.0] แก้
  if (barKey != cacheBars) {
    cacheBars = barKey;
    tft.fillRect(148, 4, 18, 14, UI_BAR);
    drawLinkBars(150, 4, lastHostRssi, isHostOnline);
  }
}

// ---- จุดบอกหน้า ----
void drawPageDots(int activeIdx, int y) {
  int count = 4;
  int w = count * 12 - 6;
  int x = (172 - w) / 2;
  tft.fillRect(0, y - 2, 172, 8, THEME_BG);
  for (int i = 0; i < count; i++) {
    if (i == activeIdx) tft.fillRoundRect(x + i * 12, y, 8, 4, 2, THEME_SKYBLUE);
    else                tft.fillRoundRect(x + i * 12, y, 6, 4, 2, UI_LINE);
  }
}

// ---- แถบความคืบหน้า ----
void drawProgressBar(int x, int y, int w, int h, int pct, uint16_t fill) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  tft.fillRoundRect(x, y, w, h, h / 2, UI_CARD);
  int fw = w * pct / 100;
  if (fw >= h) tft.fillRoundRect(x, y, fw, h, h / 2, fill);
  else if (fw > 0) tft.fillRect(x, y, fw, h, fill);
}

// ============================================================================
// หน้า 1 รุ่น Fallout: นักเดินทางก้าวหนึ่งก้าวต่อหนึ่งหยด แทนถุงน้ำเกลือ ([1.0.0-FO] เพิ่มทั้งหมด)
//
// หลักการคือภาพต้อง "บอกความจริง" เหมือนกระเปาะหยดของสายหลัก ไม่ใช่อนิเมชันลอย ๆ
//   - ทุกครั้งที่ตัวตรวจจับนับได้หนึ่งหยด (onDropDetected ตั้ง isDropFalling)
//     ตัวละครก้าวหนึ่งก้าว แล้วยืนนิ่งจนกว่าหยดถัดไปจะมา
//     หยดถี่ = เดินเร็ว หยดห่าง = เดินช้า หยุดไหล = หยุดเดิน พยาบาลเทียบกับหยดจริงได้ด้วยตา
//   - หยุดชั่วคราว = ยืนนิ่งพร้อมสัญลักษณ์ || สีเหลือง
//   - ไม่มีการไหล (สายพับ) = คุกเข่าลงพร้อม ! สีแดง
//   - ปริมาตรที่เหลือย้ายจากระดับน้ำในถุงไปเป็นแถบแบ่งช่องสิบช่องใต้ฉาก
//
// ตัวละครวาดจากท่อนแขนขาที่คำนวณมุมตามเฟสการเดิน แล้ววางลงตารางพิกเซลใหญ่ (หนึ่งช่อง
// = 3x3 พิกเซล) จึงได้ลายเส้นแบบพิกเซลอาร์ตและก้าวได้ลื่น ไม่ต้องเก็บภาพทีละเฟรม
// ตัวละครออกแบบใหม่ทั้งหมด (นักเดินทางในหมวกกันน็อกและเป้สะพาย) ไม่ได้ลอกลายเส้นของใคร
// ============================================================================
#define FO_SC_X        6            // กรอบฉาก
#define FO_SC_Y        34
#define FO_SC_W        72
#define FO_SC_H        150
#define FO_GROUND_Y    (FO_SC_Y + FO_SC_H - 16)
#define FO_FIG_X       (FO_SC_X + 36)
#define FO_U           3            // พิกเซลต่อหนึ่งช่องของตัวละคร
#define FO_RUIN        0x09E4       // เงาซากตึกไกล ๆ (เขียวมืด #0B3F22)
#define FO_GAUGE_Y     202

const unsigned long FO_STEP_MS = 340;   // ความยาวของหนึ่งก้าว
const int           FO_STRIDE  = 18;    // พื้นเลื่อนไปกี่พิกเซลต่อหนึ่งก้าว

float         foPhase      = 0.0f;  // เฟสการเดิน 0..1 — 0 และ 0.5 คือยืนเท้าชิด
float         foFrom       = 0.0f;
float         foScroll     = 0.0f;  // ระยะที่พื้นเลื่อนไปแล้ว
float         foScrollFrom = 0.0f;
bool          foStepping   = false;
unsigned long foStepStart  = 0;
int           foOX = 0, foOY = 0;   // จุดสะโพกของตัวละคร (พิกเซลจริง)

// วาดสี่เหลี่ยมโดยตัดขอบให้อยู่ในฉาก ไม่ให้ซากตึกหรือเศษหินล้นออกไปทับกรอบ
void foFillClip(int x, int y, int w, int h, uint16_t c) {
  int x0 = FO_SC_X + 2, y0 = FO_SC_Y + 14, x1 = FO_SC_X + FO_SC_W - 2, y1 = FO_SC_Y + FO_SC_H - 2;
  if (x < x0) { w -= x0 - x; x = x0; }
  if (y < y0) { h -= y0 - y; y = y0; }
  if (x + w > x1) w = x1 - x;
  if (y + h > y1) h = y1 - y;
  if (w > 0 && h > 0) tft.fillRect(x, y, w, h, c);
}

// หนึ่งช่องของตัวละคร (gx, gy นับเป็นช่อง เทียบกับสะโพก แกน y ชี้ลง)
void foCell(int gx, int gy, uint16_t c) {
  foFillClip(foOX + gx * FO_U, foOY + gy * FO_U, FO_U, FO_U, c);
}

// เส้นหนาสองช่องตามแนวนอน ใช้วาดแขนขา
void foLimb(float x0, float y0, float x1, float y1, uint16_t c) {
  int ax = (int)lroundf(x0), ay = (int)lroundf(y0), bx = (int)lroundf(x1), by = (int)lroundf(y1);
  int dx = abs(bx - ax), dy = -abs(by - ay), sx = ax < bx ? 1 : -1, sy = ay < by ? 1 : -1, err = dx + dy;
  for (;;) {
    foCell(ax, ay, c);
    foCell(ax + 1, ay, c);
    if (ax == bx && ay == by) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; ax += sx; }
    if (e2 <= dx) { err += dx; ay += sy; }
  }
}

// ขาหนึ่งข้าง: ต้นขายาว 4 ช่อง หน้าแข้งยาว 4 ช่อง แล้วรองเท้ายื่นไปข้างหน้า
void foLeg(float thigh, float shin, uint16_t c) {
  float kx = 4.0f * sinf(thigh), ky = 4.0f * cosf(thigh);
  float fx = kx + 4.0f * sinf(shin), fy = ky + 4.0f * cosf(shin);
  foLimb(0, 0, kx, ky, c);
  foLimb(kx, ky, fx, fy, c);
  foCell((int)lroundf(fx) + 2, (int)lroundf(fy), c);       // ปลายรองเท้า
}

void foArm(float a, uint16_t c) {
  foLimb(0, -6, 3.5f * sinf(a), -6 + 3.5f * cosf(a), c);
  foLimb(3.5f * sinf(a), -6 + 3.5f * cosf(a), 5.0f * sinf(a + 0.35f), -6 + 5.0f * cosf(a + 0.35f), c);
}

// ลำตัว หัว (หมวกกันน็อกพร้อมกระจกหน้า) และเป้สะพาย
void foBody(uint16_t c, uint16_t dim) {
  for (int y = -2; y <= 5; y++) for (int x = -4; x <= -3; x++) foCell(x, y - 8, dim);   // เป้
  for (int y = -7; y <= -1; y++) for (int x = -2; x <= 1; x++) foCell(x, y, c);        // ลำตัว
  for (int x = -2; x <= 1; x++) foCell(x, -1, dim);                                    // เข็มขัด
  for (int x = -1; x <= 1; x++) foCell(x, -12, c);                                     // หมวก
  for (int y = -11; y <= -9; y++) for (int x = -2; x <= 2; x++) foCell(x, y, c);
  foCell(1, -10, THEME_BG); foCell(2, -10, THEME_BG);                                  // กระจกหน้า
  for (int x = -1; x <= 1; x++) foCell(x, -8, c);                                      // คาง
}

// ท่าของตัวละครตามโหมด  0 = เดิน/ยืน, 2 = หยุดชั่วคราว (ยืน), 3 = ไม่มีการไหล (คุกเข่า)
void drawWanderer(int8_t mode) {
  uint16_t c = COLOR_GREEN, dim = UI_DIM;
  if (mode == 3) {
    foOX = FO_FIG_X; foOY = FO_GROUND_Y - 1 - 5 * FO_U;         // สะโพกต่ำลง = คุกเข่า
    foLeg(-0.35f, -1.75f, dim);                                  // ขาหลังพับราบกับพื้น
    foArm(0.15f, dim);
    foBody(c, dim);
    foLeg(1.45f, 0.0f, c);                                       // ขาหน้าตั้งเข่า
    foArm(0.55f, c);
    return;
  }
  float ph = (mode == 2) ? 0.0f : foPhase;
  float swing = sinf(ph * 6.2831853f);
  int bob = (fabsf(swing) > 0.6f) ? 1 : 0;                      // ช่วงก้าวยาวตัวต่ำลงนิดหนึ่ง
  foOX = FO_FIG_X; foOY = FO_GROUND_Y - 1 - 8 * FO_U + bob * FO_U;
  float a = 0.45f * swing;
  foArm(0.4f * swing, dim);                                      // แขนข้างไกล
  foLeg(-a, -a - (swing > 0 ? 0.5f * swing : 0.0f), dim);        // ขาข้างไกล
  foBody(c, dim);
  foLeg(a, a - (swing < 0 ? -0.5f * swing : 0.0f), c);           // ขาข้างใกล้
  foArm(-0.4f * swing, c);                                       // แขนข้างใกล้
}

// ฉากทั้งหมด: ซากตึกเลื่อนช้า (ไกล) พื้นและเศษหินเลื่อนตามก้าว (ใกล้) แล้วตัวละคร
void drawWandererScene(int8_t mode) {
  tft.fillRect(FO_SC_X + 2, FO_SC_Y + 14, FO_SC_W - 4, FO_SC_H - 16, THEME_BG);

  static const uint8_t ruinH[] = {34, 22, 46, 28};
  int far = ((int)(foScroll / 3)) % 96;
  for (int k = 0; k < 6; k++) {
    int rx = FO_SC_X + 2 + k * 24 - far;
    int h = ruinH[k % 4];
    foFillClip(rx, FO_GROUND_Y - h, 14, h, FO_RUIN);
    foFillClip(rx + 3, FO_GROUND_Y - h + 5, 3, 3, THEME_BG);      // หน้าต่างที่แตก
  }

  tft.drawFastHLine(FO_SC_X + 2, FO_GROUND_Y, FO_SC_W - 4, UI_LINE);
  int near = ((int)foScroll) % 12;
  for (int x = FO_SC_X + 4 - near; x < FO_SC_X + FO_SC_W; x += 12) {
    foFillClip(x, FO_GROUND_Y + 4, 3, 1, UI_LINE);
    foFillClip(x + 6, FO_GROUND_Y + 9, 2, 1, UI_LINE);
  }

  drawWanderer(mode);

  if (mode == 2) {                                                   // หยุดชั่วคราว
    foFillClip(FO_SC_X + FO_SC_W - 16, FO_SC_Y + 18, 3, 12, COLOR_YELLOW);
    foFillClip(FO_SC_X + FO_SC_W - 10, FO_SC_Y + 18, 3, 12, COLOR_YELLOW);
  } else if (mode == 3) {                                            // ไม่มีการไหล
    textAt("!", FO_SC_X + FO_SC_W - 16, FO_SC_Y + 18, 2, COLOR_RED, THEME_BG);
  }
}

// ปริมาตรที่เหลือ: แถบแบ่งสิบช่อง (แทนระดับน้ำในถุง) แล้วตัวเลข % อยู่ใต้แถบ
void drawBagLevel(bool force) {
  int pct = infusedPct();
  int left = (pct < 0) ? 100 : (100 - pct);
  uint16_t fill = (uiAlertCode == ALERT_NEAR_END) ? COLOR_ORANGE : COLOR_GREEN;
  if (!force && left == cacheBagPct) return;
  cacheBagPct = left;

  int on = (pct < 0) ? 0 : (left + 5) / 10;
  for (int i = 0; i < 10; i++) {
    int x = FO_SC_X + 1 + i * 7;
    tft.fillRect(x, FO_GAUGE_Y, 6, 12, (i < on) ? fill : UI_CARD);
  }
  String s = (pct < 0) ? String("-- LEFT") : (String(left) + "% LEFT");
  tft.fillRect(FO_SC_X, FO_GAUGE_Y + 18, FO_SC_W, 8, THEME_BG);
  textCenterIn(s, FO_SC_X, FO_SC_W, FO_GAUGE_Y + 18, 1, fill, THEME_BG);
}

// โหมดของภาพ 0 = เดิน/ยืน, 2 = หยุดชั่วคราว, 3 = ไม่มีการไหล
int8_t foModeNow() {
  if (!isRunning) return 2;
  // การ์ดสถานะขึ้น NO FLOW ได้ทั้งจากเครื่องนี้เองและจาก Host ภาพต้องตรงกับการ์ดเสมอ
  return (isLocallyOccluded() || uiStatusOf(uiAlertCode) == UI_NOFLOW) ? 3 : 0;
}

void updateDripAnimation() {
  if (currentState != STATE_NORMAL_VIEW || currentNursePage != 1 || stateNeedsRedraw) return;

  unsigned long now = millis();
  int8_t mode = foModeNow();

  // หยดใหม่มาถึง = เริ่มก้าวใหม่หนึ่งก้าว (ถ้าก้าวเดิมยังไม่จบ ให้จบทันทีก่อน ไม่ให้ก้าวหาย)
  if (isDropFalling) {
    isDropFalling = false;          // ตัวแปรนี้มีไว้ให้ภาพเคลื่อนไหวของหน้า 1 เท่านั้น
    if (foStepping) { foPhase = foFrom + 0.5f; foScroll = foScrollFrom + FO_STRIDE; }
    if (foPhase >= 1.0f) foPhase -= 1.0f;
    foFrom = foPhase;
    foScrollFrom = foScroll;
    if (foScroll > 100000.0f) { foScroll = 0; foScrollFrom = 0; }
    foStepping = true;
    foStepStart = now;
  }

  bool redraw = (mode != animModeDrawn);
  if (foStepping) {
    if (!redraw && now - lastAnimFrame < 30) return;              // ราว 30 เฟรมต่อวินาที
    float t = (float)(now - foStepStart) / (float)FO_STEP_MS;
    if (t >= 1.0f) { t = 1.0f; foStepping = false; }
    foPhase = foFrom + 0.5f * t;
    foScroll = foScrollFrom + FO_STRIDE * t;
    redraw = true;
  }
  if (!redraw) return;

  lastAnimFrame = now;
  animModeDrawn = mode;
  drawWandererScene(mode);
}

void drawPage1Framework() {
  tft.fillScreen(THEME_BG);
  drawTopBar(true);

  // กรอบฉากแบบจอ Pip-Boy: ขอบมน มุมหนา และป้ายบอกว่าหนึ่งก้าวคือหนึ่งหยด
  tft.drawRoundRect(FO_SC_X, FO_SC_Y, FO_SC_W, FO_SC_H, 5, UI_LINE);
  tft.fillRect(FO_SC_X, FO_SC_Y, 8, 3, COLOR_GREEN);
  tft.fillRect(FO_SC_X + FO_SC_W - 8, FO_SC_Y, 8, 3, COLOR_GREEN);
  tft.fillRect(FO_SC_X, FO_SC_Y + FO_SC_H - 3, 8, 3, COLOR_GREEN);
  tft.fillRect(FO_SC_X + FO_SC_W - 8, FO_SC_Y + FO_SC_H - 3, 8, 3, COLOR_GREEN);
  textCenterIn("STEP=DROP", FO_SC_X, FO_SC_W, FO_SC_Y + 4, 1, UI_DIM, THEME_BG);
  textAt("VOLUME", FO_SC_X + 2, FO_GAUGE_Y - 11, 1, UI_DIM, THEME_BG);

  textAt("RATE", RIGHT_X + 4, 36, 1, UI_DIM, THEME_BG);
  textAt("mL/h", RIGHT_X + 4, 82, 1, UI_DIM, THEME_BG);
  tft.drawFastHLine(RIGHT_X + 2, 110, 82, UI_LINE);
  textAt("LEFT", RIGHT_X + 4, 118, 1, UI_DIM, THEME_BG);
  tft.drawFastHLine(RIGHT_X + 2, 154, 82, UI_LINE);
  textAt("TIME LEFT", RIGHT_X + 4, 160, 1, UI_DIM, THEME_BG);

  drawPageDots(0, 308);

  cacheBagPct = -999; cacheRate = -9999; cacheTarget = -9999; cacheLeftMl = -9999;
  cachePct = -999; cacheTimeLeft = ""; cacheEndClock = ""; cacheStatusWord = "";
  // วาดฉากทันทีพร้อมกรอบ ไม่ต้องรอให้ loop() เรียก updateDripAnimation() รอบแรก
  animModeDrawn = foModeNow();
  drawWandererScene(animModeDrawn);
  poolFlashUntil = 0;
  stateNeedsRedraw = false;
}

void updatePage1Dynamic() {
  drawTopBar(false);
  drawBagLevel(false);

  UiStatus st = uiStatusOf(uiAlertCode);
  int rate = isRunning ? (int)(currentFlowRate_ml_hr + 0.5f) : -1;
  int target = (int)(targetRateHr + 0.5f);

  if (rate != cacheRate || target != cacheTarget) {
    cacheRate = rate; cacheTarget = target;
    tft.fillRect(RIGHT_X + 4, 48, 84, 32, THEME_BG);
    if (!isRunning) textAt("--", RIGHT_X + 4, 48, 4, THEME_SKYBLUE, THEME_BG);
    else textAt(String(rate), RIGHT_X + 4, 48, 4, (st == UI_NOFLOW) ? COLOR_RED : COLOR_WHITE, THEME_BG);
    textAt(padTo(targetRateHr > 0 ? ("SET " + String(target)) : String("SET --"), 9),
           RIGHT_X + 4, 94, 1, THEME_SKYBLUE, THEME_BG);
  }

  int left = (totalPlanMl > 0) ? remainingMlInt() : -1;
  if (left != cacheLeftMl) {
    cacheLeftMl = left;
    textAt(padTo(left >= 0 ? (String(left) + " mL") : String("-- mL"), 7), RIGHT_X + 4, 130, 2, COLOR_WHITE, THEME_BG);
  }

  String tl = timeLeftText(true);
  if (tl != cacheTimeLeft) {
    cacheTimeLeft = tl;
    textAt(padTo(tl, 7), RIGHT_X + 4, 172, 2, COLOR_WHITE, THEME_BG);
  }

  String ec = endClockText();
  if (ec != cacheEndClock) {
    cacheEndClock = ec;
    textAt(padTo("END " + ec, 11), RIGHT_X + 4, 196, 1, UI_DIM, THEME_BG);
  }

  String word = String(uiStatusWord(st));
  if (word != cacheStatusWord) {
    cacheStatusWord = word;
    uint16_t sc = uiStatusColor(st);
    tft.fillRoundRect(RIGHT_X, 214, 84, 40, 6, sc);
    textCenterIn(word, RIGHT_X, 84, 229, 1, THEME_BG, sc);
  }

  int pct = infusedPct();
  if (pct != cachePct) {
    cachePct = pct;
    String line = (totalPlanMl > 0)
      ? ("INFUSED " + String((int)totalVolumeMl) + "/" + String((int)totalPlanMl) + " mL")
      : ("INFUSED " + String((int)totalVolumeMl) + " mL");
    textAt(padTo(line, 22), 12, 272, 1, UI_DIM, THEME_BG);
    textRight(padTo(pct >= 0 ? (String(pct) + "%") : String("--"), 4), 164, 272, 1, COLOR_WHITE, THEME_BG);
    drawProgressBar(12, 286, 148, 12, (pct < 0) ? 0 : pct,
                    (uiAlertCode == ALERT_NEAR_END) ? COLOR_ORANGE : THEME_CYAN);
  }
}

// ============================================================================
// หน้า 2: PLAN — แผนการให้สารน้ำและเวลา (ตัวเลขใหญ่ 4 ค่า)
// ============================================================================
void drawStatCard(int y, const char* label, const String &value, uint16_t valueColor, bool labelOnly) {
  if (labelOnly) {
    tft.fillRoundRect(6, y, 160, 46, 6, UI_CARD);
    textAt(String(label), 14, y + 7, 1, UI_DIM, UI_CARD);
    return;
  }
  tft.fillRect(60, y + 18, 100, 26, UI_CARD);
  textRight(value, 158, y + 20, 3, valueColor, UI_CARD);
}

void drawPage2Framework() {
  tft.fillScreen(THEME_BG);
  drawTopBar(true);
  drawStatCard(30,  "TARGET   mL/h", "", 0, true);
  drawStatCard(84,  "PLAN   mL",     "", 0, true);
  drawStatCard(138, "INFUSED   mL",  "", 0, true);
  drawStatCard(192, "FINISH AT",     "", 0, true);
  drawPageDots(1, 308);
  cacheP2a = ""; cacheP2b = ""; cacheP2c = ""; cacheP2d = ""; cacheP2foot = "";
  stateNeedsRedraw = false;
}

// ============================================================================
// หน้า 3: LINK — สถานะการเชื่อมต่อและเซนเซอร์
// ============================================================================
void drawPage3Framework() {
  tft.fillScreen(THEME_BG);
  drawTopBar(true);

  tft.fillRoundRect(6, 30, 160, 60, 6, UI_CARD);
  textAt("HOST LINK", 14, 36, 1, UI_DIM, UI_CARD);

  tft.fillRoundRect(6, 98, 160, 86, 6, UI_CARD);
  textAt("SENSOR", 14, 104, 1, UI_DIM, UI_CARD);

  tft.fillRoundRect(6, 192, 160, 76, 6, UI_CARD);
  textAt("DEVICE", 14, 198, 1, UI_DIM, UI_CARD);

  drawPageDots(2, 308);
  cacheP3a = ""; cacheP3b = ""; cacheP3c = "";
  stateNeedsRedraw = false;
}

void updatePage3Dynamic() {
  drawTopBar(false);

  // [7.10.0] แก้: แยก "ได้ยินกันทางเดียว" ออกจาก ONLINE
  uint8_t ls = linkState();
  String linkKey = String(ls) + ":" + String(lastHostRssi / 5) + ":" + String(espnowChannel);
  if (linkKey != cacheP3a) {
    cacheP3a = linkKey;
    const char *word = (ls == LINK_OK) ? "ONLINE" : (ls == LINK_NONE) ? "NO LINK" : "ONE-WAY";
    uint16_t wc = (ls == LINK_OK) ? UI_GREEN : (ls == LINK_NONE) ? COLOR_RED : COLOR_ORANGE;
    textAt(padTo(word, 8), 14, 50, 2, wc, UI_CARD);
    tft.fillRect(134, 48, 20, 16, UI_CARD);
    drawLinkBars(136, 48, lastHostRssi, isHostOnline);
    String sub;
    if (ls == LINK_NONE)            sub = "SEARCHING CH " + String(espnowChannel);
    else if (ls == LINK_NOT_HEARD)  sub = "HOST CAN'T HEAR BED";
    else if (ls == LINK_NOT_LISTED) sub = "BED NOT ON HOST LIST";
    else                            sub = "CH " + String(espnowChannel) + "   " + String(lastHostRssi) + " dBm";
    textAt(padTo(sub, 22), 14, 72, 1, (ls == LINK_OK || ls == LINK_NONE) ? UI_DIM : COLOR_ORANGE, UI_CARD);
  }

  // สถานะที่พยาบาลอ่านแล้วรู้ว่าต้องทำอะไรต่อ — ไม่ต้องตีความตัวเลขเอง
  const iv::DetectorStatus &ds = det();
  String sensorKey = String(sensorStateWord()) + ":" + String(ds.snrX10 / 5) + ":" +
                     String(ds.motion ? 1 : 0);
  if (sensorKey != cacheP3b) {
    cacheP3b = sensorKey;
    textAt(padTo(String(sensorStateWord()), 14), 14, 118, 2, sensorStateColor(), UI_CARD);
    textAt(padTo("SNR " + String(ds.snrX10 / 10) + "." + String(ds.snrX10 % 10) +
                 "x   STEADY " + String(ds.confidencePct) + "%", 22),
           14, 144, 1, UI_DIM, UI_CARD);
    textAt(padTo(ds.motion ? "MOVING - bag swinging"
                           : ("DROP " + String(ds.learnedAmp) + " / " +
                              String(ds.learnedWidthMs) + " ms"), 22),
           14, 160, 1, ds.motion ? COLOR_YELLOW : UI_DIM, UI_CARD);
  }

  float bv = readStationBattery();
  String devKey = String(currentStationId) + ":" + String(safeDropFactor(dropFactor)) + ":" + String((int)(bv * 10)) + ":" + String(totalDrops);
  if (devKey != cacheP3c) {
    cacheP3c = devKey;
    char bed[12];
    snprintf(bed, sizeof(bed), "BED %02d", currentStationId);
    textAt(padTo(String(bed) + "  DF" + String(safeDropFactor(dropFactor)), 12), 14, 212, 2, COLOR_WHITE, UI_CARD);
    textAt(padTo("DROPS " + String(totalDrops), 22), 14, 236, 1, UI_DIM, UI_CARD);
    String pw = (bv > 0.5f) ? ("BATT " + String(bv, 2) + " V") : String("POWER USB");
    textAt(padTo("FW " APP_VERSION "   " + pw, 22), 14, 252, 1, UI_DIM, UI_CARD);
  }
}

void drawTrendChart() {
  const int chartH = CHART_BOT - CHART_TOP;
  const int chartW = TREND_BARS * CHART_BARW;

  float maxV = (targetRateHr > 0) ? targetRateHr : 1.0f;
  for (int i = 1; i < TREND_BARS; i++) if (trendValueAt(i) > maxV) maxV = trendValueAt(i);
  if (trendLive > maxV) maxV = trendLive;
  trendScale = niceScale(maxV * 1.1f);

  tft.fillRect(CHART_X - 3, CHART_TOP - 12, chartW + 6, chartH + 26, THEME_BG);
  for (int g = 1; g <= 3; g++) {
    int gy = CHART_BOT - (chartH * g) / 4;
    for (int x = CHART_X; x < CHART_X + chartW; x += 4) tft.drawPixel(x, gy, UI_CARD);
  }

  for (int i = 0; i < TREND_BARS; i++) {
    bool live = (i == TREND_BARS - 1);
    float v = live ? trendLive : trendValueAt(i + 1);
    bool has = live ? true : trendValidAt(i + 1);
    int x = CHART_X + i * CHART_BARW;
    int h = (int)((v / trendScale) * chartH);
    if (h > chartH) h = chartH;
    if (!has)        tft.drawFastHLine(x, CHART_BOT, CHART_BARW - 1, UI_CARD);
    else if (h <= 1) tft.drawFastHLine(x, CHART_BOT, CHART_BARW - 1, COLOR_RED);
    else             tft.fillRect(x, CHART_BOT - h, CHART_BARW - 1, h, live ? THEME_PINK : THEME_CYAN);
  }

  if (targetRateHr > 0) {                       // เส้นเป้าหมาย
    int ty = CHART_BOT - (int)((targetRateHr / trendScale) * chartH);
    if (ty < CHART_TOP) ty = CHART_TOP;
    for (int x = CHART_X; x < CHART_X + chartW; x += 6) tft.drawFastHLine(x, ty, 3, COLOR_YELLOW);
  }

  tft.drawFastHLine(CHART_X - 2, CHART_BOT + 1, chartW + 4, UI_LINE);
  textAt(padTo(String((int)trendScale) + " mL/h", 10), CHART_X, CHART_TOP - 12, 1, UI_DIM, THEME_BG);
  textAt("-30m", CHART_X, CHART_BOT + 6, 1, UI_DIM, THEME_BG);
  textRight("now", CHART_X + chartW, CHART_BOT + 6, 1, UI_DIM, THEME_BG);
  trendNeedsRedraw = false;
}

void drawTrendLiveBar() {
  if (trendLive > trendScale) { drawTrendChart(); return; }
  const int chartH = CHART_BOT - CHART_TOP;
  int x = CHART_X + (TREND_BARS - 1) * CHART_BARW;
  int h = (int)((trendLive / trendScale) * chartH);
  if (h > chartH) h = chartH;
  tft.fillRect(x, CHART_TOP, CHART_BARW - 1, chartH, THEME_BG);
  if (targetRateHr > 0) {                       // คืนเส้นเป้าหมายในช่วงที่ลบไป
    int ty = CHART_BOT - (int)((targetRateHr / trendScale) * chartH);
    if (ty >= CHART_TOP && ty <= CHART_BOT) tft.drawFastHLine(x, ty, CHART_BARW - 1, COLOR_YELLOW);
  }
  if (h <= 1) tft.drawFastHLine(x, CHART_BOT, CHART_BARW - 1, COLOR_RED);
  else        tft.fillRect(x, CHART_BOT - h, CHART_BARW - 1, h, THEME_PINK);
}

void drawPage4Framework() {
  tft.fillScreen(THEME_BG);
  drawTopBar(true);
  textAt("FLOW TREND", 12, 32, 1, UI_DIM, THEME_BG);
  textRight(padTo(targetRateHr > 0 ? ("TARGET " + String((int)(targetRateHr + 0.5f))) : String("TARGET --"), 10),
            164, 32, 1, COLOR_YELLOW, THEME_BG);
  tft.fillRoundRect(6, 216, 160, 54, 6, UI_CARD);
  textAt("NOW", 14, 222, 1, UI_DIM, UI_CARD);
  textAt("AVG 30m", 100, 222, 1, UI_DIM, UI_CARD);
  drawTrendChart();
  drawPageDots(3, 308);
  cacheP4 = "";
  stateNeedsRedraw = false;
}

// ============================================================================
// จอเตือนเหตุวิกฤต (เต็มจอสีแดง)
// ============================================================================
void drawEmergencyScreen(uint8_t code) {
  tft.fillScreen(COLOR_RED);
  tft.fillRoundRect(12, 12, 148, 32, 6, COLOR_WHITE);
  char bed[12];
  snprintf(bed, sizeof(bed), "BED %02d", currentStationId);
  drawCenteredString(String(bed), 20, 2, COLOR_RED, COLOR_WHITE);

  const char* headline = "ALERT";
  const char* sub      = "CHECK IV SYSTEM";
  if (code == ALERT_OCCLUSION)      { headline = "NO FLOW";  sub = "CHECK IV LINE"; }
  else if (code == ALERT_TOO_FAST)  { headline = "TOO FAST"; sub = "ADJUST ROLLER CLAMP"; }
  else if (code == ALERT_TOO_SLOW)  { headline = "TOO SLOW"; sub = "ADJUST ROLLER CLAMP"; }
  else if (code == ALERT_COMPLETE)  { headline = "COMPLETE"; sub = "INFUSION FINISHED"; }

  drawCenteredString(String(headline), 62, 3, COLOR_WHITE, COLOR_RED);
  drawCenteredString(String(sub), 96, 1, COLOR_YELLOW, COLOR_RED);

  tft.fillRoundRect(12, 118, 148, 100, 8, THEME_BG);
  textAt("RATE", 22, 126, 1, UI_DIM, THEME_BG);
  textAt("mL/h", 22, 186, 1, UI_DIM, THEME_BG);
  textRight(targetRateHr > 0 ? ("SET " + String((int)(targetRateHr + 0.5f))) : String("SET --"),
            150, 186, 1, THEME_SKYBLUE, THEME_BG);

  tft.fillRoundRect(14, 230, 144, 62, 8, COLOR_WHITE);
  drawCenteredString("CLICK = SNOOZE 2min", 244, 1, COLOR_RED, COLOR_WHITE);
  drawCenteredString("DOUBLE = PAUSE", 268, 1, COLOR_RED, COLOR_WHITE);

  lastEmergencyCode = code;
  cacheRate = -9999;
  stateNeedsRedraw = false;
}

void updateEmergencyDynamic() {
  int rate = (int)(currentFlowRate_ml_hr + 0.5f);
  if (rate == cacheRate) return;
  cacheRate = rate;
  tft.fillRect(20, 140, 132, 40, THEME_BG);
  textAt(String(rate), 22, 142, 5, COLOR_WHITE, THEME_BG);
}

// ============================================================================
// จอเตือนใกล้หมดถุง (สีส้ม ไม่ใช่เหตุวิกฤต)
// ============================================================================
void drawNearEndNoticeFramework() {
  tft.fillScreen(COLOR_ORANGE);
  tft.fillRoundRect(12, 12, 148, 32, 6, COLOR_WHITE);
  char bed[12];
  snprintf(bed, sizeof(bed), "BED %02d", currentStationId);
  drawCenteredString(String(bed), 20, 2, COLOR_ORANGE, COLOR_WHITE);

  drawCenteredString("NEXT BAG", 60, 3, THEME_BG, COLOR_ORANGE);
  drawCenteredString("PREPARE NEW IV BAG", 94, 1, THEME_BG, COLOR_ORANGE);

  tft.fillRoundRect(12, 114, 148, 104, 8, THEME_BG);
  tft.fillRoundRect(14, 236, 144, 58, 8, COLOR_WHITE);
  drawCenteredString("CLICK = OK", 248, 2, COLOR_ORANGE, COLOR_WHITE);
  drawCenteredString("(stop reminder)", 274, 1, THEME_BG, COLOR_WHITE);

  cacheNear = "";
  stateNeedsRedraw = false;
}

// ============================================================================
// Screensaver — เห็นเฉพาะสิ่งที่ต้องดูจากระยะไกล
// ============================================================================
void drawScreensaverFramework() {
  tft.fillScreen(THEME_BG);
  UiStatus st = uiStatusOf(uiAlertCode);
  char bed[12];
  snprintf(bed, sizeof(bed), "BED %02d", currentStationId);
  tft.fillRoundRect(46, 20, 80, 28, 6, uiStatusColor(st));
  textCenterIn(String(bed), 46, 80, 26, 2, THEME_BG, uiStatusColor(st));
  drawCenteredString("mL/h", 232, 1, UI_DIM, THEME_BG);
  drawCenteredString("CLICK TO WAKE", 296, 1, UI_DIM, THEME_BG);
  cacheClock = ""; cacheRate = -9999; cacheStatusWord = "";
  stateNeedsRedraw = false;
}

void updateScreensaverDynamic() {
  String clockStr = getStationClockStr();
  if (clockStr != cacheClock) {
    cacheClock = clockStr;
    drawCenteredString(clockStr, 76, 5, COLOR_WHITE, THEME_BG);
  }

  UiStatus st = uiStatusOf(uiAlertCode);
  String word = String(uiStatusWord(st));
  if (word != cacheStatusWord) {
    cacheStatusWord = word;
    tft.fillRect(6, 136, 160, 20, THEME_BG);
    drawCenteredString(word, 138, 2, uiStatusColor(st), THEME_BG);
  }

  int rate = isRunning ? (int)(currentFlowRate_ml_hr + 0.5f) : -1;
  if (rate != cacheRate) {
    cacheRate = rate;
    tft.fillRect(6, 180, 160, 44, THEME_BG);
    drawCenteredString(rate >= 0 ? String(rate) : String("--"), 182, 5,
                       (st == UI_NOFLOW) ? COLOR_RED : COLOR_WHITE, THEME_BG);
  }
}

// ============================================================================
// หน้าผู้พัฒนา / หน้าตั้งหมายเลขเตียง
// ============================================================================
void drawDeveloperCreditScreen() {
  tft.fillScreen(THEME_BG);
  tft.fillRoundRect(6, 16, 160, 34, 6, THEME_PINK);
  drawCenteredString("SMART IV ALERT", 26, 2, THEME_BG, THEME_PINK);

  drawCenteredString("Bed Station Node", 66, 1, UI_DIM, THEME_BG);
  drawCenteredString("FW v" APP_VERSION, 82, 1, THEME_SKYBLUE, THEME_BG);

  tft.fillRoundRect(6, 106, 160, 96, 8, UI_CARD);
  drawCenteredString("DEVELOPER", 114, 1, UI_DIM, UI_CARD);
  drawCenteredString("Kittiphan", 132, 2, COLOR_YELLOW, UI_CARD);
  drawCenteredString("Rattanakorn", 154, 2, COLOR_YELLOW, UI_CARD);
  drawCenteredString("MCU Phrae Campus", 180, 1, UI_DIM, UI_CARD);

  tft.fillRoundRect(6, 212, 160, 60, 8, UI_CARD);
  drawCenteredString("BCN Phrae Innovation", 224, 1, THEME_CYAN, UI_CARD);
  drawCenteredString("Suk Mean Hospital Trial", 244, 1, UI_DIM, UI_CARD);

  drawCenteredString("CLICK TO RETURN", 296, 1, UI_DIM, THEME_BG);
  stateNeedsRedraw = false;
}

// ----------------------------------------------------------------------------
// ทิศการแสดงผลของจอ
// ----------------------------------------------------------------------------
// setRotation(0) กับ (2) เป็นแนวตั้ง 172x320 เหมือนกัน ต่างกันแค่บิต MX/MY ใน
// MADCTL พิกัดที่ใช้วาดทั้งไฟล์จึงไม่ต้องแก้ เพียงสั่งวาดหน้าเดิมซ้ำอีกครั้ง
void applyScreenRotation(uint8_t rot) {
  screenRotation = (rot == TFT_ROT_FLIP180) ? TFT_ROT_FLIP180 : TFT_ROT_NORMAL;
  tft.setRotation(screenRotation);
  tft.setTextWrap(false);
  tft.fillScreen(THEME_BG);
  stateNeedsRedraw = true;      // ให้ทุกหน้าวาดกรอบใหม่ทั้งหน้า (แคชถูกล้างในนั้น)
}

void drawStationIdConfigFramework() {
  tft.fillScreen(THEME_BG);
  tft.fillRoundRect(6, 16, 160, 34, 6, THEME_SKYBLUE);
  drawCenteredString("SET BED ID", 26, 2, THEME_BG, THEME_SKYBLUE);
  tft.fillRoundRect(6, 62, 160, 150, 8, UI_CARD);

  // แถวตั้งค่าทิศจอ — ช่วยให้ติดตั้งกล่องได้ทั้งแบบสาย USB ลงล่างและขึ้นบน
  tft.fillRoundRect(6, 220, 160, 42, 8, UI_CARD);
  drawCenteredString("SCREEN", 226, 1, UI_DIM, UI_CARD);
  drawCenteredString(padTo(screenRotationLabel(), 10), 240, 2, THEME_CYAN, UI_CARD);

  drawCenteredString("CLICK = BED 1-8", 272, 1, UI_DIM, THEME_BG);
  drawCenteredString("DBL CLICK = FLIP", 284, 1, UI_DIM, THEME_BG);
  cacheConfig = "";
  stateNeedsRedraw = false;
}

void updateStationIdConfigDynamic() {
  long msLeft = (long)configAutoSaveTimeout - (long)millis();
  int leftSec = (msLeft > 0) ? (int)(msLeft / 1000) + 1 : 0;
  String key = String(tempConfigStationId) + ":" + String(leftSec);
  if (key == cacheConfig) return;
  cacheConfig = key;

  tft.fillRect(40, 84, 92, 108, UI_CARD);
  drawCenteredString(String(tempConfigStationId), 90, 12, THEME_SKYBLUE, UI_CARD);
  drawCenteredString(padTo("SAVING IN " + String(leftSec) + "s", 16), 302, 1, COLOR_YELLOW, THEME_BG);
}
