/**
 * @file      splash.h
 * @brief     หน้าเปิดเครื่อง: โลโก้ (ถ้ามี logo.h) ค้างตามเวลาที่ตั้ง แตะเพื่อข้ามได้
 * @version   2.0.5-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * วาดโลโก้ทันทีหลังเริ่มจอ (ก่อนเริ่ม Wi-Fi/ESP-NOW เพื่อให้เห็นภาพเร็ว) แล้วเริ่มระบบต่อ จบ setup() จึงรอให้ครบ CYD_SPLASH_MS
 * นับจากตอนวาด แตะจอเพื่อข้ามได้ ระหว่างรอ ESP-NOW รับแพ็กเก็ตเข้าคิวได้ตามปกติ
 * ถ้าไม่มีภาพจริง (LOGO_PRESENT 0) แสดงชื่อระบบเป็นตัวอักษรแทน ตั้ง CYD_SPLASH_MS เป็น 0 เพื่อปิดหน้านี้
 * สร้างโลโก้ใหม่ด้วย tools/logo-convert/make_logo.py (ดูหัวไฟล์ logo.h)
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.5-CYD | 2026-11-27 | เพิ่มหน้าเปิดเครื่องแบบโลโก้ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include "board.h"
#include "display.h"
#include "colors.h"
#if __has_include("logo.h")
  #include "logo.h"
#else
  #define LOGO_PRESENT 0
#endif

static uint32_t splashStartMs = 0;

// วาดหน้าเปิดเครื่อง (เรียกหลัง panelBegin)
static void splashDraw() {
#if CYD_SPLASH_MS > 0
  splashStartMs = millis();
  #if LOGO_PRESENT
    const int x = (tft.width() - LOGO_W) / 2, y = (tft.height() - LOGO_H) / 2;
    tft.fillScreen(C_WHITE);                               // พื้นหลังโลโก้ที่แปลงไว้เป็นสีขาว (ดู --bg ตอนสร้าง logo.h)
    tft.drawRGBBitmap(x, y, LOGO_DATA, LOGO_W, LOGO_H);
  #else
    tft.fillScreen(C_BLACK);
    tft.setTextSize(3); tft.setTextColor(C_WHITE, C_BLACK);
    tft.setCursor((tft.width() - 6 * 3 * 9) / 2, tft.height() / 2 - 24); tft.print("SMART IV");
    tft.setCursor((tft.width() - 6 * 3 * 5) / 2, tft.height() / 2 + 4);  tft.print("ALERT");
  #endif
#endif
}

// รอให้ครบเวลา (เรียกท้าย setup) คืน true ถ้ารอครบหรือถูกข้าม ฟังก์ชันอ่านทัชส่งเข้ามาเพื่อให้ไฟล์นี้ไม่ผูกกับไดรเวอร์ทัช
template <typename TouchFn>
static void splashWait(TouchFn touched) {
#if CYD_SPLASH_MS > 0
  while ((uint32_t)(millis() - splashStartMs) < (uint32_t)CYD_SPLASH_MS) {
    if (touched()) break;
    delay(10);
  }
#else
  (void)touched;
#endif
}
