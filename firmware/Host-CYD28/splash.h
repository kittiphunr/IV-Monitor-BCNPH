/**
 * @file      splash.h
 * @brief     หน้าเปิดเครื่อง: โลโก้บนพื้นดำ + แถบโหลดสไตล์ macOS (เส้นบางสีขาวค่อย ๆ เต็ม) แตะเพื่อข้ามได้
 * @version   2.5.0-CYD
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
 * แถบโหลดแบบ macOS: รางเทาเข้มบางปลายมน ส่วนสีขาวค่อย ๆ ยาวจากซ้ายไปขวา เต็มพอดีเมื่อครบ CYD_SPLASH_MS และเริ่มระบบเสร็จ (หยุดรอที่ 90% ถ้าเริ่มระบบยังไม่เสร็จ) มีข้อความขั้นตอนใต้แถบ
 * ความยาวแถบเป็นความคืบหน้าตามเวลา (ไม่ใช่เปอร์เซ็นต์จริง) แต่จะไม่เต็มก่อนเริ่มระบบเสร็จ ข้อความใต้แถบบอกขั้นตอนที่ทำอยู่จริง
 * สร้างโลโก้ใหม่ด้วย tools/logo-convert/make_logo.py (ดูหัวไฟล์ logo.h) ใช้พื้นหลังสีดำให้กลืนกับพื้นจอ
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.5.0-CYD | 2026-11-27 | แถบหยุดที่ 90% ระหว่างเริ่มระบบแล้วเต็มเมื่อเสร็จ (ไม่เต็มค้าง) · ข้อความขั้นตอนใต้แถบ (WI-FI/CLOCK/SD CARD/WEB SERVER) · ไฟหลังจอค่อย ๆ สว่าง |
 * | 2.3.2-CYD | 2026-11-27 | เปลี่ยนแถบโหลดจาก Windows XP เป็นสไตล์ macOS ใต้โลโก้ (ความคืบหน้าตามเวลา) |
 * | 2.0.5-CYD | 2026-11-27 | เพิ่มหน้าเปิดเครื่องแบบโลโก้บนพื้นดำ พร้อมแถบโหลดสไตล์ Windows XP |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <string.h>
#include "board.h"
#include "display.h"
#include "colors.h"
#if __has_include("logo.h")
  #include "logo.h"
#else
  #define LOGO_PRESENT 0
#endif

static uint32_t splashStartMs = 0;

// ---- แถบโหลดสไตล์ macOS (เส้นบางมนสีขาวค่อย ๆ เต็มบนรางเทาเข้ม) ----------------------------
// [2.3.2-CYD] แก้: แทนแถบ Windows XP (บล็อกน้ำเงินวิ่งวน) ด้วยแถบความคืบหน้าแบบ macOS ใต้โลโก้
constexpr int SPL_TRACK_W = 160, SPL_TRACK_H = 6;         // รางนอก
constexpr int SPL_RADIUS = 3;                             // ปลายมนครึ่งวงกลม
constexpr uint16_t SPL_C_TRACK = 0x4208;                  // เทาเข้ม (#404040)
constexpr uint16_t SPL_C_FILL = 0xFFFF;                   // ขาว

// ตำแหน่งมุมซ้ายบนของราง (กึ่งกลางแนวนอน ชิดล่างของจอ)
static int splashTrackX() { return (tft.width() - SPL_TRACK_W) / 2; }
static int splashTrackY() { return tft.height() - SPL_TRACK_H - 22; }

// ความกว้างส่วนที่เต็ม (พิกเซล 0..SPL_TRACK_W) ณ เวลา ms นับจากเริ่มวาดโลโก้ เต็มพอดีเมื่อครบ CYD_SPLASH_MS
// เป็นความคืบหน้าตามเวลาที่ตั้งไว้ ไม่ใช่ความคืบหน้าจริงของการเริ่มระบบ (ตรรกะล้วน ทดสอบบน PC ได้)
// [2.5.0-CYD] แก้: ระหว่างเริ่มระบบยังไม่เสร็จ แถบหยุดที่ 90% (SPL_CAP_PCT) ไม่เต็มค้าง พอ splashFinish() จึงไปต่อจนเต็มตามเวลา
static bool splashFinished = false;
constexpr int SPL_CAP_PCT = 90;
static int splashFillPx(uint32_t ms) {
#if CYD_SPLASH_MS > 0
  int px = ms >= (uint32_t)CYD_SPLASH_MS ? SPL_TRACK_W : (int)((uint64_t)ms * SPL_TRACK_W / (uint32_t)CYD_SPLASH_MS);
  const int cap = SPL_TRACK_W * SPL_CAP_PCT / 100;
  if (!splashFinished && px > cap) px = cap;
  return px;
#else
  (void)ms; return 0;
#endif
}

static void splashDrawTrack(int fillPx) {
  const int x = splashTrackX(), y = splashTrackY();
  tft.fillRoundRect(x, y, SPL_TRACK_W, SPL_TRACK_H, SPL_RADIUS, SPL_C_TRACK);
  if (fillPx <= 0) return;
  const int r = fillPx >= 2 * SPL_RADIUS ? SPL_RADIUS : fillPx / 2;   // ส่วนเต็มแคบกว่าปลายมน ลดรัศมีลง
  tft.fillRoundRect(x, y, fillPx, SPL_TRACK_H, r, SPL_C_FILL);
}

static int splashLastFill = -1;
static char splashLabel[20] = "";
static char splashLabelShown[20] = "";

// [2.5.0-CYD] เพิ่ม: ข้อความขั้นตอนปัจจุบันใต้แถบ (ขนาดเล็ก เทา) วาดใหม่เฉพาะเมื่อข้อความเปลี่ยน
static void splashDrawLabel() {
#if CYD_SPLASH_MS > 0
  if (!strcmp(splashLabel, splashLabelShown)) return;
  strcpy(splashLabelShown, splashLabel);
  const int y = splashTrackY() + SPL_TRACK_H + 6;
  tft.fillRect(0, y, tft.width(), 8, C_BLACK);
  const int w = (int)strlen(splashLabel) * 6;
  tft.setTextSize(1); tft.setTextColor(0x8410, C_BLACK);
  tft.setCursor((tft.width() - w) / 2, y);
  tft.print(splashLabel);
#endif
}

// ขยับแถบโหลด เรียกบ่อยแค่ไหนก็ได้ (วาดจริงเมื่อส่วนเต็มยาวขึ้นอย่างน้อย 1 พิกเซลเท่านั้น)
static void splashTick() {
#if CYD_SPLASH_MS > 0
  if (splashStartMs == 0) return;
  const int fill = splashFillPx(millis() - splashStartMs);
  if (fill == splashLastFill) return;
  splashLastFill = fill;
  splashDrawTrack(fill);
  splashDrawLabel();
#endif
}

// บอกขั้นตอนที่กำลังเริ่ม (ข้อความไม่เกิน 19 ตัว) แล้วขยับแถบ เรียกก่อนขั้นตอนที่ช้า เช่น SD
static void splashStage(const char *label) {
#if CYD_SPLASH_MS > 0
  strncpy(splashLabel, label, sizeof(splashLabel) - 1);
  splashLabel[sizeof(splashLabel) - 1] = 0;
  splashDrawLabel();
  splashTick();
#else
  (void)label;
#endif
}

// เริ่มระบบเสร็จแล้ว ปลดเพดาน 90% (ส่วนที่เหลือเต็มตามเวลา หรือแตะข้าม)
static void splashFinish() {
#if CYD_SPLASH_MS > 0
  splashFinished = true;
  splashStage("READY");
#endif
}

// เปิดไฟหลังจอค่อย ๆ สว่างขึ้น (ราว 0.25 วินาที) แทนการวาบ เรียกครั้งเดียวหลังวาดโลโก้ ปิดหน้านี้ = ตั้งทันที
static void splashFadeIn(int targetPct) {
#if CYD_SPLASH_MS > 0
  for (int i = 1; i <= 10; i++) { backlightSet(targetPct * i / 10); delay(25); }
#else
  backlightSet(targetPct);
#endif
}

// วาดหน้าเปิดเครื่อง (เรียกหลัง panelBegin)
static void splashDraw() {
#if CYD_SPLASH_MS > 0
  splashStartMs = millis();
  if (splashStartMs == 0) splashStartMs = 1;
  splashLastFill = -1;
  splashFinished = false;
  splashLabel[0] = 0; splashLabelShown[0] = 0;
  tft.fillScreen(C_BLACK);
  #if LOGO_PRESENT
    const int x = (tft.width() - LOGO_W) / 2, y = 6;
    tft.drawRGBBitmap(x, y, LOGO_DATA, LOGO_W, LOGO_H);
  #else
    tft.setTextSize(3); tft.setTextColor(C_WHITE, C_BLACK);
    tft.setCursor((tft.width() - 6 * 3 * 8) / 2, 70); tft.print("SMART IV");
    tft.setCursor((tft.width() - 6 * 3 * 5) / 2, 100); tft.print("ALERT");
  #endif
  splashTick();
#endif
}

// รอให้ครบเวลา (เรียกท้าย setup) ขยับแถบโหลดระหว่างรอ แตะจอเพื่อข้าม
// ฟังก์ชันอ่านทัชส่งเข้ามาเพื่อให้ไฟล์นี้ไม่ผูกกับไดรเวอร์ทัช
template <typename TouchFn>
static void splashWait(TouchFn touched) {
#if CYD_SPLASH_MS > 0
  while ((uint32_t)(millis() - splashStartMs) < (uint32_t)CYD_SPLASH_MS) {
    if (touched()) break;
    splashTick();
    delay(10);
  }
#else
  (void)touched;
#endif
}
