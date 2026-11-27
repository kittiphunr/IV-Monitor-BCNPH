/**
 * @file      board.h
 * @brief     ขาและค่าเลือกของบอร์ด ESP32-2432S028 (CYD)
 * @version   2.0.0-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * รวมหมายเลขขา ชิปจอ ทิศทางจอ และสวิตช์ปรับสีไว้ที่เดียว ไฟล์อื่นห้ามเขียนเลขขาเอง
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.0-CYD | 2026-11-27 | เขียนใหม่ทั้งหมดด่านที่ 1 (ฐานบอร์ด: จอ ไฟหลังจอ ทัช) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <Arduino.h>

// ---- ค่าที่ผู้ใช้ปรับได้ (ตั้งผ่าน -D หรือแก้ตรงนี้) -----------------------------------------
#ifndef CYD_PANEL
#define CYD_PANEL     9341   // 9341 = ILI9341 (USB ช่องเดียว) · 7789 = ST7789 (USB สองช่อง)
#endif
#ifndef CYD_ROTATION
#define CYD_ROTATION  1      // 1 = แนวนอน (320x240) · 3 = แนวนอนกลับหัว · 0/2 = แนวตั้ง
#endif
#ifndef CYD_INVERT
#define CYD_INVERT    0      // 1 = สั่งโหมดกลับสีของจอ ถ้าดำออกเป็นขาว/ซีด ลองสลับค่านี้
#endif
#ifndef CYD_BGR
#define CYD_BGR       1      // 1 = ลำดับสี BGR (เฉพาะ ST7789) ถ้าแดงกับน้ำเงินสลับกันให้ตั้ง 0
#endif
#ifndef CYD_SPI_HZ
#define CYD_SPI_HZ    40000000UL
#endif

// ---- ขาของบอร์ด (ตายตัว ตามผังวงจร ESP32-2432S028) ---------------------------------------
namespace cyd {
constexpr int TFT_SCK = 14, TFT_MISO = 12, TFT_MOSI = 13, TFT_CS = 15, TFT_DC = 2, TFT_BL = 21;
constexpr int TOUCH_CLK = 25, TOUCH_MOSI = 32, TOUCH_MISO = 39, TOUCH_CS = 33, TOUCH_IRQ = 36;
constexpr int BOOT_BTN = 0;
constexpr int SPEAKER = 26;
constexpr int PANEL_W = 240, PANEL_H = 320;   // ขนาดแผงตอนยังไม่หมุน
}  // namespace cyd
