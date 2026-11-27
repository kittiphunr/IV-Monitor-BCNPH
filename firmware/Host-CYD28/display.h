/**
 * @file      display.h
 * @brief     เริ่มจอ ไฟหลังจอ PWM และสวิตช์สีขณะรัน (CYD)
 * @version   2.0.0-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * เลือกชิป ILI9341 หรือ ST7789 ตอนคอมไพล์ด้วย CYD_PANEL เริ่มบัส HSPI ตั้งทิศ ลำดับสี และโหมดกลับสี มีฟังก์ชันสลับสีและ BGR ขณะรัน (ผ่าน Serial) ไว้หาค่าที่ถูกกับบอร์ดจริงโดยไม่ต้องคอมไพล์ใหม่ และอ่านรหัสชิปของ ILI9341 ให้ดู
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.0-CYD | 2026-11-27 | เขียนใหม่ทั้งหมดด่านที่ 1 (ฐานบอร์ด: จอ ไฟหลังจอ ทัช) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <SPI.h>
#include "board.h"
#if CYD_PANEL == 7789
  #include <Adafruit_ST7789.h>
#else
  #include <Adafruit_ILI9341.h>
#endif

SPIClass spiTft(HSPI);
#if CYD_PANEL == 7789
Adafruit_ST7789 tft(&spiTft, cyd::TFT_CS, cyd::TFT_DC, -1);        // ST7789: CS มาก่อน DC
#else
Adafruit_ILI9341 tft(&spiTft, cyd::TFT_DC, cyd::TFT_CS, -1);       // ILI9341: DC มาก่อน CS
#endif

static bool g_invert = CYD_INVERT;
static bool g_bgr = CYD_BGR;
static uint8_t g_rotation = CYD_ROTATION & 3;

// MADCTL ของ ST7789 ตามตารางหมุนจอของ Adafruit: 0=MX|MY 1=MY|MV 2=ไม่มี 3=MX|MV (บิต 3 = BGR)
static void panelApplyMadctl() {
#if CYD_PANEL == 7789
  static const uint8_t base[4] = { 0xC0, 0xA0, 0x00, 0x60 };
  uint8_t m = base[g_rotation] | (g_bgr ? 0x08 : 0x00);
  tft.sendCommand(0x36, &m, 1);
#endif
}

static void backlightBegin() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(cyd::TFT_BL, 5000, 8);
#else
  ledcSetup(0, 5000, 8);
  ledcAttachPin(cyd::TFT_BL, 0);
#endif
}

static void backlightSet(int pct) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(cyd::TFT_BL, (pct * 255) / 100);
#else
  ledcWrite(0, (pct * 255) / 100);
#endif
}

static void panelSetInvert(bool on) { g_invert = on; tft.invertDisplay(on); }
static void panelSetBgr(bool on) { g_bgr = on; panelApplyMadctl(); }
static void panelSetRotation(uint8_t r) { g_rotation = r & 3; tft.setRotation(g_rotation); panelApplyMadctl(); }

static void panelBegin() {
  backlightBegin();
  backlightSet(0);                                         // ปิดไฟไว้ก่อน กันภาพขยะตอนเริ่มจอ
  spiTft.begin(cyd::TFT_SCK, cyd::TFT_MISO, cyd::TFT_MOSI, cyd::TFT_CS);
#if CYD_PANEL == 7789
  tft.init(cyd::PANEL_W, cyd::PANEL_H);
  tft.setSPISpeed(CYD_SPI_HZ);
#else
  tft.begin(CYD_SPI_HZ);
#endif
  tft.invertDisplay(g_invert);
  panelSetRotation(g_rotation);
  tft.setTextWrap(false);
  tft.fillScreen(0x0000);
  backlightSet(100);
}

// ILI9341 ตอบ ID4 (คำสั่ง 0xD3) เป็น 00 93 41 ถ้าได้ค่าอื่นแปลว่าไม่ใช่ ILI9341 หรือสาย MISO ไม่ถึง
static void panelPrintId() {
#if CYD_PANEL == 9341
  Serial.print("ILI9341 ID4: ");
  for (uint8_t i = 1; i <= 3; i++) {
    char b[8];
    snprintf(b, sizeof(b), "%02X ", tft.readcommand8(0xD3, i));
    Serial.print(b);
  }
  Serial.println();
#endif
}
