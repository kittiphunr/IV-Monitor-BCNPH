/**
 * @file      board.h
 * @brief     ขาและค่าเลือกของบอร์ด ESP32-2432S028 (CYD)
 * @version   2.0.2-CYD
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
 * | 2.0.2-CYD | 2026-11-27 | เพิ่มขาและระดับขับของบัซเซอร์ Active (CYD_BUZZER_PIN, CYD_BUZZER_ON_LEVEL) · ค่าปริยายตามผลถ่ายภาพจากบอร์ดจริง: CYD_PANEL 7789 (ILI9341 ให้ภาพไม่เต็มจอและแดง/น้ำเงินสลับ), CYD_BGR 0, CYD_ROTATION 3 |
 * | 2.0.0-CYD | 2026-11-27 | เขียนใหม่ทั้งหมดด่านที่ 1 (ฐานบอร์ด: จอ ไฟหลังจอ ทัช) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <Arduino.h>

// ---- ค่าที่ผู้ใช้ปรับได้ (ตั้งผ่าน -D หรือแก้ตรงนี้) -----------------------------------------
#ifndef CYD_PANEL
#define CYD_PANEL     7789   // 7789 = ST7789 (USB สองช่อง ตั้งเป็นค่าปริยายเพราะตรงกับบอร์ดที่ทดสอบจริง) · 9341 = ILI9341 (USB ช่องเดียว)
#endif
#ifndef CYD_ROTATION
#define CYD_ROTATION  3      // 3 = แนวนอน (ปริยาย ตรงกับบอร์ดจริงที่ถือพอร์ต USB ไว้ด้านขวา) · 1 = แนวนอนกลับหัว · 0/2 = แนวตั้ง (หน้าจอ v2.0 รองรับแนวนอนเท่านั้น)
#endif
#ifndef CYD_INVERT
#define CYD_INVERT    0      // 1 = สั่งโหมดกลับสีของจอ ถ้าดำออกเป็นขาว/ซีด ลองสลับค่านี้
#endif
#ifndef CYD_BGR
#define CYD_BGR       0      // 0 = ลำดับสีปกติ (ผลจากบอร์ดจริง) · 1 = BGR ถ้าแดงกับน้ำเงินสลับกันให้สลับค่านี้ (เฉพาะ ST7789)
#endif
#ifndef CYD_BUZZER_PIN
#define CYD_BUZZER_PIN 27    // บัซเซอร์ Active ต่อที่ขา IO27 ของขั้ว CN1 (ว่างบนบอร์ด) -1 = ไม่ต่อบัซเซอร์ ห้ามใช้ GPIO26 (เป็นขาเข้าของวงจรขยายลำโพงบนบอร์ด)
#endif
#ifndef CYD_BUZZER_ON_LEVEL
#define CYD_BUZZER_ON_LEVEL HIGH   // HIGH = ขับตรงหรือผ่านทรานซิสเตอร์ NPN · LOW = โมดูลบัซเซอร์ที่ทำงานเมื่อขาเป็น LOW
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
