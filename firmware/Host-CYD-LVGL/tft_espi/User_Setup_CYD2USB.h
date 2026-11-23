/**
 * @file      User_Setup_CYD2USB.h
 * @brief     ค่าตั้ง TFT_eSPI สำหรับบอร์ด CYD ที่มีพอร์ต USB สองช่อง (จอ ST7789) ใช้กับสาย Host-CYD-LVGL
 * @version   1.0.1-LVGL
 * @date      2026-11-23
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 * อาจารย์ที่ปรึกษา ดร.กรรณิการ์ กาศสมบูรณ์
 *
 * @par Description
 * ใช้แทน User_Setup.h เมื่อบอร์ดมีพอร์ต USB สองช่อง (Type-C + Micro-USB) วิธีใช้เหมือนกัน คือคัดลอกไปทับ
 * `Documents/Arduino/libraries/TFT_eSPI/User_Setup.h` (เปลี่ยนชื่อเป็น User_Setup.h) ก่อนคอมไพล์
 *
 * ค่าตรงกับ env `cyd2usb` ในโปรเจ็ก OttoMeister/ESP32-2432S028R (platformio.ini): `ST7789_DRIVER`,
 * `TFT_RGB_ORDER=TFT_BGR`, `TFT_INVERSION_OFF`, ขาจอเดียวกับรุ่น ILI9341 ต่างจากต้นแบบตรงที่ไม่ประกาศ TFT_BL
 * (เฟิร์มแวร์คุมไฟหลังจอด้วย PWM เอง) และใช้ SPI 40 MHz แทน 55 MHz เพื่อความเสถียร
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 1.0.1-LVGL | 2026-11-23 | ประกาศ VSPI_HOST/HSPI_HOST ให้ TFT_eSPI คอมไพล์ผ่านกับ ESP32 core รุ่นใหม่ (4.0.0) |
 * | 1.0.0-LVGL | 2026-11-23 | สร้างไฟล์ค่าตั้ง TFT_eSPI สำหรับ CYD รุ่นจอ ST7789 |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง
 * @warning  การคัดลอกทับไฟล์ในโฟลเดอร์ไลบรารีกระทบทุกโปรเจ็กต์ที่ใช้ TFT_eSPI เครื่องเดียวกัน ให้สำรองไฟล์เดิมไว้ก่อน
 */

#define USER_SETUP_ID 7789

// ---- ชิปจอ (ตรงกับ env cyd2usb ของโปรเจ็กต้นแบบ) ----
#define ST7789_DRIVER
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_OFF

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// ---- ขาจอ (บัส HSPI) เหมือนรุ่น ILI9341 ----
#define USE_HSPI_PORT
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1      // ต่อกับ EN ของบอร์ด
// ไม่ประกาศ TFT_BL (เฟิร์มแวร์คุมไฟหลังจอด้วย PWM บน GPIO21 เอง)

#define LOAD_GLCD

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000

// ---- ความเข้ากันได้กับ ESP32 Arduino core รุ่นใหม่ (เช่น 4.0.0) ----
// TFT_eSPI 2.5.43 (รุ่นล่าสุด) ยังเรียกชื่อ VSPI_HOST / HSPI_HOST ในส่วน DMA ของ ESP32 รุ่นปกติ
// แต่ core รุ่นใหม่เอาชื่อเหล่านี้ออกแล้ว (คอมไพเลอร์ขึ้น 'VSPI_HOST' was not declared ... did you mean 'SPI3_HOST')
// ไฟล์นี้ถูกอ่านก่อนส่วนนั้นของไลบรารี จึงประกาศชื่อเดิมให้ชี้ไปที่ชื่อใหม่ได้ โดยไม่ต้องแก้ไลบรารี
// ESP32 รุ่นปกติ: HSPI = SPI2_HOST, VSPI = SPI3_HOST  (core รุ่นเก่ามีชื่อนี้อยู่แล้ว บล็อกนี้จึงไม่มีผล)
#ifndef VSPI_HOST
  #define VSPI_HOST SPI3_HOST
#endif
#ifndef HSPI_HOST
  #define HSPI_HOST SPI2_HOST
#endif
