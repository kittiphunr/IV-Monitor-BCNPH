/**
 * @file      splash.h
 * @brief     หน้าเปิดเครื่อง: โลโก้บนพื้นดำ + แถบโหลดสไตล์ Windows XP (บล็อกสีน้ำเงินวิ่ง) แตะเพื่อข้ามได้
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
 * แถบโหลดเป็นกรอบมนมีบล็อกสีน้ำเงินสามก้อนวิ่งจากซ้ายไปขวาวนซ้ำ (splashTick) เรียกเมื่อจบแต่ละขั้นของการเริ่มระบบและระหว่างรอ
 * จึงขยับต่อเนื่องแม้ขั้นตอนเริ่มระบบใช้เวลานาน (แถบไม่ได้บอกความคืบหน้าจริง บอกแค่ว่าเครื่องยังทำงานอยู่)
 * สร้างโลโก้ใหม่ด้วย tools/logo-convert/make_logo.py (ดูหัวไฟล์ logo.h) ใช้พื้นหลังสีดำให้กลืนกับพื้นจอ
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.5-CYD | 2026-11-27 | เพิ่มหน้าเปิดเครื่องแบบโลโก้บนพื้นดำ พร้อมแถบโหลดสไตล์ Windows XP |
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

// ---- แถบโหลดสไตล์ Windows XP ----------------------------------------------------------------
constexpr int SPL_TRACK_W = 200, SPL_TRACK_H = 16;        // กรอบนอก
constexpr int SPL_BLOCK_W = 10, SPL_BLOCK_GAP = 3, SPL_BLOCKS = 3;
constexpr int SPL_STEP_MS = 70;                           // ขยับหนึ่งช่องทุกกี่ ms
constexpr int SPL_BLOCK_PITCH = SPL_BLOCK_W + SPL_BLOCK_GAP;
constexpr int SPL_INNER_W = SPL_TRACK_W - 4;
constexpr int SPL_GROUP_W = SPL_BLOCKS * SPL_BLOCK_PITCH - SPL_BLOCK_GAP;
constexpr int SPL_CYCLE = SPL_INNER_W + SPL_GROUP_W;      // ระยะที่กลุ่มบล็อกวิ่งจนพ้นขอบแล้ววนใหม่
constexpr uint16_t SPL_C_BORDER = 0xA514;                 // เทาอ่อน
constexpr uint16_t SPL_C_LIGHT = 0x76DF, SPL_C_MID = 0x2B7C, SPL_C_DARK = 0x0AF5;   // ฟ้าอ่อน/น้ำเงิน/น้ำเงินเข้มของบล็อก

// ตำแหน่งมุมซ้ายบนของกรอบ (กึ่งกลางแนวนอน ชิดล่างของจอ)
static int splashTrackX() { return (tft.width() - SPL_TRACK_W) / 2; }
static int splashTrackY() { return tft.height() - SPL_TRACK_H - 10; }

// ขอบขวาของก้อนหน้าสุด ณ เวลา ms นับจากเริ่ม (0 .. SPL_CYCLE-1 นับจากขอบในซ้าย) ตรรกะล้วน ทดสอบบน PC ได้
static int splashHeadPos(uint32_t ms) { return (int)((ms / SPL_STEP_MS) % SPL_CYCLE); }

static void splashDrawTrack(uint32_t ms) {
  const int x = splashTrackX(), y = splashTrackY();
  tft.fillRoundRect(x, y, SPL_TRACK_W, SPL_TRACK_H, 4, C_BLACK);
  tft.drawRoundRect(x, y, SPL_TRACK_W, SPL_TRACK_H, 4, SPL_C_BORDER);
  const int head = splashHeadPos(ms);                      // ขอบขวาของก้อนหน้าสุด (นับจากขอบในซ้าย)
  const int innerX = x + 2, innerY = y + 3, innerH = SPL_TRACK_H - 6;
  for (int b = 0; b < SPL_BLOCKS; b++) {
    const int right = head - b * SPL_BLOCK_PITCH;          // ขอบขวาของก้อนที่ b
    int bx = right - SPL_BLOCK_W, bw = SPL_BLOCK_W;
    if (bx < 0) { bw += bx; bx = 0; }                      // ตัดส่วนที่ล้นขอบซ้ายและขวาของกรอบใน
    if (bx + bw > SPL_INNER_W) bw = SPL_INNER_W - bx;
    if (bw <= 0) continue;
    tft.fillRect(innerX + bx, innerY, bw, innerH, SPL_C_MID);
    tft.fillRect(innerX + bx, innerY, bw, 2, SPL_C_LIGHT);                 // แถบสว่างด้านบนให้ดูนูนแบบ XP
    tft.fillRect(innerX + bx, innerY + innerH - 2, bw, 2, SPL_C_DARK);     // เงาด้านล่าง
  }
}

static uint32_t splashLastStep = 0xFFFFFFFFu;

// ขยับแถบโหลด เรียกบ่อยแค่ไหนก็ได้ (วาดจริงเมื่อถึงช่องถัดไปเท่านั้น)
static void splashTick() {
#if CYD_SPLASH_MS > 0
  if (splashStartMs == 0) return;
  const uint32_t ms = millis() - splashStartMs;
  const uint32_t step = ms / SPL_STEP_MS;
  if (step == splashLastStep) return;
  splashLastStep = step;
  splashDrawTrack(ms);
#endif
}

// วาดหน้าเปิดเครื่อง (เรียกหลัง panelBegin)
static void splashDraw() {
#if CYD_SPLASH_MS > 0
  splashStartMs = millis();
  if (splashStartMs == 0) splashStartMs = 1;
  splashLastStep = 0xFFFFFFFFu;
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
