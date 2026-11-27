/**
 * @file      diag_screen.h
 * @brief     หน้าทดสอบจอและทัช (แถบสี ไล่เฉดเทา ช่องวาด ปุ่ม CAL/CLR/-/+)
 * @version   2.0.1-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * ใช้หาค่า CYD_INVERT / CYD_BGR และตรวจความแม่นของทัช เข้าด้วยคำสั่ง Serial m หรือกด BOOT สั้น ๆ
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.1-CYD | 2026-11-27 | แยกออกจาก Host-CYD28.ino เมื่อเริ่มด่านที่ 2 เพื่อให้ไฟล์หลักเหลือแต่แกนเครือข่าย (โค้ดเดิมจากด่านที่ 1 ไม่เปลี่ยนพฤติกรรม) |
 * | 2.0.0-CYD | 2026-11-27 | เขียนใหม่ทั้งหมดด่านที่ 1 (ฐานบอร์ด) |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include "board.h"
#include "display.h"
#include "colors.h"
#include "touch_cal.h"
#include "touch_xpt2046.h"
#include "calibration_ui.h"

struct Button { int16_t x, y, w, h; const char *label; };
TouchTracker tracker;
int brightness = 100;
int16_t lastX = 0, lastY = 0, lastRx = 0, lastRy = 0, lastZ = 0;

static void printSettings() {
  Serial.print("panel=");   Serial.print(String(CYD_PANEL));
  Serial.print(" rot=");    Serial.print(String((int)g_rotation));
  Serial.print(" invert="); Serial.print(String((int)g_invert));
  Serial.print(" bgr=");    Serial.print(String((int)g_bgr));
  Serial.print(" bright="); Serial.println(String(brightness));
}

// ---- วาดหน้าทดสอบ ----------------------------------------------------------------------------
static Button buttons[4];
static int16_t paintTop, paintBottom;

static void layoutTest() {
  const int w = tft.width(), h = tft.height();
  const int barH = 36, rampH = 14, btnH = 34;
  paintTop = barH + rampH;
  paintBottom = h - btnH - 2;
  const char *labels[4] = {"CAL", "CLR", "-", "+"};
  const int bw = (w - 5 * 4) / 4;
  for (int i = 0; i < 4; i++) buttons[i] = { (int16_t)(4 + i * (bw + 4)), (int16_t)(h - btnH), (int16_t)bw, (int16_t)(btnH - 2), labels[i] };
}

static void drawButton(const Button &b) {
  tft.fillRect(b.x, b.y, b.w, b.h, 0x2104);
  tft.drawRect(b.x, b.y, b.w, b.h, C_GRAY);
  tft.setTextSize(2);
  tft.setTextColor(C_WHITE, 0x2104);
  tft.setCursor(b.x + b.w / 2 - (int)strlen(b.label) * 6, b.y + 9);
  tft.print(b.label);
}

static void drawStatus() {
  char s[48];
  snprintf(s, sizeof(s), "%3d,%3d raw %4d,%4d z%4d  ", lastX, lastY, lastRx, lastRy, lastZ);
  tft.setTextSize(1);
  tft.setTextColor(C_WHITE, C_BLACK);
  tft.setCursor(4, paintBottom - 10);
  tft.print(s);
}

static void drawTestScreen() {
  const int w = tft.width();
  layoutTest();
  tft.fillScreen(C_BLACK);
  const uint16_t bars[8] = {C_WHITE, C_YELLOW, C_CYAN, C_GREEN, C_MAGENTA, C_RED, C_BLUE, C_BLACK};
  for (int i = 0; i < 8; i++) tft.fillRect(i * w / 8, 0, w / 8 + 1, 36, bars[i]);
  for (int i = 0; i < 16; i++) {                      // ไล่เฉดเทา 16 ระดับ ดูว่าดำสนิทหรือซีด
    uint16_t v = (uint16_t)(i * 2);
    tft.fillRect(i * w / 16, 36, w / 16 + 1, 14, (uint16_t)((v << 11) | ((v * 2) << 5) | v));
  }
  tft.drawRect(0, paintTop, w, paintBottom - paintTop, C_GRAY);
  for (int i = 0; i < 4; i++) drawButton(buttons[i]);
  drawStatus();
}

static void applyBrightness(int delta) {
  brightness += delta;
  if (brightness < 10) brightness = 10;
  if (brightness > 100) brightness = 100;
  backlightSet(brightness);
  printSettings();
}


static int hitButton(int x, int y) {
  for (int i = 0; i < 4; i++)
    if (x >= buttons[i].x && x < buttons[i].x + buttons[i].w && y >= buttons[i].y && y < buttons[i].y + buttons[i].h) return i;
  return -1;
}

static void onButton(int i) {
  if (i == 0) { runCalibration(); drawTestScreen(); }
  else if (i == 1) drawTestScreen();
  else if (i == 2) applyBrightness(-10);
  else if (i == 3) applyBrightness(+10);
}

// เรียกทุกรอบของ loop() ขณะอยู่หน้าทดสอบ: อ่านทัช วาดตามนิ้ว และกดปุ่ม
static void diagStep() {
  int16_t rx = 0, ry = 0, z = 0;
  const bool hit = touchChip.read(rx, ry, z);
  const TouchEvt ev = tracker.update(hit, millis());
  if (hit) {
    lastRx = rx; lastRy = ry; lastZ = z;
    touchCalApply(cal, rx, ry, tft.width(), tft.height(), lastX, lastY);
  }
  if (ev == TOUCH_DOWN) {
    const int b = hitButton(lastX, lastY);
    if (b >= 0) onButton(b);
  }
  if (hit && tracker.down() && lastY > paintTop + 2 && lastY < paintBottom - 12) tft.fillCircle(lastX, lastY, 2, C_YELLOW);
  if (ev == TOUCH_DOWN || ev == TOUCH_UP) drawStatus();
}
