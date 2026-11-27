/**
 * @file      board.h
 * @brief     ขาและค่าเลือกของบอร์ด ESP32-2432S028 (CYD)
 * @version   2.0.5-CYD
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
 * | 2.0.5-CYD | 2026-11-27 | เพิ่ม CYD_SPLASH_MS (เวลาหน้าเปิดเครื่อง 0 = ปิด) |
 * | 2.0.3-CYD | 2026-11-27 | เพิ่มค่าเลือกของด่านที่ 4: ชื่อ/รหัสผ่านจุดเชื่อมต่อ Wi-Fi, บัญชีเว็บ, ขา RTC DS3231 (ปิดไว้), ขา SD |
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
// ---- ด่านที่ 4: Wi-Fi จุดเชื่อมต่อของ Host และหน้าเว็บ -----------------------------------------
#ifndef CYD_AP_SSID
#define CYD_AP_SSID   "SmartIV-Host"
#endif
#ifndef CYD_AP_PASS
#define CYD_AP_PASS   "12345678"      // อย่างน้อย 8 ตัวอักษร (WPA2) ควรเปลี่ยนก่อนใช้งานจริง
#endif
#ifndef CYD_WEB_USER
#define CYD_WEB_USER  "admin"         // บัญชีสำหรับสั่งแก้ค่า/เริ่มถุงใหม่ผ่านเว็บ (ดูอย่างเดียวไม่ต้องใช้)
#endif
#ifndef CYD_WEB_PASS
#define CYD_WEB_PASS  "smartiv"       // ควรเปลี่ยนก่อนใช้งานจริง
#endif
// RTC DS3231 (ไม่บังคับ) ปิดไว้เป็นค่าปริยายเพราะขา CN1 ของบอร์ด (IO27/IO22) ตรงกับขาบัซเซอร์ ตั้งสองค่านี้เพื่อเปิด
// และย้ายบัซเซอร์ไปขาอื่นก่อน
#ifndef CYD_RTC_SDA
#define CYD_RTC_SDA   -1
#endif
#ifndef CYD_RTC_SCL
#define CYD_RTC_SCL   -1
#endif
#ifndef CYD_SPLASH_MS
#define CYD_SPLASH_MS 2500          // หน้าเปิดเครื่อง (โลโก้) ค้างนานกี่ ms แตะเพื่อข้ามได้ 0 = ปิดหน้านี้
#endif
#ifndef CYD_SPI_HZ
#define CYD_SPI_HZ    40000000UL
#endif

// ---- ขาของบอร์ด (ตายตัว ตามผังวงจร ESP32-2432S028) ---------------------------------------
namespace cyd {
constexpr int TFT_SCK = 14, TFT_MISO = 12, TFT_MOSI = 13, TFT_CS = 15, TFT_DC = 2, TFT_BL = 21;
constexpr int TOUCH_CLK = 25, TOUCH_MOSI = 32, TOUCH_MISO = 39, TOUCH_CS = 33, TOUCH_IRQ = 36;
constexpr int SD_SCK = 18, SD_MISO = 19, SD_MOSI = 23, SD_CS = 5;     // ช่องการ์ด SD (บัส VSPI แยกจากจอ)
constexpr int BOOT_BTN = 0;
constexpr int SPEAKER = 26;
constexpr int PANEL_W = 240, PANEL_H = 320;   // ขนาดแผงตอนยังไม่หมุน
}  // namespace cyd
