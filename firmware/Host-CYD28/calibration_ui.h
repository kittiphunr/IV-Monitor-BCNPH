/**
 * @file      calibration_ui.h
 * @brief     เก็บ/โหลดค่าคาลิเบรตทัชใน NVS และหน้าคาลิเบรตสามจุด
 * @version   2.0.1-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * ผูกค่าคาลิเบรตกับทิศจอ ขนาดจอ และชิปจอ ถ้าไม่ตรงถือว่าไม่มีค่า หน้าคาลิเบรตวนจนกว่าจุดทั้งสามจะแก้สมการได้
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
#include <Preferences.h>
#include <math.h>
#include "board.h"
#include "display.h"
#include "colors.h"
#include "touch_cal.h"
#include "touch_xpt2046.h"

static const uint32_t CAL_MAGIC = 0x43414C32;      // "CAL2"

struct StoredCal {
  uint32_t magic;
  uint8_t  rotation;
  uint8_t  panel;        // 0 = ILI9341, 1 = ST7789
  uint16_t w, h;
  TouchCal k;
};


XptBitbang touchChip(cyd::TOUCH_CLK, cyd::TOUCH_MOSI, cyd::TOUCH_MISO, cyd::TOUCH_CS, cyd::TOUCH_IRQ);
TouchCal cal;
static Preferences calPrefs;

static uint16_t calPanelId() { return (CYD_PANEL == 7789) ? 1 : 0; }

static bool loadCal() {
  StoredCal s;
  calPrefs.begin("cyd", true);
  size_t n = calPrefs.getBytesLength("cal");
  bool ok = (n == sizeof(s)) && calPrefs.getBytes("cal", &s, sizeof(s)) == sizeof(s);
  calPrefs.end();
  if (!ok || s.magic != CAL_MAGIC || s.rotation != g_rotation || s.panel != calPanelId()
      || s.w != tft.width() || s.h != tft.height() || !s.k.valid) return false;
  cal = s.k;
  return true;
}

static void saveCal() {
  StoredCal s = {CAL_MAGIC, g_rotation, (uint8_t)calPanelId(), (uint16_t)tft.width(), (uint16_t)tft.height(), cal};
  calPrefs.begin("cyd", false);
  calPrefs.putBytes("cal", &s, sizeof(s));
  calPrefs.end();
}


static bool readTouch(int16_t &rx, int16_t &ry, int16_t &z) { return touchChip.read(rx, ry, z); }

static void waitRelease() {
  uint32_t quiet = millis();
  int16_t a, b, c;
  while (millis() - quiet < 120) {
    if (readTouch(a, b, c)) quiet = millis();
    delay(5);
  }
}

// เก็บตัวอย่างค่าดิบที่นิ่ง (เบี่ยงจากค่าเฉลี่ยไม่เกิน 150) ครบ 12 ตัวอย่าง
static void captureStable(float &ox, float &oy) {
  for (;;) {
    int16_t rx, ry, z;
    if (!readTouch(rx, ry, z)) { delay(5); continue; }
    float sx = rx, sy = ry;
    int n = 1;
    uint32_t t0 = millis();
    while (n < 12 && millis() - t0 < 1500) {
      if (!readTouch(rx, ry, z)) { delay(5); continue; }
      const float mx = sx / n, my = sy / n;
      if (fabsf(rx - mx) > 150 || fabsf(ry - my) > 150) { n = 0; sx = sy = 0; }
      sx += rx; sy += ry; n++;
      delay(8);
    }
    if (n >= 12) { ox = sx / n; oy = sy / n; return; }
  }
}

static void drawCross(int x, int y, uint16_t col) {
  tft.drawFastHLine(x - 12, y, 25, col);
  tft.drawFastVLine(x, y - 12, 25, col);
  tft.drawCircle(x, y, 6, col);
}

static void runCalibration() {
  const int w = tft.width(), h = tft.height();
  for (;;) {
    CalPoint p[3];
    touchCalTargets(w, h, p);
    for (int i = 0; i < 3; i++) {
      tft.fillScreen(C_BLACK);
      tft.setTextSize(2);
      tft.setTextColor(C_WHITE, C_BLACK);
      tft.setCursor(8, h / 2 - 8);
      tft.print("TOUCH THE CROSS");
      char s[16];
      snprintf(s, sizeof(s), "%d / 3", i + 1);
      tft.setCursor(8, h / 2 + 12);
      tft.print(s);
      drawCross((int)p[i].sx, (int)p[i].sy, C_GREEN);
      waitRelease();
      captureStable(p[i].rx, p[i].ry);
      Serial.print("cal point ");
      Serial.print(String(i + 1));
      Serial.print(" raw ");
      Serial.print(String((int)p[i].rx));
      Serial.print(",");
      Serial.println(String((int)p[i].ry));
      drawCross((int)p[i].sx, (int)p[i].sy, C_RED);
      waitRelease();
    }
    TouchCal k;
    if (touchCalSolve(p, w, h, k)) { cal = k; saveCal(); Serial.println("cal OK, saved"); return; }
    tft.fillScreen(C_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(C_RED, C_BLACK);
    tft.setCursor(8, h / 2 - 8);
    tft.print("BAD POINTS, RETRY");
    Serial.println("cal FAILED (points collinear or out of range), retry");
    delay(1500);
  }
}

