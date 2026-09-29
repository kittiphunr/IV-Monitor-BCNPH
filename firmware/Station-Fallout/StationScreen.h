/**
 * @file      StationScreen.h
 * @brief     ทุกอย่างที่วาดลงจอ TFT ของเครื่องประจำเตียง รุ่นพิเศษ Fallout
 * @version   1.3.0-FO
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
 * จัดแบบหน้า STAT ของจอ Pip-Boy: แถบแท็บ ตัวละครในกรอบขีดหกขีด กล่องไอคอน และแถบล่าง
 * แบบ HP / LEVEL / AP ภาพถุงน้ำเกลือถูกแทนด้วย Vault Boy (ภาพอยู่ใน `pipboy_sprite.h`)
 * ที่ **เดินเมื่อน้ำเกลือกำลังหยด และหยุดเมื่อหยดหยุด** ภาพเดินเล่นวนตามเวลาของตัวเอง
 * แยกจากจังหวะของหยดแต่ละหยด หยุดให้ยายืนพร้อม || สีเหลือง ไม่ไหลยืนในกรอบสีแดงพร้อม ! สีแดง
 * ตัวเลขและการแจ้งเตือนทุกอย่างเหมือนสายหลัก
 * หน้า 2 ถึง 4 หน้าเตือน และหน้าอื่นทั้งหมดไม่ได้แตะ นอกจากสีที่ไฟล์หลักกำหนด
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 1.3.0-FO | 2026-09-28 | ภาพเดินแยกจากจังหวะหยด เล่นวนตามเวลาของตัวเองเมื่อกำลังหยด หยดหยุดภาพก็หยุด |
 * | 1.2.0-FO | 2026-09-28 | Vault Boy คมขึ้นพร้อมเส้นสแกนแบบจอ CRT ท่าเดินห้าเฟรม (ก้าวยาว ขาสวนกัน ท่ายืน) |
 * | 1.1.0-FO | 2026-09-28 | หน้า 1 จัดแบบหน้า STAT ของ Pip-Boy และใช้ภาพ Vault Boy สามเฟรมแทนนักเดินทางที่วาดจากเส้น |
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

#include "pipboy_sprite.h"   // [1.1.0-FO] เพิ่ม: ภาพ Vault Boy ท่าเดิน (สร้างด้วย tools/fallout-sprite)

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
// หน้า 1 รุ่น Fallout: หน้า STAT ของ Pip-Boy — Vault Boy เดินเมื่อน้ำเกลือกำลังหยด
// ([1.0.0-FO] เพิ่มทั้งหมด, [1.1.0-FO] แก้: จัดใหม่ตามหน้า STAT และใช้ภาพ Vault Boy,
//  [1.2.0-FO] แก้: ภาพคมขึ้นพร้อมเส้นสแกน ท่าเดินห้าเฟรม เท้าหลังยกและลำตัวขึ้นลงตอนขาสวนกัน,
//  [1.3.0-FO] แก้: ภาพเดินแยกจากจังหวะหยด เล่นวนตามเวลาของตัวเอง)
//
// แยกกันเป็นสองส่วนชัดเจน
//   ส่วนที่ 1  "กำลังหยดอยู่ไหม" (foDripping) — ตัดสินจากเวลาของหยดล่าสุดเท่านั้น
//              มีหยดมาไม่ห่างเกิน foIdleMs() = กำลังหยด
//   ส่วนที่ 2  ภาพเดิน — เล่นวนตามลำดับและเวลาของเฟรมใน pipboy_sprite.h (FO_WALK_SEQ / FO_WALK_MS)
//              ด้วยนาฬิกาของตัวเอง ไม่สนว่าหยดแต่ละหยดตกเมื่อไร หยดถี่หรือห่างก็เดินเร็วเท่าเดิม
//   ส่วนที่ 1 บอกแค่ "เดิน" หรือ "หยุด" ให้ส่วนที่ 2
//   - เริ่มหยด = เริ่มเดินต่อจากท่าที่ค้างไว้
//   - หยดหยุด  = หยุดค้างท่าปัจจุบันทันที (ภายใน foIdleMs หลังหยดสุดท้าย)
//   - หยุดชั่วคราว = ยืน (FO_SPR_STAND) พร้อมสัญลักษณ์ || สีเหลือง
//   - ไม่มีการไหล = ยืน ขอบกรอบเป็นสีแดง พร้อม ! สีแดง
//
// จัดวางตามหน้า STAT ของจอ Pip-Boy (ภาพ statpip0.bmp ของ Adafruit PipBoy_2040)
//   แถบแท็บ STAT PLAN LINK TREND · ตัวละครในกรอบขีดหกขีด · กล่องไอคอนพร้อมตัวเลขใต้ภาพ
//   ปุ่มแบบ STIMPAK/RADAWAY · แถบล่างแบบ HP / LEVEL / AP (ใช้แสดงปริมาตรที่ให้ไปแล้ว)
// ============================================================================
#define FO_FR_X        2            // กรอบตัวละคร
#define FO_FR_Y        46
#define FO_FR_W        80
#define FO_FR_H        180
#define FO_SPR_X       (FO_FR_X + (FO_FR_W - FO_SPR_W) / 2)
#define FO_SPR_Y       (FO_FR_Y + (FO_FR_H - FO_SPR_H) / 2)
#define FO_GROUND_Y    (FO_SPR_Y + FO_SPR_H + 2)
#define FO_ICON_Y      232
#define FO_BTN_Y       272
#define FO_HUD_Y       294

// [1.3.0-FO] แก้: เลิกก้าวตามหยด สถานะของภาพเดินเหลือแค่ตำแหน่งในลำดับการเล่นวน
const unsigned long FO_IDLE_MIN_MS = 3000;   // หยดหายไปนานเท่านี้ขึ้นไป ถือว่าหยุดหยดแน่นอน

bool          foWalking    = false;  // ภาพเดินอยู่หรือไม่ (ผลของส่วนที่ 1)
uint8_t       foSeqPos     = 0;      // ช่องปัจจุบันใน FO_WALK_SEQ
unsigned long foFrameStart = 0;      // เวลาที่เริ่มแสดงช่องปัจจุบัน
int8_t        foFrameDrawn = -1;
uint8_t       foGroundPhase = 0;     // เส้นพื้นเลื่อนหนึ่งพิกเซลต่อหนึ่งเฟรมของภาพเดิน
uint16_t      foFrameColor = 0;      // สีขีดกรอบที่วาดอยู่ (เปลี่ยนเป็นแดงเมื่อไม่ไหล)
String        cacheFoDrops = "", cacheFoGtt = "", cacheFoWalk = "";

// วาดหนึ่งเฟรม: ไล่ทีละแถว รวมพิกเซลสีเดียวกันที่ติดกันเป็นเส้นเดียว (ส่งข้อมูลไปจอน้อยลงมาก)
// พื้นของภาพเป็นสีเดียวกับ THEME_BG จึงวาดทับเฟรมเดิมได้ทันที ไม่ต้องล้างก่อน จอไม่กะพริบ
void foDrawSprite(uint8_t f) {
  const uint8_t *p = FO_SPR_DATA[f];
  for (int r = 0; r < FO_SPR_H; r++) {
    const uint8_t *row = p + r * FO_SPR_ROW;
    int runX = 0;
    uint8_t runC = row[0] >> 4;
    for (int c = 1; c <= FO_SPR_W; c++) {
      uint8_t v = 0xFF;
      if (c < FO_SPR_W) v = (c & 1) ? (row[c >> 1] & 0x0F) : (row[c >> 1] >> 4);
      if (v != runC) {
        tft.drawFastHLine(FO_SPR_X + runX, FO_SPR_Y + r, c - runX, FO_SPR_PAL[runC]);
        runC = v;
        runX = c;
      }
    }
  }
}

// ขีดหกขีดรอบตัวละครแบบหน้า STAT (บน ล่าง และซ้ายขวาข้างละสอง)
void foDrawFrameMarks(uint16_t c) {
  int cx = FO_FR_X + FO_FR_W / 2;
  tft.fillRect(cx - 7, FO_FR_Y, 14, 3, c);
  tft.fillRect(cx - 7, FO_FR_Y + FO_FR_H - 3, 14, 3, c);
  for (int k = 0; k < 2; k++) {
    int y = FO_FR_Y + 52 + k * 76;
    tft.fillRect(FO_FR_X, y, 11, 3, c);
    tft.fillRect(FO_FR_X + FO_FR_W - 11, y, 11, 3, c);
  }
  foFrameColor = c;
}

// เส้นพื้นประใต้เท้า เลื่อนไปทางซ้ายพร้อมภาพเดิน ให้เห็นว่าเดินไปข้างหน้าจริง
void foDrawGround() {
  int x0 = FO_SPR_X - 4, w = FO_SPR_W + 8;
  tft.fillRect(x0, FO_GROUND_Y, w, 1, THEME_BG);
  for (int x = x0 + 11 - (foGroundPhase % 12); x < x0 + w; x += 12)
    if (x + 6 > x0) tft.fillRect(max(x, x0), FO_GROUND_Y, min(x + 6, x0 + w) - max(x, x0), 1, UI_LINE);
}

// สัญลักษณ์มุมขวาบนของกรอบ: || เหลือง = หยุดชั่วคราว, ! แดง = ไม่ไหล
void foDrawBadge(int8_t mode) {
  int bx = FO_FR_X + FO_FR_W - 16, by = FO_FR_Y + 10;
  tft.fillRect(bx, by, 14, 16, THEME_BG);
  if (mode == 2) {
    tft.fillRect(bx + 2, by + 2, 3, 12, COLOR_YELLOW);
    tft.fillRect(bx + 8, by + 2, 3, 12, COLOR_YELLOW);
  } else if (mode == 3) {
    textAt("!", bx + 2, by, 2, COLOR_RED, THEME_BG);
  }
}

// ไอคอนหยดน้ำเล็ก ๆ (หัวแหลม ก้นกลม)
void foDropIcon(int x, int y, uint16_t c) {
  tft.drawFastVLine(x + 3, y, 2, c);
  tft.drawFastHLine(x + 2, y + 2, 3, c);
  tft.drawFastHLine(x + 1, y + 3, 5, c);
  tft.fillCircle(x + 3, y + 6, 3, c);
}

// ไอคอนถุงน้ำเกลือเล็ก ๆ (ถุงกับคอถุง)
void foBagIcon(int x, int y, uint16_t c) {
  tft.fillRoundRect(x, y, 8, 9, 2, c);
  tft.fillRect(x + 3, y + 9, 2, 3, c);
}

// โหมดของภาพ 0 = เดิน/ยืน, 2 = หยุดชั่วคราว, 3 = ไม่มีการไหล
int8_t foModeNow() {
  if (!isRunning) return 2;
  // การ์ดสถานะขึ้น NO FLOW ได้ทั้งจากเครื่องนี้เองและจาก Host ภาพต้องตรงกับการ์ดเสมอ
  return (isLocallyOccluded() || uiStatusOf(uiAlertCode) == UI_NOFLOW) ? 3 : 0;
}

// ปริมาตรที่เหลือในกล่องไอคอนถุง (แทนระดับน้ำในภาพถุงของสายหลัก)
void drawBagLevel(bool force) {
  int pct = infusedPct();
  int left = (pct < 0) ? -1 : (100 - pct);
  uint16_t fill = (uiAlertCode == ALERT_NEAR_END) ? COLOR_ORANGE : COLOR_WHITE;
  int key = left * 2 + ((uiAlertCode == ALERT_NEAR_END) ? 1 : 0);
  if (!force && key == cacheBagPct) return;
  cacheBagPct = key;
  int bx = FO_FR_X + 41;
  tft.fillRect(bx, FO_ICON_Y, 39, 22, UI_CARD);
  foBagIcon(bx + 4, FO_ICON_Y + 5, fill);
  textRight(left < 0 ? String("--") : (String(left) + "%"), bx + 37, FO_ICON_Y + 8, 1, fill, UI_CARD);
}

// ---- ส่วนที่ 1: กำลังหยดอยู่ไหม ----
// รอหยดถัดไปได้นานสุด 2.5 เท่าของช่วงห่างระหว่างหยดขณะนี้ (อย่างน้อย FO_IDLE_MIN_MS)
// แต่ไม่เกินเกณฑ์ NO FLOW ภาพจึงหยุดก่อนหรือพร้อมกับที่การ์ดขึ้น NO FLOW เสมอ
unsigned long foIdleMs() {
  unsigned long lim = occlusionThresholdMs();
  if (currentGttMin <= 0.0f) return lim;
  unsigned long ms = (unsigned long)(2.5f * 60000.0f / currentGttMin);
  if (ms < FO_IDLE_MIN_MS) ms = FO_IDLE_MIN_MS;
  return (ms < lim) ? ms : lim;
}

bool foDripping(unsigned long now) {
  return isRunning && hasFirstDropOccurred && (now - lastDropTimestamp) <= foIdleMs();
}

// คำตัดสินสุดท้ายว่าภาพเดินหรือหยุด ใช้ทั้งภาพเดินและปุ่ม WALKING / STOPPED ให้ตรงกันเสมอ
// หยุดชั่วคราวและ NO FLOW (รวมที่ Host แจ้ง) หยุดเดินแม้จะเพิ่งมีหยด
bool foShouldWalk(unsigned long now) {
  return foModeNow() == 0 && foDripping(now);
}

// ---- ส่วนที่ 2: ภาพเดิน เล่นวนตามเวลาของตัวเอง ----
void updateDripAnimation() {
  if (currentState != STATE_NORMAL_VIEW || currentNursePage != 1 || stateNeedsRedraw) return;

  unsigned long now = millis();
  int8_t mode = foModeNow();
  isDropFalling = false;      // หน้านี้ไม่ใช้จังหวะของหยดแต่ละหยดแล้ว ล้างทิ้งไว้เฉย ๆ

  if (mode != animModeDrawn) {
    animModeDrawn = mode;
    foDrawFrameMarks(mode == 3 ? COLOR_RED : COLOR_GREEN);
    foDrawBadge(mode);
    foFrameDrawn = -1;
  }

  bool walk = foShouldWalk(now);
  uint8_t f;
  if (walk) {
    if (!foWalking || now - foFrameStart > 2000UL) foFrameStart = now;   // เริ่มใหม่ หรือกลับมาจากหน้าอื่น
    foWalking = true;
    bool moved = false;
    while (now - foFrameStart >= FO_WALK_MS[foSeqPos]) {
      foFrameStart += FO_WALK_MS[foSeqPos];
      foSeqPos = (foSeqPos + 1) % FO_WALK_LEN;
      foGroundPhase++;
      moved = true;
    }
    if (moved) foDrawGround();
    f = FO_WALK_SEQ[foSeqPos];
  } else {
    foWalking = false;
    // หยดหยุด = ค้างท่าปัจจุบัน, หยุดชั่วคราว / ไม่ไหล = ท่ายืน
    f = (mode != 0) ? FO_SPR_STAND : FO_WALK_SEQ[foSeqPos];
  }
  if ((int8_t)f == foFrameDrawn) return;
  foFrameDrawn = f;
  lastAnimFrame = now;
  foDrawSprite(f);
}

void drawPage1Framework() {
  tft.fillScreen(THEME_BG);
  drawTopBar(true);

  // แถบแท็บแบบ Pip-Boy: เส้นยาวใต้แท็บ แท็บที่เปิดอยู่มีขายกขึ้นสองข้างและไม่มีเส้นใต้
  static const char *tabs[] = {"STAT", "PLAN", "LINK", "TREND"};
  int tx = 12;
  for (int i = 0; i < 4; i++) {
    int w = strlen(tabs[i]) * 6;
    if (i == 0) {
      tft.drawFastHLine(2, 40, tx - 5, COLOR_GREEN);
      tft.drawFastVLine(tx - 4, 30, 11, COLOR_GREEN);
      tft.drawFastVLine(tx + w + 2, 30, 11, COLOR_GREEN);
      tft.drawFastHLine(tx - 4, 30, 3, COLOR_GREEN);
      tft.drawFastHLine(tx + w, 30, 3, COLOR_GREEN);
      tft.drawFastHLine(tx + w + 2, 40, 170 - (tx + w + 2), COLOR_GREEN);
    }
    textAt(tabs[i], tx, 32, 1, i == 0 ? COLOR_WHITE : UI_DIM, THEME_BG);
    tx += w + 14;
  }

  animModeDrawn = foModeNow();
  foDrawFrameMarks(animModeDrawn == 3 ? COLOR_RED : COLOR_GREEN);
  foDrawBadge(animModeDrawn);
  foFrameDrawn = (animModeDrawn != 0) ? FO_SPR_STAND : FO_WALK_SEQ[foSeqPos];
  foWalking = false;
  foDrawSprite(foFrameDrawn);
  foDrawGround();

  // กล่องไอคอนสองกล่องใต้ภาพ (แบบแถวไอคอนอาวุธ/ชุดเกราะของหน้า STAT)
  tft.fillRect(FO_FR_X, FO_ICON_Y, 39, 22, UI_CARD);
  foDropIcon(FO_FR_X + 4, FO_ICON_Y + 5, COLOR_WHITE);
  textAt("gtt/m", FO_FR_X + 2, FO_ICON_Y + 26, 1, UI_DIM, THEME_BG);
  textAt("left", FO_FR_X + 43, FO_ICON_Y + 26, 1, UI_DIM, THEME_BG);

  textAt("RATE", RIGHT_X + 4, 48, 1, UI_DIM, THEME_BG);
  textAt("mL/h", RIGHT_X + 4, 96, 1, UI_DIM, THEME_BG);
  tft.drawFastHLine(RIGHT_X + 2, 122, 84, UI_LINE);
  textAt("LEFT", RIGHT_X + 4, 128, 1, UI_DIM, THEME_BG);
  tft.drawFastHLine(RIGHT_X + 2, 162, 84, UI_LINE);
  textAt("TIME LEFT", RIGHT_X + 4, 168, 1, UI_DIM, THEME_BG);

  cacheBagPct = -999; cacheRate = -9999; cacheTarget = -9999; cacheLeftMl = -9999;
  cachePct = -999; cacheTimeLeft = ""; cacheEndClock = ""; cacheStatusWord = "";
  cacheFoDrops = ""; cacheFoGtt = ""; cacheFoWalk = "";
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
    tft.fillRect(RIGHT_X + 4, 60, 84, 32, THEME_BG);
    if (!isRunning) textAt("--", RIGHT_X + 4, 60, 4, THEME_SKYBLUE, THEME_BG);
    else textAt(String(rate), RIGHT_X + 4, 60, 4, (st == UI_NOFLOW) ? COLOR_RED : COLOR_WHITE, THEME_BG);
    textAt(padTo(targetRateHr > 0 ? ("SET " + String(target)) : String("SET --"), 9),
           RIGHT_X + 4, 108, 1, THEME_SKYBLUE, THEME_BG);
  }

  int left = (totalPlanMl > 0) ? remainingMlInt() : -1;
  if (left != cacheLeftMl) {
    cacheLeftMl = left;
    textAt(padTo(left >= 0 ? (String(left) + " mL") : String("-- mL"), 7), RIGHT_X + 4, 140, 2, COLOR_WHITE, THEME_BG);
  }

  String tl = timeLeftText(true);
  if (tl != cacheTimeLeft) {
    cacheTimeLeft = tl;
    textAt(padTo(tl, 7), RIGHT_X + 4, 180, 2, COLOR_WHITE, THEME_BG);
  }

  String ec = endClockText();
  if (ec != cacheEndClock) {
    cacheEndClock = ec;
    textAt(padTo("END " + ec, 11), RIGHT_X + 4, 202, 1, UI_DIM, THEME_BG);
  }

  // การ์ดสถานะเป็นแถบทึบแบบรายการที่ถูกเลือกของ Pip-Boy (มุมเหลี่ยม ตัวอักษรสีพื้น)
  String word = String(uiStatusWord(st));
  if (word != cacheStatusWord) {
    cacheStatusWord = word;
    uint16_t sc = uiStatusColor(st);
    tft.fillRect(RIGHT_X + 2, 220, 84, 34, sc);
    textCenterIn(word, RIGHT_X + 2, 84, 233, 1, THEME_BG, sc);
  }

  // อัตราหยดในกล่องไอคอนหยด
  String gtt = isRunning ? String((int)(currentGttMin + 0.5f)) : String("--");
  if (gtt != cacheFoGtt) {
    cacheFoGtt = gtt;
    tft.fillRect(FO_FR_X + 12, FO_ICON_Y + 8, 26, 8, UI_CARD);
    textRight(gtt, FO_FR_X + 37, FO_ICON_Y + 8, 1, COLOR_WHITE, UI_CARD);
  }

  // ปุ่มแบบ STIMPAK/RADAWAY: สถานะการหยด (ตัวเดียวกับที่สั่งภาพเดิน) กับตัวนับหยดสะสม
  // [1.3.0-FO] แก้: ป้ายเดิม "1 STEP=1 DROP" ไม่จริงแล้ว เปลี่ยนเป็นสถานะ WALKING / STOPPED
  String walkWord = foShouldWalk(millis()) ? "WALKING" : "STOPPED";
  if (walkWord != cacheFoWalk) {
    cacheFoWalk = walkWord;
    tft.fillRect(FO_FR_X, FO_BTN_Y, 80, 14, UI_CARD);
    textCenterIn(walkWord, FO_FR_X, 80, FO_BTN_Y + 3, 1, walkWord == "WALKING" ? COLOR_WHITE : UI_DIM, UI_CARD);
  }
  String drops = "DROPS " + String(totalDrops);
  if (drops != cacheFoDrops) {
    cacheFoDrops = drops;
    tft.fillRect(RIGHT_X + 2, FO_BTN_Y, 84, 14, UI_CARD);
    textCenterIn(drops, RIGHT_X + 2, 84, FO_BTN_Y + 3, 1, COLOR_WHITE, UI_CARD);
  }

  // แถบล่างแบบ HP / LEVEL / AP: ปริมาตรที่ให้ไปแล้ว | แถบความคืบหน้า | เปอร์เซ็นต์
  int pct = infusedPct();
  if (pct != cachePct) {
    cachePct = pct;
    uint16_t barC = (uiAlertCode == ALERT_NEAR_END) ? COLOR_ORANGE : COLOR_GREEN;
    tft.fillRect(2, FO_HUD_Y, 80, 16, UI_CARD);
    String inf = (totalPlanMl > 0)
      ? (String((int)totalVolumeMl) + "/" + String((int)totalPlanMl))
      : (String((int)totalVolumeMl) + " mL");
    textAt("IN", 5, FO_HUD_Y + 4, 1, UI_DIM, UI_CARD);
    textRight(inf, 80, FO_HUD_Y + 4, 1, COLOR_WHITE, UI_CARD);
    tft.fillRect(84, FO_HUD_Y, 54, 16, UI_CARD);
    tft.drawRect(88, FO_HUD_Y + 5, 46, 6, barC);
    int fw = (pct < 0) ? 0 : 44 * min(pct, 100) / 100;
    if (fw > 0) tft.fillRect(89, FO_HUD_Y + 6, fw, 4, barC);
    tft.fillRect(140, FO_HUD_Y, 30, 16, UI_CARD);
    textRight(pct >= 0 ? (String(pct) + "%") : String("--"), 168, FO_HUD_Y + 4, 1, COLOR_WHITE, UI_CARD);
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
