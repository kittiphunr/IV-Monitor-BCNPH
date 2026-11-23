/**
 * @file      User_Setup.h
 * @brief     ค่าตั้งของไลบรารี TFT_eSPI สำหรับบอร์ด ESP32-2432S028 (CYD) พอร์ต USB ช่องเดียว (ILI9341) ใช้คู่กับ `TFT_LIB 1`
 * @version   1.4.1-CYD
 * @date      2026-11-23
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Description
 * TFT_eSPI อ่านค่าตั้งจากไฟล์ในโฟลเดอร์ไลบรารีเท่านั้น (Arduino IDE ส่งตัวแปรจากสเก็ตช์ไปให้ไม่ได้)
 * จึงต้องคัดลอกไฟล์นี้ไปทับ `Documents/Arduino/libraries/TFT_eSPI/User_Setup.h` ก่อนคอมไพล์
 * และต้องตรวจว่าใน `User_Setup_Select.h` ของไลบรารีเปิดใช้ `#include <User_Setup.h>` (ค่าปริยายของไลบรารี)
 * ไฟล์นี้อยู่ในโฟลเดอร์ย่อย `tft_espi/` เพื่อไม่ให้ Arduino IDE นับเป็นส่วนของสเก็ตช์
 *
 * ค่าอ้างอิงจากโปรเจ็ก witnessmenow/ESP32-Cheap-Yellow-Display (DisplayConfig/User_Setup.h)
 * ที่ใช้กับ ESP32-2432S028 ล็อต ILI9341 ต่างจากไฟล์ต้นทางสองจุด คือไม่ประกาศ TFT_BL เพราะเฟิร์มแวร์
 * คุมไฟหลังจอด้วย PWM เอง (ถ้าประกาศ ไลบรารีจะดึงขา GPIO21 ไปควบคุมซ้ำ) และใช้ SPI 40 MHz
 * ซึ่งต่ำกว่าต้นทาง (55 MHz) เพื่อความเสถียร
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 1.4.1-CYD | 2026-11-23 | ประกาศ VSPI_HOST/HSPI_HOST ให้ TFT_eSPI คอมไพล์ผ่านกับ ESP32 core รุ่นใหม่ (4.0.0) |
 * | 1.4.0-CYD | 2026-11-23 | ตัดบล็อก ST7789 ที่เดาไว้ออก (ผิด) ชี้ไปที่ไฟล์ User_Setup_CYD2USB.h |
 * | 1.3.0-CYD | 2026-11-23 | สร้างไฟล์ค่าตั้ง TFT_eSPI สำหรับบอร์ด CYD |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง
 * @warning  การคัดลอกทับไฟล์ในโฟลเดอร์ไลบรารีกระทบทุกโปรเจ็กต์ที่ใช้ TFT_eSPI เครื่องเดียวกัน ให้สำรองไฟล์เดิมไว้ก่อน
 */

#define USER_SETUP_ID 9341

// ---- ชิปจอ ----
// CYD ล็อต ILI9341 ใช้ตัวนี้ (ไดรเวอร์ทางเลือกของ TFT_eSPI ตามที่โปรเจ็กอ้างอิงใช้)
#define ILI9341_2_DRIVER
// ถ้าลำดับสีหรือสีกลับด้านให้ลองสลับ: ILI9341_DRIVER แทน ILI9341_2_DRIVER
// #define ILI9341_DRIVER
// บอร์ดที่มีพอร์ต USB สองช่อง (Type-C + Micro-USB) ใช้จอ ST7789 ให้ใช้ไฟล์ User_Setup_CYD2USB.h แทนไฟล์นี้

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// ---- ขาจอ (บัส HSPI) ----
#define USE_HSPI_PORT
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1      // ต่อกับ EN ของบอร์ด
// ไม่ประกาศ TFT_BL (เฟิร์มแวร์คุมไฟหลังจอด้วย PWM บน GPIO21 เอง)

// ---- ฟอนต์ที่เฟิร์มแวร์ใช้ (ฟอนต์มาตรฐาน 6x8 ขยายด้วย setTextSize) ----
#define LOAD_GLCD

// ---- ความเร็ว SPI ----
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
