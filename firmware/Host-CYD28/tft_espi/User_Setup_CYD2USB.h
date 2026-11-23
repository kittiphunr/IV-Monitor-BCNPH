/**
 * @file      User_Setup_CYD2USB.h
 * @brief     ค่าตั้ง TFT_eSPI สำหรับบอร์ด CYD ที่มีพอร์ต USB สองช่อง (จอ ST7789) ใช้คู่กับ `TFT_LIB 1`
 * @version   1.4.0-CYD
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
 * | 1.4.0-CYD | 2026-11-23 | สร้างไฟล์ค่าตั้ง TFT_eSPI สำหรับ CYD รุ่นจอ ST7789 |
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
