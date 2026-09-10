/**
 * @file      Host-TFT-Gateway.ino
 * @brief     เครื่องส่วนกลางจอ TFT 2.8" รุ่นเกตเวย์ คำนวณทุกอย่างแทน Station แล้วส่งขึ้นพีซี
 * @version   1.2.0-TG
 * @date      2026-11-21
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 * อาจารย์ที่ปรึกษา ดร.กรรณิการ์ กาศสมบูรณ์
 *
 * @par Hardware
 * ESP32-S3 Dev Module (N16R8) + จอสี TFT 2.8" (ST7789V SPI 240x320) + Passive Buzzer
 * ผังขาเหมือน Host-TFT28 ทุกขา
 *
 * @par Description
 * ใช้คู่กับ Station-Thin ผ่าน Protocol v4 (ดู protocol_v4.h) แบ่งงานใหม่ดังนี้
 *   Station-Thin ส่งแค่จำนวนหยดสะสมกับเวลาจากหยดล่าสุด
 *   เครื่องนี้  คำนวณอัตราไหล (rate_estimator.h) ปริมาตร % เวลาที่เหลือ ตัดสินเตือน
 *               แล้วส่งผลทั้งหมดกลับไปให้จอ Station แสดง
 *   พีซี       เก็บประวัติและเสิร์ฟหน้าเว็บ (server/PC-Dashboard) เครื่องนี้ไม่เปิดเว็บเอง
 *
 * ส่วนลิงก์ขึ้นพีซี (เธรดแยก คำสั่งมีเลขลำดับและ ack หน้าตั้งค่าครั้งแรก) ยกมาจาก Host-Gateway 1.0.0-GW
 * โดยใช้โปรโตคอลฝั่งพีซีเดียวกัน (docs/GATEWAY-PROTOCOL.md) เซิร์ฟเวอร์ PC-Dashboard ตัวเดียวใช้ได้ทั้งสองสาย
 * ส่วนจอ TFT ตรรกะเตือน และปุ่ม ยกมาจาก Host-TFT28 4.9.1-TFT
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 1.2.0-TG | 2026-11-21 | เหลือปุ่มเดียว (GPIO 4) ทำทุกอย่างตามจังหวะกด ตัดปุ่มเปลี่ยนหน้า (GPIO 5) ออก |
 * | 1.1.0-TG | 2026-11-20 | ตรวจระดับแบตเตอรี่ทั้งของ Host เองและของทุกเตียง: ไอคอน แถบล่าง เสียงเตือนเบา ๆ เว้นช่วงนาน และส่งระดับขึ้นพีซี |
 * | 1.0.0-TG | 2026-11-19 | รุ่นแรก แตกจาก Host-TFT28 4.9.1-TFT ตัดเว็บ ล็อก CSV และ AP ออก เพิ่มลิงก์ขึ้นพีซี ใช้ Protocol v4 ให้ Host เป็นผู้คำนวณอัตราไหลและส่งผลกลับให้ Station แสดง |
 * | ↓ | ↓ | ป้ายรุ่นด้านล่างสืบมาจากโค้ดของ Host-TFT28 ที่ยกมาโดยไม่แก้ ใช้ตามรอยเหตุผลได้ใน Host-TFT28.ino |
 * | 4.9.1-TFT | 2026-09-28 | Sync บอกเตียงด้วยว่า Host ได้ยินมันอยู่หรือไม่ (บิตว่างใน flags ขนาดโครงสร้างไม่เปลี่ยน) |
 * | 4.9.0-TFT | 2026-09-23 | เตียงที่หายไประหว่างให้น้ำเกลือมีเสียงเตือนแล้ว เพิ่มสุนัขเฝ้าบ้าน รายงานสาเหตุการรีบูต และปฏิเสธแพ็กเก็ตที่ค่าเป็นไปไม่ได้ |
 *
 * @warning  ไฟล์วาดจอถูก `#include` ท้ายไฟล์นี้ก่อน `setup()` ห้ามย้ายขึ้นไปบนสุด
 *           เพราะโค้ดในไฟล์นั้นใช้ตัวแปรและฟังก์ชันช่วยที่ประกาศไว้ด้านบน
 *
 * @warning  `protocol_v4.h` ต้องเหมือนกันทุกไบต์กับของ Station-Thin
 *           ตรวจด้วย `bash tools/v4-test/run.sh` ก่อน commit ทุกครั้งที่แตะ
 *
 * @warning  เครื่องนี้คุยได้เฉพาะ Station-Thin (Protocol v4) ไม่รับ Station สายอื่นที่ใช้ Protocol v3
 *           และไม่ส่ง Sync แบบ v3 ไปให้ใคร
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 *
 * @par บันทึกการเปลี่ยนแปลงโดยละเอียด
 * ประวัติก่อน 1.0.0-TG อยู่ใน Host-TFT28.ino และ Host-Gateway.ino ซึ่งยังคงอยู่ครบ ที่นี่ไม่คัดลอกมาซ้ำ
 *
 *  เปลี่ยนใน V1.0.0-TG: Host เป็นผู้คำนวณ Station เป็นแค่ตากับจอ
 *   1) รับ Protocol v4 จาก Station-Thin: หยดสะสม + เวลาจากหยดล่าสุด + ธงสถานะ + แบตเตอรี่ (14 ไบต์)
 *   2) คำนวณอัตราไหลด้วย rate_estimator.h จากจุดยึดเวลาของหยดที่แม่นยำ ผลต่างจากวิธีเดิมของ Station
 *      (หน้าต่าง 8 หยด) ไม่เกิน 0.4% ทดสอบแม้แพ็กเก็ตหาย 50% (tools/v4-test)
 *   3) ส่งผลกลับให้ Station แสดงเตียงละหนึ่งแพ็กเก็ตต่อวินาที (32 ไบต์) แบบเฉลี่ยช่องเวลา ไม่ใช้ delay()
 *   4) ตัด WebServer ล็อก CSV และ AP ออก ใช้ลิงก์ขึ้นพีซีแบบ Host-Gateway เวลามาจากพีซี
 *   5) การตัดสินเตือนทั้งหมดยังอยู่ที่ Host เหมือนเดิม ไม่ผ่านพีซี
 *   6) จอ TFT แสดงสถานะลิงก์พีซีแทนจำนวนอุปกรณ์ที่ต่อ AP
 *
 *  เปลี่ยนใน V1.2.0-TG: เหลือปุ่มเดียว
 *   - ปุ่มเปิด-ปิด/พักเสียง (GPIO 4) ทำทุกอย่างตามจังหวะกด และตัดปุ่มเปลี่ยนหน้า (GPIO 5) ออก
 *     กดสั้นขณะมีเสียงเตือน = พักเสียง, ขณะเตือนใกล้หมด = รับทราบ, ปกติ: 1 ครั้ง = เตียงถัดไป
 *     2 ครั้ง = โหมดนาฬิกา, 3 ครั้ง = ดับ/เปิดจอ, กดค้าง 2 วินาที = ปิดเครื่อง
 *   - เหตุทางคลินิกมาก่อนเสมอ และตอบสนองทันที ไม่ต้องรอช่องนับคลิก
 *   - ล้างค่าเครือข่าย: กดปุ่มค้างตอนเปิดเครื่อง 8 วินาที (เดิม 3 วินาทีบนปุ่มเปลี่ยนหน้า) ยาวขึ้นเพราะ
 *     ปุ่มเดียวกันใช้ปลุกเครื่อง (ค้าง 1.5 วินาที) การค้างเผลอ ๆ จึงต้องไม่ลบค่าเครือข่ายทิ้ง
 *   - ข้อความบนจอบอกปุ่มเดียว ("PRESS BUTTON = SNOOZE", "PRESS BUTTON TO WAKE")
 *
 *  เปลี่ยนใน V1.1.0-TG: ตรวจระดับแบตเตอรี่ทั้งสองฝั่ง (battery_check.h ใช้ร่วมกับ Station-Thin)
 *   - Host ตรวจแบตเตอรี่ของตัวเอง และของทุกเตียงจากแรงดัน (batteryMv) ที่เตียงส่งมาในแพ็กเก็ตขึ้น
 *     เกณฑ์เดียวกับที่เครื่องเตียงตรวจตัวเอง: ต่ำ < 3.50 V วิกฤต < 3.35 V ต้องอยู่ต่อเนื่อง 30 วินาที
 *   - ไอคอนแบตเตอรี่ที่หัวจอมีเครื่องหมาย ! เมื่อ Host เองต่ำ และแถบล่างบอกเตียงที่แบตต่ำ
 *     (ลำดับความสำคัญ: เหตุวิกฤต > เตือนใกล้หมด > แบตต่ำ > ปกติ สีเหลือง/ส้ม ไม่ใช้แดงซึ่งสงวนไว้ให้เหตุทางคลินิก)
 *   - เสียงเตือนเบา ๆ สองครั้งทุก 5 นาที (ต่ำ) หรือสามครั้งทุก 1 นาที (วิกฤต) เงียบเมื่อมีเหตุวิกฤตที่ยังไม่พักเสียง
 *     หรือกำลังเตือนใกล้หมด และพักเสียงด้วยปุ่มเดียวกับเตือนปกติได้
 *   - เตียงที่ออฟไลน์ไม่นับ (ระดับของมันเก่าแล้ว และมีเตือนเตียงหายอยู่แล้ว)
 *   - ส่งระดับ batLevel ของ Host และของทุกเตียงขึ้นพีซีในสถานะที่ส่งทุกวินาที
 */

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>          // [1.0.0-TG] เพิ่ม: ส่งสถานะขึ้นพีซี
#include "battery_check.h"      // [1.1.0-TG] เพิ่ม: ตัวตรวจระดับแบตเตอรี่ ใช้ร่วมกับ Station-Thin
#include "protocol_v4.h"        // [1.0.0-TG] เพิ่ม: Protocol v4 ใช้ร่วมกับ Station-Thin
#include "rate_estimator.h"     // [1.0.0-TG] เพิ่ม: Host คำนวณอัตราไหลเอง
#include <ctype.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <time.h>
#include <sys/time.h>
#include <driver/rtc_io.h>
#include <esp_task_wdt.h>          // [4.9.0-TFT] เพิ่ม: สุนัขเฝ้าบ้าน กันเครื่องค้างเงียบ
#include <esp_system.h>            // [4.9.0-TFT] เพิ่ม: อ่านสาเหตุการรีบูตครั้งล่าสุด

// ----------------------------------------------------------------------------
// สุนัขเฝ้าบ้านและสาเหตุการรีบูต ([4.9.0-TFT] เพิ่มทั้งหมด)
//
// เครื่องนี้ทำหน้าที่เตือนภัย การค้างแบบเงียบจึงอันตรายกว่าการรีบูต เพราะจอยัง
// ค้างภาพเดิมไว้ พยาบาลจึงเข้าใจว่าระบบยังเฝ้าอยู่ ทั้งที่หยุดไปแล้ว
// ตั้งไว้ 8 วินาที ซึ่งยาวกว่ารอบ loop() ปกติหลายเท่า จึงไม่รีบูตเพราะงานหนักชั่วคราว
// ----------------------------------------------------------------------------
#define LOOP_WDT_TIMEOUT_S      8

esp_reset_reason_t bootResetReason = ESP_RST_UNKNOWN;


#define APP_VERSION         "1.2.0-TG"
#define DEV_NAME            "กิตติพันธ์ รัตนคร"
#define DEV_ROLE            "นักวิชาการคอมพิวเตอร์"
#define DEV_INSTITUTION     "มหาวิทยาลัยมหาจุฬาลงกรณราชวิทยาลัย วิทยาเขตแพร่"

// ----------------------------------------------------------------------------
// กำหนดขาเชื่อมต่อฮาร์ดแวร์ฝั่ง Host
// ----------------------------------------------------------------------------
#define HOST_BAT_ADC_PIN    1   // ขาอ่านแบตเตอรี่ Host
#define POWER_BTN_PIN       4   // ปุ่มเปิด-ปิดหน้าจอ / พักเสียงเตือน (Snooze) — ต้องเป็น RTC GPIO (0-21)
#define BUZZER_PIN          7   // ขาต่อลำโพง Passive Buzzer

// ---- จอ TFT 2.8" ST7789V (SPI 14 ขา ไม่มีทัช) ----
// ต่อสาย: VCC->3V3, GND->GND, SCL/SCK->TFT_SCLK, SDA/MOSI->TFT_MOSI,
//         RES->TFT_RST, DC->TFT_DC, CS->TFT_CS, BLK->TFT_BLK (หรือต่อ 3V3 ตลอดแล้วตั้ง -1)
// หมายเหตุ: บอร์ด N16R8 ใช้ GPIO33-37 กับ PSRAM ห้ามนำมาใช้
#define TFT_CS              10
#define TFT_DC              9
#define TFT_RST             8
#define TFT_MOSI            11
#define TFT_SCLK            12
#define TFT_BLK             13  // -1 = ไม่ได้ต่อขาควบคุมไฟหน้าจอ
#define TFT_ROTATION        1   // 1 = แนวนอน 320x240 (ถ้าภาพกลับหัวให้ใช้ 3)

// ----------------------------------------------------------------------------
// ค่าคงที่ของระบบสื่อสารและการแจ้งเตือน (ต้องตรงกับ Station)
// ----------------------------------------------------------------------------
#define ESPNOW_CHANNEL          1        // ช่อง SoftAP / ESP-NOW เริ่มต้น
#define SYNC_INTERVAL_MS        1000     // ส่ง Sync ให้ Station ทุก 1 วินาที
#define ONLINE_TIMEOUT_MS       5000     // ไม่ได้รับข้อมูลเกิน 5 วินาที = OFFLINE
#define LOST_LINK_GRACE_MS      30000    // [4.9.0-TFT] เพิ่ม: เงียบต่อจาก OFFLINE อีกเท่านี้จึงถือว่าขาดการติดต่อจริง
#define MAX_PLAUSIBLE_RATE_HR   3000.0f  // [4.9.0-TFT] เพิ่ม: ชุดให้สารน้ำมาตรฐานไหลได้ไม่ถึงเท่านี้
#define MAX_PLAUSIBLE_BATT_V    6.0f     // [4.9.0-TFT] เพิ่ม: แบตลิเธียมเซลล์เดียว ไม่มีทางถึง 6 โวลต์
#define MIN_OCCLUSION_MS        8000     // เวลาต่ำสุดที่ไม่มีหยดก่อนถือว่าหยุดไหล
#define MAX_OCCLUSION_MS        300000   // เพดานเวลาตัดสินหยุดไหล (อัตราต่ำมาก)
#define RATE_DEVIATION_HOLD_MS  20000    // เร็ว/ช้าเกินต่อเนื่อง 20 วินาทีจึงเตือน
#define SNOOZE_MS               120000   // พักเสียง 2 นาที
#define NEAR_END_REMIND_MS      300000   // เตือนใกล้หมดซ้ำทุก 5 นาทีจนกว่าจะรับทราบ
#define SCREENSAVER_IDLE_MS     300000   // ไม่มีการกดปุ่ม 5 นาที -> เข้าโหมดนาฬิกา
#define MANUAL_FOCUS_HOLD_MS    60000    // เลือกเตียงเองแล้วคงไว้ 1 นาที ก่อนกลับเป็นอัตโนมัติ
#define DEFAULT_NEAR_END_PCT    80       // เตือนเมื่อให้ไปแล้ว 80% ของปริมาตรตามแผน
#define TIME_BACKUP_INTERVAL_MS 600000   // สำรองเวลาปัจจุบันลง NVS ทุก 10 นาที

// ---- ลิงก์ขึ้นพีซี ([1.0.0-TG] เพิ่มทั้งหมวด) ----
// เครื่องนี้ไม่เปิดเว็บเซิร์ฟเวอร์ให้ผู้ใช้อีกแล้ว ส่งสถานะทุกเตียงไปพีซีวินาทีละหนึ่งครั้ง
// แล้วรับคำสั่งกลับมาในคำตอบของคำขอเดียวกัน (Host ไม่ต้องรับการเชื่อมต่อขาเข้าเลย)
#define PC_PUSH_INTERVAL_MS     1000     // ส่งสถานะวินาทีละครั้ง
#define PC_CONNECT_TIMEOUT_MS   1200     // รอต่อพีซีนานสุดเท่านี้ ไม่ให้เธรดส่งค้างนาน
#define PC_REPLY_TIMEOUT_MS     1500     // รอคำตอบจากพีซีนานสุดเท่านี้
#define PC_DEFAULT_PORT         8080
#define PC_LINK_STALE_MS        5000     // ส่งไม่สำเร็จต่อเนื่องเท่านี้ = ลิงก์พีซีขาด (แสดงบนจอ)
#define CMD_QUEUE_LEN           12       // คำสั่งจากพีซีที่รอ loop() มาประมวลผล
#define CMD_ARGS_MAX            120
#define UPLINK_TASK_STACK       8192
#define SETUP_PORTAL_MS         600000UL // โหมดตั้งค่าเปิดนานสุด 10 นาทีแล้วรีบูตกลับเข้าโหมดปกติ

// ---- เครือข่าย: เกาะเราเตอร์อย่างเดียว ไม่เปิด AP ให้ผู้ใช้ ----
// ต่อไม่ได้หรือหลุดก็ลองใหม่เรื่อย ๆ ระหว่างนั้นการเตือนหน้างานทำงานตามปกติ
// (เดิมเปิด AP สำรองให้พยาบาลเข้าเว็บที่ตัวเครื่อง ซึ่งคือภาระที่รุ่นนี้ตัดทิ้ง)
#define STA_CONNECT_TIMEOUT_MS  15000    // รอเกาะเราเตอร์นานสุด 15 วินาทีต่อรอบ
#define STA_RECONNECT_MS        10000    // ยังไม่ได้ต่อ/หลุด ลองเกาะใหม่ครั้งแรกหลัง 10 วินาที
#define STA_RECONNECT_MAX_MS    300000   // แล้วเว้นห่างขึ้นเท่าตัวทุกครั้งที่พลาด สูงสุด 5 นาที
                                         // เพราะทุกครั้งที่ WiFi.begin() ค้นหาเราเตอร์ วิทยุจะกระโดดข้ามช่อง
                                         // แพ็กเก็ต ESP-NOW จากเตียงหายไประหว่างนั้น ยิ่งถี่ยิ่งเสียหาย
#define SETUP_HOLD_MS           8000     // [1.2.0-TG] แก้: กดปุ่มค้างตอนเปิดเครื่องนานเท่านี้ = ล้างค่าเครือข่าย (เดิม 3000 บนปุ่มเปลี่ยนหน้า)
#define SETUP_AP_MAX_CLIENTS    2        // หน้าตั้งค่าใช้ครั้งเดียว ไม่ต้องรองรับหลายเครื่อง
#define SYNC_SLOT_MIN_MS        60       // ช่องเวลาต่ำสุดระหว่างแพ็กเก็ตลงสองใบ (เฉลี่ยช่อง ไม่ใช้ delay)
#define LINK_WINDOW_MS          10000UL  // หน้าต่างวัดคุณภาพลิงก์ (คาดหวัง 10 ใบ)
#define LINK_WEAK_PCT           60       // ต่ำกว่านี้ = ลิงก์อ่อน

// รหัสแจ้งเตือน (ใช้ร่วมกันทั้ง Host / Station / Web)
#define ALERT_NONE        0
#define ALERT_TOO_FAST    1
#define ALERT_TOO_SLOW    2
#define ALERT_NEAR_END    3   // ให้ไปแล้วถึง % ที่ตั้ง (เตือนเบา ไม่ใช่เหตุวิกฤต)
#define ALERT_COMPLETE    4   // ให้ครบตามแผนแล้ว
#define ALERT_OCCLUSION   5   // สายพับ / หยุดไหล
#define ALERT_LOST_LINK   7   // [4.9.0-TFT] เพิ่ม: เตียงที่กำลังให้น้ำเกลือหายไปจากอากาศ (ใช้ฝั่ง Host เท่านั้น)

SPIClass SPI_TFT(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI_TFT, TFT_CS, TFT_DC, TFT_RST);

const char *default_ap_ssid = "IV-Gateway-Setup";   // [1.0.0-TG] แก้: เปิดเฉพาะโหมดตั้งค่าครั้งแรก
const char *default_ap_pass = "12345678";

// เขตเวลาไทย (ICT UTC+7) — ตั้งค่าในตัวเครื่อง ไม่พึ่ง NTP อีกต่อไป
const char* TZ_THAILAND     = "ICT-7";

#define MAX_SUPPORTED_STATIONS 8

WebServer server(80);
Preferences preferences;
// ---- สถานะเครือข่าย ----
enum NetMode : uint8_t { NET_MODE_AP = 0, NET_MODE_STA = 1 };
NetMode  netMode          = NET_MODE_AP;   // โหมดที่ใช้อยู่จริงในขณะนี้
String   staSsid          = "";
String   staPass          = "";
bool     staConfigured    = false;         // มีรหัสเราเตอร์บันทึกไว้หรือไม่
bool     staConnecting    = false;         // กำลังรอเกาะเราเตอร์อยู่
uint32_t staAttemptStart  = 0;
uint32_t staLostSince     = 0;             // เวลาที่เริ่มหลุดจากเราเตอร์ (0 = ไม่ได้หลุด)
uint32_t lastStaRetry     = 0;
uint32_t staRetryDelayMs  = STA_RECONNECT_MS;   // [1.0.0-TG] เพิ่ม: ช่วงรอก่อนลองเกาะใหม่ ขยายเท่าตัวเมื่อพลาด
int8_t   lastStaRssi      = 0;

// ---- ปลายทางพีซี ([1.0.0-TG] เพิ่ม) ----
String   pcHost           = "";            // IP หรือชื่อเครื่องพีซี
uint16_t pcPort           = PC_DEFAULT_PORT;
String   pcToken          = "";            // ว่าง = พีซีไม่ตรวจรหัส
bool     pcConfigured     = false;

// สถานะลิงก์ขึ้นพีซี (เธรดส่งเขียน loop() และจออ่าน จึงเป็น volatile)
volatile bool     pcLinkUp   = false;
volatile uint32_t lastPcOkAt = 0;
volatile uint32_t pcPushOk   = 0;
volatile uint32_t pcPushFail = 0;
String            hostId     = "";      // MAC ของ Host ไม่มีโคลอน ใช้แยก Host หลายตัวที่พีซี

// โหมดตั้งค่าครั้งแรก (Setup Portal) เปิดเฉพาะตอนยังไม่มีรหัสเราเตอร์
bool     portalActive     = false;
uint32_t portalStarted    = 0;
void handlePortalRoot();            // นิยามไว้ท้ายไฟล์ เรียกจาก startSetupPortal() ซึ่งอยู่เหนือมัน
void handlePortalSave();

portMUX_TYPE dataMux = portMUX_INITIALIZER_UNLOCKED;

uint8_t activeStationCount  = 5;
int  manualFocusBed         = -1;      // เตียงที่ผู้ใช้เลือกดูเอง (-1 = อัตโนมัติ)
unsigned long manualFocusUntil = 0;
bool displaySleeping        = false;
bool screensaverActive      = false;
bool isPowerOffProgressActive = false;

// ---- โหมดของหน้าจอ ----
#define UI_MODE_MAIN    0
#define UI_MODE_ALARM   1
#define UI_MODE_SAVER   2
#define UI_MODE_POWER   3

int   uiMode           = -1;
int   uiFocusDrawn     = -1;
int   uiRowsDrawn      = -1;
String cacheClock      = "";
String cacheTopRight   = "";
String cacheFocusHead  = "";
String cacheStatusWord = "";
String cacheRate       = "";
String cacheSetRate    = "";
String cacheSummary    = "";
String cacheBottom     = "";
int    cachePct        = -999;
String cacheSide[MAX_SUPPORTED_STATIONS];
String cacheAlarmRate  = "";

unsigned long lastUserActivityTime = 0;
bool isTimeSynced           = false;   // ตั้งเวลาจากเครื่องผู้ใช้ผ่านหน้าเว็บแล้ว
bool isTimeApprox           = false;   // กู้เวลาจากที่สำรองไว้ตอนบูต (ยังไม่ซิงก์ใหม่)
unsigned long lastTimeBackup = 0;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ---- สถานะของเตียงสำหรับการแสดงผล ----
// ต้องประกาศไว้ตอนต้นไฟล์ เพราะ Arduino IDE แทรก prototype ของฟังก์ชันไว้ก่อนส่วนแสดงผล
enum BedUiStatus { BU_NORMAL = 0, BU_FAST, BU_SLOW, BU_NOFLOW, BU_NEAREND, BU_DONE, BU_PAUSED, BU_OFFLINE };

// ----------------------------------------------------------------------------
// โครงสร้างข้อมูลรับ-ส่ง ESP-NOW (Protocol v4) อยู่ใน protocol_v4.h ใช้ร่วมกับ Station-Thin
// ----------------------------------------------------------------------------
struct BedConfig {
  float   targetRateHr = 0.0f;
  float   planVolumeMl = 0.0f;
  uint8_t dropFactor   = 20;
  uint8_t resetSeq     = 0;
  char    patientName[64] = "";
};

struct TrialCaseRecord {
  bool active = false;
  uint8_t caseNumber = 1;
  char startTimeStr[24] = "";
  float targetRate = 0.0;
  float planVolume = 0.0;
  float actualVolume = 0.0;
  float errorMl = 0.0;
  float errorPercent = 0.0;
};

struct StationData {
  bool active = false;
  bool isRunning = true;
  int8_t rssi = -100;
  float batteryVolts = 0.0;
  uint32_t totalDrops = 0;
  uint32_t lastTotalDrops = 0;
  bool hasBaseline = false;
  uint32_t dropsInCurrentMinute = 0;
  float totalVolumeMl = 0.0;
  float flowRate_ml_hr = 0.0;
  uint32_t msSinceLastDrop = 0;
  unsigned long lastRecvTime = 0;
  uint8_t alertCode = ALERT_NONE;
  unsigned long deviationSince = 0;
  uint8_t nearEndPct = DEFAULT_NEAR_END_PCT;   // เก็บแยก key ใน NVS เพื่อไม่ให้ค่าตั้งเดิมหาย
  bool nearEndAck = false;
  bool wasMonitoring = false;   // [4.9.0-TFT] เพิ่ม: เคยเห็นเตียงนี้ออนไลน์และกำลังนับอยู่จริง
  bool lostLinkAck = false;     // [4.9.0-TFT] เพิ่ม: พยาบาลรับทราบว่าเตียงนี้ขาดการติดต่อแล้ว
  unsigned long nearNextChime = 0;

  BedConfig cfg;
  TrialCaseRecord trialCase;

  // [1.0.0-TG] เพิ่ม: ผลคำนวณอัตราไหลและคุณภาพลิงก์ (เดิมอัตราไหลมาจาก Station)
  RateEstimator est;
  BatteryMonitor bat;            // [1.1.0-TG] เพิ่ม: ระดับแบตเตอรี่ของเตียงนี้ (จากแรงดันที่ส่งมา)
  bool     rateValid = false;
  uint8_t  srcMac[6] = {0};
  bool     macKnown = false;
  uint32_t rxTotal = 0;
  uint16_t rxWindowCount = 0;
  uint8_t  linkPct = 0;
};

// สำเนาของสถานะเตียงที่ต้องส่งขึ้นพีซี (คัดลอกใต้ล็อกแล้วค่อยสร้างข้อความนอกล็อก)
struct SnapBed {
  bool     online, running, idConflict, macKnown, nearEndAck, caseActive;
  int8_t   rssi;
  uint8_t  link, dropFactor, alertCode, nearEndPct, mac[6], batLevel;
  uint32_t rxTotal, totalDrops, msSinceLastDrop;
  float    battery, volumeMl, flowRateHr, targetRate, planVolume;
  char     patientName[64];
  char     caseStart[24];
};

StationData stations[MAX_SUPPORTED_STATIONS];
unsigned long lastSyncSlotTime      = 0;
uint16_t      pendingSyncMask       = 0;   // บิตที่ 0 = เตียง 1 ... ต้องส่งแพ็กเก็ตลงทันที
uint8_t       syncRoundIdx          = 0;
uint32_t      syncSendFail          = 0;
unsigned long lastLinkWindowTime    = 0;
unsigned long lastCalcTime           = 0;
unsigned long lastMinuteLogTime      = 0;
unsigned long lastOledUpdateTime     = 0;
unsigned long lastSyncBroadcastTime  = 0;
unsigned long globalSnoozeUntil      = 0;
uint32_t globalMinuteCounter         = 0;

// ---- ตรวจระดับแบตเตอรี่ ([1.1.0-TG] เพิ่ม) ----
#define BAT_CHIME_LOW_MS        300000UL   // ต่ำ: เตือนทุก 5 นาที
#define BAT_CHIME_CRIT_MS       60000UL    // วิกฤต: เตือนทุก 1 นาที
BatteryMonitor hostBat;                    // แบตเตอรี่ของ Host เอง
uint8_t        batChimeRemaining = 0;
unsigned long  batChimeNextBeep  = 0;
unsigned long  batChimeNextRound = 0;
uint8_t        batChimeLevelSeen = BAT_NONE;   // ระดับที่เสียงเตือนรอบล่าสุดอิงอยู่ ใช้ตรวจว่าแย่ลงหรือยัง

float hostBatteryVolts = 4.2;
int hostBatteryPct = 100;

bool globalAlarmTriggered            = false;
unsigned long lastBuzzerAlarmTime    = 0;
uint8_t nearChimeRemaining           = 0;
unsigned long nearChimeNextBeep      = 0;

// ฟังก์ชันวาดจอที่ย้ายไป HostScreen.h แล้ว แต่ถูกเรียกจากโค้ดเหนือบรรทัด include
void textAt(const String &s, int x, int y, uint8_t size, uint16_t col, uint16_t bg);
void drawWifiIcon(int x, int y, int clients);
void drawBatteryIcon(int x, int y, int pct);
void updateBottomBar();
void updateSideList(int focus, bool force);
void drawFocusPanel(int focus, bool force);
void drawMainFramework();
void drawAlarmFramework();
void updateAlarmScreen();
void drawSaverFramework();
void updateHostDisplay();
void setDisplaySleep(bool sleep);
int  currentFocusBed();
void drawPowerOffProgress(int pct);
void drawGoodbyeScreen();

// ----------------------------------------------------------------------------
// ฟังก์ชันช่วย
// ----------------------------------------------------------------------------
// เรียกได้ทุกที่ รวมถึงก่อนสมัครสมาชิก — ถ้ายังไม่ได้สมัครจะคืนค่าผิดพลาดเฉย ๆ
inline void feedWatchdog() {
  esp_task_wdt_reset();
}

void setupLoopWatchdog() {
#if defined(ESP_IDF_VERSION) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  esp_task_wdt_config_t wdtCfg = {};
  wdtCfg.timeout_ms     = LOOP_WDT_TIMEOUT_S * 1000;
  wdtCfg.idle_core_mask = 0;            // ไม่เฝ้างานว่าง เฝ้าเฉพาะ loop() ของเรา
  wdtCfg.trigger_panic  = true;         // ค้างจริง = รีบูต ดีกว่าค้างเงียบต่อไป
  // core 3.x เปิดตัวเฝ้าไว้ให้แล้วในบางการตั้งค่า จึงต้องเผื่อทางตั้งค่าใหม่ด้วย
  if (esp_task_wdt_init(&wdtCfg) == ESP_ERR_INVALID_STATE) esp_task_wdt_reconfigure(&wdtCfg);
#else
  esp_task_wdt_init(LOOP_WDT_TIMEOUT_S, true);
#endif
  esp_task_wdt_add(NULL);
}

// ปิดการเฝ้าก่อนเข้าโหมดหลับ มิฉะนั้นการรอให้ปล่อยปุ่มจะถูกนับว่าค้าง
void stopLoopWatchdog() {
  esp_task_wdt_delete(NULL);
}

// ข้อความสั้น ๆ ไว้แสดงในหน้าเว็บและใน Serial เพื่อให้ตามรอยปัญหาในวอร์ดจริงได้
const char* resetReasonText(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:  return "power-on";
    case ESP_RST_EXT:      return "external";
    case ESP_RST_SW:       return "software";
    case ESP_RST_PANIC:    return "panic";
    case ESP_RST_INT_WDT:  return "int-wdt";
    case ESP_RST_TASK_WDT: return "task-wdt";
    case ESP_RST_WDT:      return "other-wdt";
    case ESP_RST_DEEPSLEEP:return "deep-sleep";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO:     return "sdio";
    default:               return "unknown";
  }
}

uint8_t safeDropFactor(uint8_t df) {
  return (df == 10 || df == 15 || df == 20 || df == 60) ? df : 20;
}

bool isCriticalAlert(uint8_t code) {
  return code == ALERT_TOO_FAST || code == ALERT_TOO_SLOW ||
         code == ALERT_COMPLETE || code == ALERT_OCCLUSION ||
         code == ALERT_LOST_LINK;
}

uint8_t safeNearPct(uint8_t p) {
  return (p >= 50 && p <= 95) ? p : DEFAULT_NEAR_END_PCT;
}

bool isSnoozed() {
  return globalSnoozeUntil > 0 && millis() < globalSnoozeUntil;
}

bool isStationOnline(int i) {
  unsigned long lr = stations[i].lastRecvTime;
  if (lr == 0) return false;
  unsigned long now = millis();
  return (now < lr) || (now - lr < ONLINE_TIMEOUT_MS);
}

// [4.9.0-TFT] เพิ่ม: เตียงที่กำลังให้น้ำเกลืออยู่ แล้วเงียบหายไปจากอากาศ
// ต้องเคยเห็นว่าออนไลน์และกำลังนับมาก่อน เตียงที่ยังไม่เคยเปิดใช้จึงไม่ร้อง
bool isStationLostLink(int i) {
  const StationData &s = stations[i];
  if (!s.wasMonitoring || s.lostLinkAck) return false;
  if (isStationOnline(i)) return false;
  if (s.lastRecvTime == 0) return false;
  return (millis() - s.lastRecvTime) > (ONLINE_TIMEOUT_MS + LOST_LINK_GRACE_MS);
}

// เกณฑ์เวลา "ไม่มีหยด" = 2.5 เท่าของช่วงหยดตามเป้าหมาย (ขั้นต่ำ 8 วินาที) — สูตรเดียวกับ Station
uint32_t occlusionThresholdMs(float rateHr, uint8_t df) {
  if (rateHr <= 0.0f) return MIN_OCCLUSION_MS;
  float dropsPerHr = rateHr * (float)df;
  uint32_t expected = (uint32_t)(3600000.0f / dropsPerHr);
  uint32_t th = (expected * 5) / 2;
  if (th < MIN_OCCLUSION_MS) th = MIN_OCCLUSION_MS;
  if (th > MAX_OCCLUSION_MS) th = MAX_OCCLUSION_MS;
  return th;
}

bool isStationOccluded(int i) {
  const StationData &s = stations[i];
  if (!isStationOnline(i) || !s.isRunning || s.totalDrops == 0 || s.msSinceLastDrop == 0) return false;
  unsigned long age = millis() - s.lastRecvTime;
  uint32_t since = s.msSinceLastDrop + (uint32_t)age;
  return since > occlusionThresholdMs(s.cfg.targetRateHr, safeDropFactor(s.cfg.dropFactor));
}

// [4.9.0-TFT] เพิ่ม: รหัสที่ส่งออกไปให้ Station — รหัสเฉพาะฝั่ง Host ต้องกลายเป็น ALERT_NONE
uint8_t alertCodeForStation(uint8_t code) {
  return (code == ALERT_LOST_LINK) ? ALERT_NONE : code;
}

const char* alertTextEn(uint8_t code) {
  switch (code) {
    case ALERT_TOO_FAST:  return "TOO FAST";
    case ALERT_TOO_SLOW:  return "TOO SLOW";
    case ALERT_NEAR_END:  return "NEXT BAG";
    case ALERT_COMPLETE:  return "BAG EMPTY";
    case ALERT_OCCLUSION: return "OCCLUSION";
    case ALERT_LOST_LINK: return "BED LOST";
    default:              return "NORMAL";
  }
}

String jsonEscape(const char* s) {
  String o;
  for (const char* p = s; *p; p++) {
    char c = *p;
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c < 0x20) o += ' ';
    else o += c;
  }
  return o;
}

uint8_t currentWifiChannel() {
  uint8_t primary = 0;
  wifi_second_chan_t second;
  esp_wifi_get_channel(&primary, &second);
  return primary;
}

// ----------------------------------------------------------------------------
// ค่าตั้งเตียง (เก็บถาวรใน NVS)
// ----------------------------------------------------------------------------
// [1.0.0-TG] เพิ่ม: ผูก peer ใหม่ทุกครั้งที่สลับโหมด เพราะอินเทอร์เฟซเปลี่ยนจาก AP เป็น STA
// ผูก peer กระจายเสียงของ ESP-NOW เข้ากับอินเทอร์เฟซที่ใช้งานอยู่จริง
void attachEspNowPeer(wifi_interface_t ifidx) {
  esp_now_del_peer(broadcastAddress);          // ไม่เป็นไรถ้ายังไม่มี
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;                        // 0 = ใช้ช่องปัจจุบันของอินเทอร์เฟซ
  peerInfo.ifidx   = ifidx;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

// [1.0.0-TG] เพิ่ม: กันไม่ให้ Arduino core สั่งโหมดประหยัดพลังงานทับตอนเข้าโหมด STA
// บังคับปิดโหมดประหยัดพลังงานของวิทยุ (ดูข้อ 4 ด้านบน)
void forceRadioAwake() {
  WiFi.setSleep(false);
  esp_wifi_set_ps(WIFI_PS_NONE);
}

void loadWifiConfig() {
  preferences.begin("wifi-config", true);
  staSsid = preferences.getString("ssid", "");
  staPass = preferences.getString("pass", "");
  pcHost  = preferences.getString("pchost", "");
  pcPort  = (uint16_t)preferences.getUInt("pcport", PC_DEFAULT_PORT);
  pcToken = preferences.getString("pctok", "");
  preferences.end();
  staConfigured = (staSsid.length() > 0);
  pcConfigured  = (pcHost.length() > 0);
  if (pcPort == 0) pcPort = PC_DEFAULT_PORT;
}

// [1.0.0-TG] เพิ่ม: เก็บที่อยู่พีซีไว้ข้างรหัสเราเตอร์
void savePcConfig(const String &host, uint16_t port, const String &token) {
  preferences.begin("wifi-config", false);
  preferences.putString("pchost", host);
  preferences.putUInt("pcport", port);
  preferences.putString("pctok", token);
  preferences.end();
  pcHost = host;
  pcPort = port;
  pcToken = token;
  pcConfigured = (pcHost.length() > 0);
}

void saveWifiConfig(const String &ssid, const String &pass) {
  preferences.begin("wifi-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);
  preferences.end();
  staSsid = ssid;
  staPass = pass;
  staConfigured = (staSsid.length() > 0);
}

void clearWifiConfig() {
  preferences.begin("wifi-config", false);
  preferences.clear();
  preferences.end();
  staSsid = "";
  staPass = "";
  pcHost = "";
  pcToken = "";
  staConfigured = false;
  pcConfigured = false;
}

// [1.0.0-TG] แก้: โหมดตั้งค่าเปิด AP เฉพาะตอนยังไม่มีรหัสเราเตอร์ ไม่ใช่โหมดสำรองอีกต่อไป
// เปิด Access Point + หน้าตั้งค่า (เฉพาะตอนยังไม่มีรหัสเราเตอร์ หรือผู้ใช้ล้างค่าด้วยปุ่ม)
void startSetupPortal() {
  staConnecting = false;
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(default_ap_ssid, default_ap_pass, ESPNOW_CHANNEL, 0, SETUP_AP_MAX_CLIENTS);
  forceRadioAwake();
  netMode = NET_MODE_AP;
  lastStaRssi = 0;
  attachEspNowPeer(WIFI_IF_AP);

  server.on("/", handlePortalRoot);
  server.on("/save", HTTP_POST, handlePortalSave);
  server.begin();
  portalActive  = true;
  portalStarted = millis();
  Serial.printf("[GW] โหมดตั้งค่า: เชื่อมต่อ Wi-Fi \"%s\" แล้วเปิด http://%s\n",
                default_ap_ssid, WiFi.softAPIP().toString().c_str());
}

// [1.0.0-TG] แก้: เกาะเราเตอร์อย่างเดียว ต่อไม่ได้ก็ลองใหม่ ไม่เปิด AP สำรอง
// เริ่มเกาะเราเตอร์ แบบไม่บล็อก (ผลลัพธ์ไปเช็กใน serviceWifi)
void startStaMode() {
  if (!staConfigured) { startSetupPortal(); return; }
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  forceRadioAwake();                 // ครั้งที่ 1 ก่อน begin
  netMode = NET_MODE_STA;
  attachEspNowPeer(WIFI_IF_STA);     // ผูกทันที ไม่รอต่อติด เพราะ ESP-NOW ต้องรับได้ตลอดแม้เราเตอร์ล่ม
  WiFi.begin(staSsid.c_str(), staPass.c_str());
  forceRadioAwake();                 // ครั้งที่ 2 หลัง begin เพราะ core ทับค่าตอน STA_START
  staConnecting   = true;
  staAttemptStart = millis();
  Serial.printf("[GW] กำลังเกาะเราเตอร์ \"%s\"\n", staSsid.c_str());
}

void onStaConnected() {
  staConnecting = false;
  staLostSince  = 0;
  staRetryDelayMs = STA_RECONNECT_MS;   // ต่อติดแล้ว เริ่มนับถอยหลังใหม่
  netMode       = NET_MODE_STA;
  forceRadioAwake();                 // ครั้งที่ 3 หลังเชื่อมต่อสำเร็จ กันไว้อีกชั้น
  attachEspNowPeer(WIFI_IF_STA);
  lastStaRssi = WiFi.RSSI();
  Serial.printf("[GW] เกาะเราเตอร์แล้ว IP %s ช่อง %d RSSI %d พีซี %s:%u\n",
                WiFi.localIP().toString().c_str(), currentWifiChannel(), lastStaRssi,
                pcHost.c_str(), (unsigned)pcPort);
}

// เครื่องสถานะของเครือข่าย เรียกทุกรอบของ loop() ห้ามบล็อก
void serviceWifi(uint32_t now) {
  // --- โหมดตั้งค่า: หมดเวลาแล้วรีบูต เผื่อมีคนกดค้างปุ่มตอนเปิดเครื่องโดยบังเอิญ ---
  if (portalActive) {
    if (now - portalStarted >= SETUP_PORTAL_MS) ESP.restart();
    return;
  }

  // --- กำลังรอเกาะเราเตอร์ ---
  if (staConnecting) {
    if (WiFi.status() == WL_CONNECTED) { onStaConnected(); return; }
    if (now - staAttemptStart >= STA_CONNECT_TIMEOUT_MS) {
      Serial.println("[GW] เกาะเราเตอร์ไม่สำเร็จ จะลองใหม่ (การเตือนหน้างานทำงานต่อตามปกติ)");
      staConnecting = false;
      staLostSince  = now;
      lastStaRetry  = now;
      lastStaRssi   = 0;
    }
    return;
  }

  // --- ต่อเราเตอร์อยู่: เฝ้าดูว่าหลุดหรือยัง ---
  if (WiFi.status() == WL_CONNECTED) {
    staLostSince = 0;
    lastStaRssi  = WiFi.RSSI();
    return;
  }
  lastStaRssi = 0;
  if (staLostSince == 0) { staLostSince = now; lastStaRetry = now; return; }
  if (now - lastStaRetry >= staRetryDelayMs) {
    lastStaRetry = now;
    staRetryDelayMs = (staRetryDelayMs * 2 > STA_RECONNECT_MAX_MS) ? STA_RECONNECT_MAX_MS : staRetryDelayMs * 2;
    Serial.printf("[GW] เราเตอร์หลุด ลองเกาะใหม่ (ครั้งต่อไปอีก %u วินาที)\n", (unsigned)(staRetryDelayMs / 1000));
    startStaMode();
  }
}

// สถานะลิงก์ขึ้นพีซีเป็นข้อความสั้นสำหรับจอ OLED (ASCII ล้วน)
// [1.0.0-TG] เพิ่ม: แทนที่ที่อยู่หน้าเว็บ ซึ่งเครื่องนี้ไม่มีแล้ว
String gatewayLine() {
  if (portalActive)   return String("SETUP ") + WiFi.softAPIP().toString();
  if (!staConfigured) return String("NO WIFI SET");
  if (WiFi.status() != WL_CONNECTED) return String("NO ROUTER");
  if (!pcConfigured)  return String("NO PC SET");
  return pcLinkUp ? String("PC OK ") + pcHost : String("PC LINK DOWN");
}


void loadBedConfigs() {
  preferences.begin("bed-cfg", true);
  for (int i = 0; i < MAX_SUPPORTED_STATIONS; i++) {
    char key[6];
    snprintf(key, sizeof(key), "b%d", i + 1);
    if (preferences.getBytesLength(key) == sizeof(BedConfig)) {
      preferences.getBytes(key, &stations[i].cfg, sizeof(BedConfig));
      stations[i].cfg.dropFactor = safeDropFactor(stations[i].cfg.dropFactor);
      stations[i].cfg.patientName[sizeof(stations[i].cfg.patientName) - 1] = '\0';
    }
    char pkey[6];
    snprintf(pkey, sizeof(pkey), "p%d", i + 1);
    stations[i].nearEndPct = safeNearPct(preferences.getUChar(pkey, DEFAULT_NEAR_END_PCT));
    stations[i].trialCase.active     = stations[i].cfg.planVolumeMl > 0;
    stations[i].trialCase.planVolume = stations[i].cfg.planVolumeMl;
    stations[i].trialCase.targetRate = stations[i].cfg.targetRateHr;
  }
  preferences.end();
}

void saveBedConfig(int idx) {
  char key[6];
  snprintf(key, sizeof(key), "b%d", idx + 1);
  preferences.begin("bed-cfg", false);
  preferences.putBytes(key, &stations[idx].cfg, sizeof(BedConfig));
  char pkey[6];
  snprintf(pkey, sizeof(pkey), "p%d", idx + 1);
  preferences.putUChar(pkey, stations[idx].nearEndPct);
  preferences.end();
}

// ----------------------------------------------------------------------------
// ระบบเวลาและการซิงก์ข้อมูล
// ----------------------------------------------------------------------------
String getFormattedDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) {
    char buf[24];
    snprintf(buf, sizeof(buf), "Min-%04u", (unsigned)globalMinuteCounter);
    return String(buf);
  }
  char buf[24];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buf);
}

String getDisplayClockStr() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) return "--:--";
  char buf[12];
  strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
  return String(buf);
}

// นาฬิกาพร้อมใช้งานหรือยัง (ตั้งจากเครื่องผู้ใช้ หรือกู้จากค่าที่สำรองไว้)
bool hasValidClock() {
  time_t now;
  time(&now);
  return now > 1600000000;
}

// บันทึกเวลาปัจจุบันลง NVS เพื่อกู้คืนเมื่อไฟดับ (ไม่มี RTC สำรองในบอร์ด)
void backupClockToNvs() {
  time_t now;
  time(&now);
  if (now <= 1600000000) return;
  preferences.begin("sys-config", false);
  preferences.putUInt("lastEpoch", (uint32_t)now);
  preferences.end();
}

float readHostBattery() {
  int raw = analogRead(HOST_BAT_ADC_PIN);
  float volts = (raw / 4095.0) * 3.3 * 2.0;
  if (volts > 4.25) volts = 4.2;
  if (volts < 2.5) volts = 0.0;
  return volts;
}

int calculateBatteryPct(float volts) {
  if (volts <= 0.5) return 100;
  int pct = (int)(((volts - 3.2) / (4.2 - 3.2)) * 100.0);
  if (pct > 100) pct = 100;
  if (pct < 0) pct = 0;
  return pct;
}

void playWelcomeMelody() {
  int melody[] = { 523, 659, 784, 1046 };
  int durations[] = { 80, 80, 80, 200 };
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i], durations[i]);
    delay(durations[i] + 30);
  }
  noTone(BUZZER_PIN);
}

void playShutdownMelody() {
  int melody[] = { 1046, 784, 659, 523 };
  int durations[] = { 80, 80, 80, 250 };
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i], durations[i]);
    delay(durations[i] + 30);
  }
  noTone(BUZZER_PIN);
}

// ----------------------------------------------------------------------------
// ระบบควบคุม Power & Deep Sleep (ESP32-S3 ใช้ ext0 wakeup)
// ----------------------------------------------------------------------------
void enterDeepSleepWaitPowerButton() {
  stopLoopWatchdog();   // [4.9.0-TFT] เพิ่ม: เลิกเฝ้าก่อนหลับ ไม่งั้นถูกนับว่าค้าง
  rtc_gpio_pullup_en((gpio_num_t)POWER_BTN_PIN);
  rtc_gpio_pulldown_dis((gpio_num_t)POWER_BTN_PIN);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)POWER_BTN_PIN, 0);
  esp_deep_sleep_start();
}

void powerOffSystem() {
  isPowerOffProgressActive = true;

  drawGoodbyeScreen();

  playShutdownMelody();

  while (digitalRead(POWER_BTN_PIN) == LOW) { feedWatchdog(); delay(20); }
  delay(200);

  tft.fillScreen(0x0000);
#if TFT_BLK >= 0
  digitalWrite(TFT_BLK, LOW);
#endif

  if (portalActive) server.stop();
  esp_now_deinit();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  enterDeepSleepWaitPowerButton();
}

// ----------------------------------------------------------------------------
// ประเมินสภาวะเตือนภัย (Host เป็นผู้ตัดสินจุดเดียว)
// ----------------------------------------------------------------------------
void evaluateClinicalAlerts() {
  bool anyCritical = false;
  unsigned long now = millis();

  for (int i = 0; i < activeStationCount; i++) {
    StationData &s = stations[i];
    // [4.9.0-TFT] แก้: แยก "ติดต่อเตียงไม่ได้" ออกจาก "พยาบาลสั่งหยุด" ของเดิมรวมสองกรณีนี้
    // ไว้ด้วยกันแล้วล้างรหัสเตือนทิ้งทั้งคู่ เตียงที่กำลังให้น้ำเกลืออยู่แล้วหายไป
    // จากอากาศจึงไม่มีเสียงใด ๆ ทั้งที่ระบบเลิกเฝ้าเตียงนั้นไปแล้ว
    if (!isStationOnline(i)) {
      uint8_t lost = isStationLostLink(i) ? ALERT_LOST_LINK : ALERT_NONE;
      s.alertCode = lost;
      s.deviationSince = 0;
      if (lost != ALERT_NONE) anyCritical = true;   // รหัสนี้ตัดสินตรงนี้ เพราะข้ามส่วนท้ายไป
      continue;
    }

    // ติดต่อได้ตามปกติ = ล้างสถานะขาดการติดต่อทิ้ง พร้อมรับเหตุการณ์ครั้งใหม่
    s.lostLinkAck = false;
    s.wasMonitoring = s.isRunning;        // เฝ้าอยู่จริงเฉพาะตอนที่กำลังนับ

    if (!s.isRunning) {
      s.alertCode = ALERT_NONE;
      s.deviationSince = 0;
      continue;
    }

    float tr   = s.cfg.targetRateHr;
    float act  = s.flowRate_ml_hr;
    float vol  = s.totalVolumeMl;
    float plan = s.cfg.planVolumeMl;
    uint8_t code = ALERT_NONE;

    if (isStationOccluded(i)) {
      code = ALERT_OCCLUSION;
      s.deviationSince = 0;
    } else if (plan > 0 && vol >= plan) {
      code = ALERT_COMPLETE;
      s.deviationSince = 0;
    } else {
      bool rateValid = (act > 0.0f) && (s.totalDrops >= 3);
      uint8_t devCode = ALERT_NONE;
      if (tr > 0 && rateValid && act > tr * 1.35f)      devCode = ALERT_TOO_FAST;
      else if (tr > 0 && rateValid && act < tr * 0.65f) devCode = ALERT_TOO_SLOW;

      if (devCode != ALERT_NONE) {
        if (s.deviationSince == 0) s.deviationSince = now;
        if (now - s.deviationSince >= RATE_DEVIATION_HOLD_MS) code = devCode;
      } else {
        s.deviationSince = 0;
      }

      if (code == ALERT_NONE && plan > 0 && vol >= plan * (float)safeNearPct(s.nearEndPct) / 100.0f) code = ALERT_NEAR_END;
    }

    s.alertCode = code;
    if (isCriticalAlert(code)) anyCritical = true;
  }

  // ถ้าไม่มีเหตุวิกฤตแล้ว ยกเลิก snooze เพื่อให้เหตุการณ์ครั้งถัดไปดังทันที
  if (!anyCritical) globalSnoozeUntil = 0;
  globalAlarmTriggered = anyCritical;
}

bool anyNearEndPending() {
  for (int i = 0; i < activeStationCount; i++) {
    if (stations[i].alertCode == ALERT_NEAR_END && !stations[i].nearEndAck) return true;
  }
  return false;
}

void acknowledgeAllNearEnd() {
  for (int i = 0; i < activeStationCount; i++) {
    if (stations[i].alertCode == ALERT_NEAR_END) stations[i].nearEndAck = true;
  }
  nearChimeRemaining = 0;
}

// เสียงเตือนใกล้หมด: 3 ครั้งสั้น ๆ ต่อเตียง ซ้ำทุก 5 นาทีจนกว่าจะรับทราบ (ไม่ดังทับเหตุวิกฤต)
void handleNearEndChime() {
  unsigned long now = millis();
  if (globalAlarmTriggered && !isSnoozed()) { nearChimeRemaining = 0; return; }

  if (nearChimeRemaining == 0) {
    for (int i = 0; i < activeStationCount; i++) {
      StationData &s = stations[i];
      if (s.alertCode != ALERT_NEAR_END || s.nearEndAck) { s.nearNextChime = 0; continue; }
      if (s.nearNextChime == 0 || now >= s.nearNextChime) {
        s.nearNextChime = now + NEAR_END_REMIND_MS;
        nearChimeRemaining = 3;
        nearChimeNextBeep = now;
        break;
      }
    }
  }
  if (nearChimeRemaining > 0 && now >= nearChimeNextBeep) {
    tone(BUZZER_PIN, 2000, 70);
    nearChimeNextBeep = now + 220;
    nearChimeRemaining--;
  }
}

void handleBuzzerAlarm() {
  static uint8_t beepPhase = 0;
  if (!globalAlarmTriggered || isSnoozed()) {
    beepPhase = 0;
    handleNearEndChime();
    return;
  }

  unsigned long now = millis();
  if (beepPhase == 0 && now - lastBuzzerAlarmTime >= 2500) {
    tone(BUZZER_PIN, 1500, 100);
    lastBuzzerAlarmTime = now;
    beepPhase = 1;
  } else if (beepPhase == 1 && now - lastBuzzerAlarmTime >= 200) {
    tone(BUZZER_PIN, 1500, 100);
    beepPhase = 0;
  }
}

// [1.1.0-TG] เพิ่ม: อ่านระดับแบตเตอรี่ของ Host เอง (0 = ไม่มีวงจรวัด/ใช้ไฟ USB ไม่เตือน)
void serviceHostBattery(unsigned long now) {
  uint16_t mv = (hostBatteryVolts > 0.0f) ? (uint16_t)(hostBatteryVolts * 1000.0f + 0.5f) : 0;
  hostBat.update((uint32_t)now, mv);
}

// ระดับแย่สุดในบรรดา Host และเตียงที่ยังออนไลน์ (เตียงออฟไลน์ไม่นับ ระดับของมันเก่าแล้ว)
uint8_t worstBatteryLevel() {
  uint8_t w = hostBat.level;
  for (int i = 0; i < activeStationCount; i++) {
    if (!isStationOnline(i)) continue;
    if (stations[i].bat.level > w) w = stations[i].bat.level;
  }
  return w;
}

// เสียงเตือนแบตเตอรี่เบา ๆ เว้นช่วงนาน เป็นเรื่องบำรุงรักษา ไม่ใช่เหตุทางคลินิก
// เงียบเมื่อมีเหตุวิกฤตที่ยังไม่พักเสียง หรือกำลังเตือนใกล้หมด และพักเสียงด้วยปุ่มเดียวกับเตือนปกติได้
void handleBatteryChime() {
  unsigned long now = millis();
  uint8_t worst = worstBatteryLevel();
  if (worst < BAT_LOW) { batChimeRemaining = 0; batChimeNextRound = 0; batChimeLevelSeen = BAT_NONE; return; }
  // ระดับเปลี่ยน (โดยเฉพาะต่ำ -> วิกฤต) ต้องเตือนทันที ไม่รอรอบเดิมที่นับตามช่วงของระดับก่อนหน้าซึ่งนานสุด 5 นาที
  if (worst != batChimeLevelSeen) { batChimeLevelSeen = worst; batChimeNextRound = 0; }
  if ((globalAlarmTriggered && !isSnoozed()) || nearChimeRemaining > 0 || isSnoozed()) {
    batChimeRemaining = 0;
    return;
  }
  if (batChimeRemaining == 0 && (batChimeNextRound == 0 || now >= batChimeNextRound)) {
    batChimeRemaining = (worst == BAT_CRIT) ? 3 : 2;
    batChimeNextBeep  = now;
    batChimeNextRound = now + ((worst == BAT_CRIT) ? BAT_CHIME_CRIT_MS : BAT_CHIME_LOW_MS);
  }
  if (batChimeRemaining > 0 && now >= batChimeNextBeep) {
    tone(BUZZER_PIN, 1500, 70);
    batChimeNextBeep = now + 250;
    batChimeRemaining--;
  }
}

// ----------------------------------------------------------------------------
// ปุ่มเดียว ([1.2.0-TG] แก้: เดิมมีสองปุ่ม คือปุ่มเปิด-ปิด/พักเสียง กับปุ่มเปลี่ยนหน้า)
// ----------------------------------------------------------------------------
// ปุ่มเดียว (POWER_BTN_PIN ต้องเป็น RTC GPIO เพราะใช้ปลุกจาก deep sleep ด้วย) ทำทุกอย่างตามจังหวะกด
//
//   กดสั้นขณะมีเสียงเตือนที่ยังไม่พัก   = พักเสียง 2 นาที (ทันที ไม่รอจับดับเบิลคลิก)
//   กดสั้นขณะมีเตือนใกล้หมดยังไม่รับทราบ = รับทราบทุกเตียง (ทันที)
//   กดสั้นปกติ  : 1 ครั้ง = เลื่อนดูเตียงถัดไปเอง (กลับเป็นอัตโนมัติเมื่อครบเวลาหรือมีเหตุใหม่)
//                 2 ครั้ง = โหมดนาฬิกา (Screensaver)
//                 3 ครั้ง = ดับ/เปิดจอ
//                 (จอดับอยู่หรืออยู่ในโหมดนาฬิกา: กดครั้งแรกแค่ปลุกจอ ไม่ทำอย่างอื่น)
//   กดค้าง 0.45 วินาทีขึ้นไป = ขึ้นหน้าถามปิดเครื่อง กดค้างต่อถึง 2 วินาที = ปิดเครื่อง (ปล่อยก่อน = ยกเลิก)
//
// เหตุทางคลินิกมาก่อนเสมอ: ขณะมีเสียงเตือนหรือเตือนใกล้หมด กดสั้นครั้งแรกเป็นการพัก/รับทราบเท่านั้น
// เลื่อนหน้าได้อีกครั้งหลังจากนั้น กลุ่มกดสั้นปกตินับในช่อง 280 มิลลิวินาทีหลังปล่อยครั้งล่าสุด
void checkHostButton() {
  static bool          isHolding = false;
  static unsigned long pressStart = 0;
  static unsigned long lastProgressFrame = 0;
  static unsigned long lastReleaseTime = 0;
  static uint8_t       clickCount = 0;

  unsigned long now = millis();

  if (digitalRead(POWER_BTN_PIN) == LOW) {
    if (!isHolding) { pressStart = now; isHolding = true; }
    unsigned long holdDuration = now - pressStart;

    if (holdDuration >= 450 && !displaySleeping) {
      isPowerOffProgressActive = true;
      if (now - lastProgressFrame >= 60) {
        lastProgressFrame = now;
        int progressPct = map(holdDuration, 450, 2000, 0, 100);
        drawPowerOffProgress(progressPct);
      }
    }
    if (holdDuration >= 2000) powerOffSystem();
    return;
  }

  // ---- ปล่อยปุ่ม ----
  if (isHolding) {
    unsigned long held = now - pressStart;
    isHolding = false;
    pressStart = 0;
    lastUserActivityTime = now;

    if (isPowerOffProgressActive) {          // ปล่อยก่อนครบ 2 วินาที = ยกเลิกการปิดเครื่อง
      isPowerOffProgressActive = false;
      uiMode = -1;
      updateHostDisplay();
    }

    if (held < 400) {
      if (globalAlarmTriggered && !isSnoozed()) {
        // กดสั้นขณะมีเสียงเตือน = พักเสียง 2 นาที ทันที
        globalSnoozeUntil = now + SNOOZE_MS;
        noTone(BUZZER_PIN);
        tone(BUZZER_PIN, 2200, 40);
        if (displaySleeping) setDisplaySleep(false);
        screensaverActive = false;
        clickCount = 0;
        uiMode = -1;
        updateHostDisplay();
      } else if (anyNearEndPending()) {
        // กดสั้นขณะมีเตือนใกล้หมดที่ยังไม่รับทราบ = รับทราบทุกเตียง ทันที
        acknowledgeAllNearEnd();
        tone(BUZZER_PIN, 2600, 40);
        if (displaySleeping) setDisplaySleep(false);
        screensaverActive = false;
        clickCount = 0;
        uiMode = -1;
        updateHostDisplay();
      } else {
        if (clickCount < 255) clickCount++;
        lastReleaseTime = now;
      }
    } else {
      clickCount = 0;                         // กดนานกว่ากดสั้นแต่ไม่ถึงปิดเครื่อง ไม่นับเป็นคลิก
    }
  }

  // ---- ครบช่องนับคลิก: ตัดสินตามจำนวนครั้ง ----
  if (clickCount > 0 && (now - lastReleaseTime > 280)) {
    uint8_t n = clickCount;
    clickCount = 0;
    lastUserActivityTime = now;
    if (displaySleeping) {                    // จอดับอยู่: ปลุกจออย่างเดียว
      setDisplaySleep(false);
      tone(BUZZER_PIN, 1800, 25);
    } else if (screensaverActive) {           // โหมดนาฬิกา: ปลุกกลับหน้าปกติอย่างเดียว
      screensaverActive = false;
    } else if (n >= 3) {                      // สามครั้ง = ดับจอ
      setDisplaySleep(true);
      tone(BUZZER_PIN, 1800, 25);
      return;
    } else if (n == 2) {                      // สองครั้ง = โหมดนาฬิกา
      screensaverActive = true;
    } else {
      // ครั้งเดียว = เลื่อนไปดูเตียงถัดไปเอง (กลับเป็นอัตโนมัติเมื่อครบเวลาหรือมีเหตุใหม่)
      int base = (manualFocusBed >= 0) ? manualFocusBed : currentFocusBed();
      manualFocusBed = (base + 1) % activeStationCount;
      manualFocusUntil = now + MANUAL_FOCUS_HOLD_MS;
      tone(BUZZER_PIN, 2400, 15);
    }
    uiMode = -1;
    updateHostDisplay();
  }
}

// ----------------------------------------------------------------------------
// ESP-NOW Receive Callback
// ----------------------------------------------------------------------------
#if defined(ESP_IDF_VERSION) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingDataBytes, int len) {
  int8_t pktRssi = (info && info->rx_ctrl) ? info->rx_ctrl->rssi : -60;
  const uint8_t *src = (info && info->src_addr) ? info->src_addr : nullptr;
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataBytes, int len) {
  int8_t pktRssi = -60;
  const uint8_t *src = mac;
#endif
  // [1.0.0-TG] แก้: รับ Protocol v4 เท่านั้น กรองด้วยความยาวกับรหัสนำหน้า
  // แพ็กเก็ต v3 (23 ไบต์) หรือของอุปกรณ์อื่นที่บังเอิญอยู่ช่องเดียวกันจะถูกทิ้งตรงนี้
  if (len != sizeof(struct_up_v4)) return;
  if (incomingDataBytes[0] != V4_MAGIC_UP) return;

  struct_up_v4 incoming;
  memcpy(&incoming, incomingDataBytes, sizeof(incoming));
  if (incoming.stationId < 1 || incoming.stationId > MAX_SUPPORTED_STATIONS) return;

  // ปฏิเสธค่าที่เป็นไปไม่ได้ทางกายภาพ (แพ็กเก็ต ESP-NOW ไม่มีลายเซ็น)
  if (incoming.flags & 0xF0) return;                                  // บิตสำรองต้องเป็น 0
  if (incoming.batteryMv > (uint16_t)(MAX_PLAUSIBLE_BATT_V * 1000.0f)) return;
  if (incoming.msSinceLastDrop > 0 && incoming.totalDrops == 0) return;   // มีเวลาจากหยดล่าสุดแต่ไม่เคยมีหยด

  int idx = incoming.stationId - 1;
  uint32_t nowMs = millis();

  portENTER_CRITICAL(&dataMux);
  StationData &s = stations[idx];

  if (src != nullptr) { memcpy(s.srcMac, src, 6); s.macKnown = true; }

  // นับหยดรายนาทีจากผลต่าง totalDrops (แพ็กเก็ตหายก็ไม่ทำให้หยดหาย)
  if (s.hasBaseline) {
    uint32_t delta = (incoming.totalDrops >= s.lastTotalDrops)
                     ? (incoming.totalDrops - s.lastTotalDrops)
                     : incoming.totalDrops;   // Station รีเซ็ต/รีบูต
    s.dropsInCurrentMinute += delta;
  } else {
    s.hasBaseline = true;
  }
  s.lastTotalDrops  = incoming.totalDrops;

  s.active          = true;
  s.isRunning       = (incoming.flags & V4_UP_RUNNING) != 0;
  s.rssi            = pktRssi;
  s.batteryVolts    = incoming.batteryMv / 1000.0f;
  s.bat.update(nowMs, incoming.batteryMv);   // [1.1.0-TG] เพิ่ม: ตรวจระดับแบตเตอรี่ของเตียง (0 = ไม่มีวงจรวัด ไม่เตือน)
  s.totalDrops      = incoming.totalDrops;
  s.msSinceLastDrop = incoming.msSinceLastDrop;
  s.lastRecvTime    = nowMs;
  if (s.rxWindowCount < 0xFFFF) s.rxWindowCount++;
  s.rxTotal++;

  // [1.0.0-TG] เพิ่ม: Host คำนวณอัตราไหลเอง จากจุดยึดเวลาของหยดที่แม่นยำ (rate_estimator.h)
  uint8_t df = safeDropFactor(s.cfg.dropFactor);
  float r = s.est.update(nowMs, incoming.totalDrops, incoming.msSinceLastDrop, s.isRunning, df);
  s.flowRate_ml_hr  = s.isRunning ? r : 0.0f;
  s.rateValid       = s.isRunning && s.est.valid;
  s.totalVolumeMl   = (float)s.totalDrops / (float)df;
  if ((incoming.flags & V4_UP_NEAR_ACK) && s.cfg.planVolumeMl > 0) s.nearEndAck = true;

  if (s.trialCase.active) {
    s.trialCase.actualVolume = s.totalVolumeMl;
    s.trialCase.errorMl = s.trialCase.actualVolume - s.trialCase.planVolume;
    if (s.trialCase.planVolume > 0) {
      s.trialCase.errorPercent = (s.trialCase.errorMl / s.trialCase.planVolume) * 100.0;
    }
  }
  portEXIT_CRITICAL(&dataMux);
}

// ----------------------------------------------------------------------------
// Host OLED Display & Block-Style UI Renderer
// ----------------------------------------------------------------------------
// ============================================================================
// ส่วนแสดงผลบนจอ TFT 2.8" (ST7789V 240x320 ใช้แนวนอน 320x240)
//   หน้าหลัก FOCUS : เตียงที่ต้องดูตอนนี้ตัวใหญ่ครึ่งจอซ้าย + รายการเตียงอื่นด้านขวา
//   จอเตือนเต็มจอ  : เมื่อมีเหตุวิกฤตและยังไม่พักเสียง
//   Screensaver    : นาฬิกาใหญ่เมื่อไม่มีการใช้งาน
// วาดเฉพาะส่วนที่ค่าเปลี่ยน เพื่อไม่ให้จอกะพริบและไม่กิน CPU ตอนรับ ESP-NOW
// ============================================================================

// ---- สีที่ใช้บนจอ (RGB565) ----
#define C_BG        0x0000
#define C_CARD      0x18C5   // การ์ดพื้นเทาเข้ม
#define C_BAR       0x2104   // รางแถบความคืบหน้า
#define C_OFFCARD   0x1082   // การ์ดเตียงออฟไลน์/ยังไม่ใช้งาน
#define C_TOPBAR    0x0A49   // แถบบนสุด
#define C_LINE      0x39E7
#define C_DIM       0x8410
#define C_WHITE     0xFFFF
#define C_GREEN     0x2FEB
#define C_CYAN      0x07FF
#define C_YELLOW    0xFFE0
#define C_ORANGE    0xFD20
#define C_RED       0xF800
#define C_SKY       0x5DFF

BedUiStatus bedUiStatus(int i) {
  if (!isStationOnline(i)) return BU_OFFLINE;
  const StationData &s = stations[i];
  if (!s.isRunning) return BU_PAUSED;
  switch (s.alertCode) {
    case ALERT_OCCLUSION: return BU_NOFLOW;
    case ALERT_TOO_FAST:  return BU_FAST;
    case ALERT_TOO_SLOW:  return BU_SLOW;
    case ALERT_COMPLETE:  return BU_DONE;
    case ALERT_NEAR_END:  return BU_NEAREND;
    default:              return BU_NORMAL;
  }
}

uint16_t bedColor(BedUiStatus s) {
  switch (s) {
    case BU_NORMAL:  return C_GREEN;
    case BU_FAST:    return C_ORANGE;
    case BU_SLOW:    return C_YELLOW;
    case BU_NOFLOW:  return C_RED;
    case BU_NEAREND: return C_ORANGE;
    case BU_DONE:    return C_CYAN;
    case BU_PAUSED:  return C_SKY;
    default:         return C_LINE;
  }
}

const char* bedWord(BedUiStatus s) {
  switch (s) {
    case BU_NORMAL:  return "NORMAL";
    case BU_FAST:    return "TOO FAST";
    case BU_SLOW:    return "TOO SLOW";
    case BU_NOFLOW:  return "NO FLOW";
    case BU_NEAREND: return "NEXT BAG";
    case BU_DONE:    return "COMPLETE";
    case BU_PAUSED:  return "PAUSED";
    default:         return "OFFLINE";
  }
}

bool bedIsAlarm(BedUiStatus s) {
  return s == BU_FAST || s == BU_SLOW || s == BU_NOFLOW || s == BU_DONE;
}


void textRight(const String &s, int xRight, int y, uint8_t size, uint16_t col, uint16_t bg) {
  textAt(s, xRight - (int)s.length() * 6 * size, y, size, col, bg);
}

void textCenterIn(const String &s, int x, int w, int y, uint8_t size, uint16_t col, uint16_t bg) {
  textAt(s, x + (w - (int)s.length() * 6 * size) / 2, y, size, col, bg);
}

String padTo(const String &s, unsigned int width) {
  String out = s;
  while (out.length() < width) out += ' ';
  return out;
}




// ---- ข้อมูลของเตียงที่ใช้บ่อย ----
int bedPct(int i) {
  const StationData &s = stations[i];
  if (s.cfg.planVolumeMl <= 0) return -1;
  int p = (int)(s.totalVolumeMl * 100.0f / s.cfg.planVolumeMl + 0.5f);
  return (p > 100) ? 100 : p;
}

int bedMinutesLeft(int i) {
  const StationData &s = stations[i];
  if (s.cfg.planVolumeMl <= 0 || !isStationOnline(i) || !s.isRunning) return -1;
  if (s.alertCode == ALERT_OCCLUSION) return -1;   // สายพับ/ไม่มีการไหล คำนวณเวลาที่เหลือไม่ได้
  float left = s.cfg.planVolumeMl - s.totalVolumeMl;
  if (left < 0) left = 0;
  float rate = (s.flowRate_ml_hr > 5.0f) ? s.flowRate_ml_hr : s.cfg.targetRateHr;
  if (rate <= 0) return -1;
  return (int)((left / rate) * 60.0f);
}

// เตียงที่ควรถูกเน้น: เหตุวิกฤตมาก่อน ตามด้วยใกล้หมดถุง ไม่มีเลยจึงใช้เตียงที่ผู้ใช้เลือกไว้
int autoFocusBed() {
  for (int i = 0; i < activeStationCount; i++) if (bedIsAlarm(bedUiStatus(i))) return i;
  for (int i = 0; i < activeStationCount; i++)
    if (stations[i].alertCode == ALERT_NEAR_END && !stations[i].nearEndAck) return i;
  return -1;
}

int currentFocusBed() {
  int a = autoFocusBed();
  if (a >= 0) return a;
  if (manualFocusBed >= 0 && manualFocusBed < activeStationCount) return manualFocusBed;
  return 0;
}

// ---------------------------------------------------------------- ผังหน้าจอ
#define SCR_W       320
#define SCR_H       240
#define TOPBAR_H    24
#define PANEL_X     5
#define PANEL_Y     30
#define PANEL_W     190
#define PANEL_H     188
#define SIDE_X      200
#define SIDE_W      115
#define BOTBAR_Y    222
#define BOTBAR_H    18

void resetUiCaches() {
  cacheClock = ""; cacheTopRight = ""; cacheFocusHead = ""; cacheStatusWord = "";
  cacheRate = ""; cacheSetRate = ""; cacheSummary = ""; cacheBottom = ""; cacheAlarmRate = "";
  cachePct = -999;
  for (int i = 0; i < MAX_SUPPORTED_STATIONS; i++) cacheSide[i] = "";
  uiFocusDrawn = -1;
  uiRowsDrawn = -1;
}


void updateTopBar() {
  String clockStr = getDisplayClockStr();
  if (clockStr != cacheClock) {
    cacheClock = clockStr;
    textCenterIn(clockStr, 0, SCR_W, 6, 2, isTimeApprox ? C_YELLOW : C_WHITE, C_TOPBAR);
  }
  // [1.0.0-TG] แก้: แท่งสัญญาณเราเตอร์ (เดิมคือจำนวนมือถือที่ต่อ AP) สีเขียวเมื่อพีซีรับข้อมูลอยู่ ไม่งั้นสีเทา
  int bars = 0;
  if (netMode == NET_MODE_STA && lastStaRssi != 0) {
    if (lastStaRssi >= -60) bars = 4; else if (lastStaRssi >= -70) bars = 3;
    else if (lastStaRssi >= -80) bars = 2; else bars = 1;
  }
  // [1.1.0-TG] เพิ่ม: เครื่องหมาย ! ข้างไอคอนแบตเตอรี่เมื่อแบตเตอรี่ของ Host ต่ำ (วิกฤตกะพริบครึ่งวินาที)
  bool batBlink = (hostBat.level == BAT_CRIT) && (((millis() / 500) % 2) == 1);
  String right = String(bars) + (pcLinkUp ? "P" : "-") + ":" + String(hostBatteryPct / 5) + ":" +
                 String((int)hostBat.level) + (batBlink ? "b" : "");
  if (right != cacheTopRight) {
    cacheTopRight = right;
    drawWifiIcon(SCR_W - 52, 6, pcLinkUp ? bars : 0);
    drawBatteryIcon(SCR_W - 30, 7, hostBatteryPct);
    tft.fillRect(SCR_W - 70, 4, 14, 16, C_TOPBAR);
    if (hostBat.isLow() && !batBlink)
      textAt("!", SCR_W - 66, 6, 2, (hostBat.level == BAT_CRIT) ? C_RED : C_YELLOW, C_TOPBAR);
  }
}






void updateMainScreen() {
  int focus = currentFocusBed();
  bool focusChanged = (focus != uiFocusDrawn);
  uiFocusDrawn = focus;
  updateTopBar();
  drawFocusPanel(focus, focusChanged);
  updateSideList(focus, focusChanged);
  updateBottomBar();
}




void updateSaverScreen() {
  String clockStr = getDisplayClockStr();
  if (clockStr != cacheClock) {
    cacheClock = clockStr;
    textCenterIn(clockStr, 0, SCR_W, 76, 7, C_WHITE, C_BG);
  }
  int online = 0;
  for (int i = 0; i < activeStationCount; i++) if (isStationOnline(i)) online++;
  char buf[40];
  snprintf(buf, sizeof(buf), "ONLINE %d/%d   ALL NORMAL", online, activeStationCount);
  // [1.0.0-TG] เพิ่ม: บรรทัดสถานะลิงก์พีซีใต้ข้อความเดิม (ASCII ล้วน)
  String pcLine = gatewayLine();
  if (pcLine != cacheSide[0]) {
    cacheSide[0] = pcLine;
    textCenterIn(padTo(pcLine, 26), 0, SCR_W, 176, 1, pcLinkUp ? C_DIM : C_ORANGE, C_BG);
  }
  String line = String(buf);
  if (line != cacheBottom) {
    cacheBottom = line;
    textCenterIn(line, 0, SCR_W, 156, 1, C_GREEN, C_BG);
  }
}

// ---- ตัวควบคุมการแสดงผลทั้งหมด ----
void updateHostDisplay() {
  if (displaySleeping || isPowerOffProgressActive) return;

  bool nearPending = anyNearEndPending();
  if (!screensaverActive && !globalAlarmTriggered && !nearPending &&
      (millis() - lastUserActivityTime >= SCREENSAVER_IDLE_MS)) {
    screensaverActive = true;
  }
  // มีเหตุวิกฤต (ยังไม่พักเสียง) หรือเตือนใกล้หมดที่ยังไม่รับทราบ -> ออกจาก Screensaver
  if (screensaverActive && ((globalAlarmTriggered && !isSnoozed()) || nearPending)) {
    screensaverActive = false;
  }

  int mode = UI_MODE_MAIN;
  if (screensaverActive) mode = UI_MODE_SAVER;
  else if (globalAlarmTriggered && !isSnoozed()) mode = UI_MODE_ALARM;

  if (mode != uiMode) {
    uiMode = mode;
    resetUiCaches();
    if (mode == UI_MODE_SAVER)      drawSaverFramework();
    else if (mode == UI_MODE_ALARM) drawAlarmFramework();
    else                            drawMainFramework();
  }

  if (mode == UI_MODE_SAVER)      updateSaverScreen();
  else if (mode == UI_MODE_ALARM) updateAlarmScreen();
  else                            updateMainScreen();
}





// ---------------------------------------------------------
// ส่งผลคำนวณกลับให้ Station แสดง (Protocol v4 ลง) — เฉลี่ยช่องเวลา ไม่ใช้ delay()
// ---------------------------------------------------------
// [1.0.0-TG] ย้าย: แทน broadcastSyncToNodes() เดิมที่ส่งรวดเดียวพร้อม delay(4) ต่อเตียงใน loop()
// ส่งทีละใบห่างกันเท่า ๆ กันตลอดวินาที แต่ละเตียงยังได้รับครบหนึ่งใบต่อวินาที
// เรียงลำดับไว้ท้ายส่วนตรรกะ เพราะเรียก bedPct/bedMinutesLeft ที่นิยามไว้ด้านบนแล้ว
uint16_t syncSlotIntervalMs() {
  uint8_t n = (activeStationCount < 1) ? 1 : activeStationCount;
  uint16_t slot = SYNC_INTERVAL_MS / n;
  if (slot < SYNC_SLOT_MIN_MS) slot = SYNC_SLOT_MIN_MS;
  return slot;
}

void sendDisplayForStation(int i) {
  if (i < 0 || i >= MAX_SUPPORTED_STATIONS) return;
  time_t now;
  time(&now);
  const StationData &s = stations[i];
  bool online = isStationOnline(i);
  // [1.0.0-TG] เพิ่ม: สายพับ = อัตราจริงเป็น 0 ตัดที่ตรงนี้ด้วย ไม่พึ่งแค่ตัวล้างค่าในลูปรายสองวินาที
  // เพราะแพ็กเก็ตที่ส่งลงต้องไม่พาอัตราเก่าที่ไม่ไหลแล้วไปแสดงบนจอเตียงที่กำลังขึ้น NO FLOW
  bool occluded = (s.alertCode == ALERT_OCCLUSION);

  struct_down_v4 d = {};
  d.magic      = V4_MAGIC_DOWN;
  d.stationId  = (uint8_t)(i + 1);
  d.flags      = (s.nearEndAck ? V4_DOWN_NEAR_ACK : 0) |
                 (online ? V4_DOWN_HEARS_YOU : 0) |
                 ((now > 1600000000) ? V4_DOWN_TIME_OK : 0) |
                 ((s.cfg.planVolumeMl > 0) ? V4_DOWN_PLAN_SET : 0) |
                 ((online && s.isRunning && s.rateValid && !occluded) ? V4_DOWN_RATE_OK : 0);
  d.alertCode  = alertCodeForStation(s.alertCode);   // รหัสภายใน Host ที่ Station ไม่รู้จักถูกแปลงเป็น 0
  d.resetSeq   = s.cfg.resetSeq;
  d.dropFactor = safeDropFactor(s.cfg.dropFactor);
  d.nearEndPct = safeNearPct(s.nearEndPct);
  d.hostChannel = currentWifiChannel();
  d.epochTime  = (now > 1600000000) ? (uint32_t)now : 0;
  d.rateHr     = (online && s.isRunning && !occluded) ? s.flowRate_ml_hr : 0.0f;
  d.targetRateHr = s.cfg.targetRateHr;
  d.planMl     = s.cfg.planVolumeMl;
  d.infusedMl  = s.totalVolumeMl;
  int m = bedMinutesLeft(i);
  d.minutesLeft = (m < 0) ? V4_MINUTES_UNKNOWN : (uint16_t)((m > 65534) ? 65534 : m);
  int p = bedPct(i);
  d.infusedPct = (p < 0) ? 0 : (uint8_t)p;

  // ไม่ส่งซ้ำและไม่ delay() เมื่อคิวส่งเต็ม เพราะรอบถัดไปมาถึงในอีกไม่กี่สิบมิลลิวินาทีอยู่แล้ว
  if (esp_now_send(broadcastAddress, (uint8_t *)&d, sizeof(d)) != ESP_OK) syncSendFail++;
}

// ขอให้ส่งผลของเตียงนี้ในช่องเวลาถัดไป (ใช้ตอนค่าตั้งเปลี่ยนหรือรหัสเตือนเปลี่ยน)
void requestSyncNow(int idx) {
  if (idx < 0 || idx >= MAX_SUPPORTED_STATIONS) return;
  pendingSyncMask |= (uint16_t)(1u << idx);
}

void requestSyncAll() {
  for (int i = 0; i < activeStationCount; i++) requestSyncNow(i);
}

// เรียกทุกรอบของ loop() — ส่งได้มากสุด 1 ใบต่อหนึ่งช่องเวลา
void serviceSyncScheduler() {
  unsigned long now = millis();
  if (now - lastSyncSlotTime < syncSlotIntervalMs()) return;
  lastSyncSlotTime = now;

  if (pendingSyncMask != 0) {                    // งานด่วนมาก่อน
    for (int i = 0; i < MAX_SUPPORTED_STATIONS; i++) {
      if (pendingSyncMask & (1u << i)) {
        pendingSyncMask &= (uint16_t)~(1u << i);
        sendDisplayForStation(i);
        return;
      }
    }
  }

  if (activeStationCount < 1) return;
  if (syncRoundIdx >= activeStationCount) syncRoundIdx = 0;
  sendDisplayForStation(syncRoundIdx);
  syncRoundIdx = (uint8_t)((syncRoundIdx + 1) % activeStationCount);
}

// ปรับคุณภาพลิงก์ทุก 10 วินาที: ได้รับกี่ใบจาก 10 ใบที่ควรได้
void serviceLinkQuality() {
  unsigned long now = millis();
  if (now - lastLinkWindowTime < LINK_WINDOW_MS) return;
  lastLinkWindowTime = now;

  const uint16_t expected = (uint16_t)(LINK_WINDOW_MS / SYNC_INTERVAL_MS);
  portENTER_CRITICAL(&dataMux);
  for (int i = 0; i < MAX_SUPPORTED_STATIONS; i++) {
    uint16_t got = stations[i].rxWindowCount;
    stations[i].rxWindowCount = 0;
    uint16_t pct = (uint16_t)((got * 100UL) / expected);
    stations[i].linkPct = (uint8_t)((pct > 100) ? 100 : pct);
  }
  portEXIT_CRITICAL(&dataMux);
}

// ---------------------------------------------------------
// ตั้งค่าครั้งแรก (Setup Portal) — เปิดเฉพาะตอนยังไม่มีรหัสเราเตอร์
// ---------------------------------------------------------
// [1.0.0-TG] เพิ่ม: เว็บเซิร์ฟเวอร์ตัวเดียวที่เหลืออยู่ในเครื่องนี้ ทำงานเฉพาะโหมดตั้งค่า
// กรอกชื่อ Wi-Fi รหัสผ่าน และที่อยู่พีซี แล้วรีบูตเข้าโหมดปกติ ไม่มี API ไม่มีหน้า Dashboard
// ในโหมดปกติ server ไม่ถูก begin() และ loop() ไม่เรียก handleClient() จึงไม่มีภาระเลย
String htmlEscape(const String &in) {
  String o;
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '&') o += "&amp;";
    else if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;";
    else if (c == '"') o += "&quot;";
    else o += c;
  }
  return o;
}

void handlePortalRoot() {
  String h = "<!doctype html><html><head><meta charset=utf-8>"
             "<meta name=viewport content='width=device-width,initial-scale=1'>"
             "<title>Smart IV Alert - Gateway Setup</title>"
             "<style>body{font-family:sans-serif;max-width:420px;margin:20px auto;padding:0 14px}"
             "input{width:100%;padding:10px;margin:4px 0 12px;box-sizing:border-box;font-size:16px}"
             "button{padding:12px 20px;font-size:16px}</style></head><body>"
             "<h2>Smart IV Alert Gateway " APP_VERSION "</h2>"
             "<form method=POST action=/save>"
             "<label>ชื่อ Wi-Fi (SSID)</label><input name=ssid maxlength=32 value=\"";
  h += htmlEscape(staSsid);
  h += "\"><label>รหัสผ่าน Wi-Fi</label><input name=pass type=password maxlength=63>"
       "<label>ที่อยู่พีซี (IP)</label><input name=pc maxlength=63 placeholder=192.168.1.10 value=\"";
  h += htmlEscape(pcHost);
  h += "\"><label>พอร์ต</label><input name=port type=number min=1 max=65535 value=\"";
  h += String(pcPort);
  h += "\"><label>รหัสเกตเวย์ (ถ้าตั้งไว้ที่พีซี)</label><input name=token maxlength=40 value=\"";
  h += htmlEscape(pcToken);
  h += "\"><button>บันทึกและรีสตาร์ต</button></form></body></html>";
  server.send(200, "text/html; charset=utf-8", h);
}

void handlePortalSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  String pc   = server.arg("pc");
  int    port = server.arg("port").toInt();
  String tok  = server.arg("token");
  if (ssid.length() == 0 || ssid.length() > 32 || pass.length() > 63 ||
      pc.length() == 0 || pc.length() > 63 || port < 1 || port > 65535 || tok.length() > 40) {
    server.send(400, "text/plain; charset=utf-8", "ข้อมูลไม่ถูกต้อง / Bad input");
    return;
  }
  saveWifiConfig(ssid, pass);
  savePcConfig(pc, (uint16_t)port, tok);
  server.send(200, "text/plain; charset=utf-8", "OK - restarting");
  delay(400);                          // ให้คำตอบออกไปให้พ้นก่อนรีบูต
  ESP.restart();
}

// ---------------------------------------------------------
// คำสั่งจากพีซี ([1.0.0-TG] ย้าย: เดิมคือ handleStationConfig/Reset/Ack/SetStationCount)
// ---------------------------------------------------------
// พีซีส่งคำสั่งมาในคำตอบของคำขอ POST /api/ingest เป็นข้อความทีละบรรทัด
//   T <epoch>                                  เวลาปัจจุบัน (UTC)
//   C <seq> <เตียง> rate= vol= df= near= name=  ตั้งค่าเตียง (ส่งเฉพาะคีย์ที่เปลี่ยน name เข้ารหัส %XX)
//   R <seq> <เตียง>                             เริ่มถุงใหม่
//   A <seq> <เตียง>                             รับทราบเตือน
//   N <seq> 0 count=<n>                        จำนวนเตียงที่เปิดใช้
// ทุกคำสั่งมีเลขลำดับ Host รายงานเลขล่าสุดที่ทำแล้วกลับไปทุกครั้ง (ack) พีซีส่งซ้ำจนกว่าจะได้ ack
// คำสั่งจึงไม่หายแม้ลิงก์ขาดกลางทาง และไม่ถูกทำซ้ำเพราะ Host ข้ามเลขที่เคยรับแล้ว
// เธรดส่งเป็นคนแกะข้อความแล้วใส่คิว loop() เป็นคนลงมือทำ เพราะการแก้ stations[] และ NVS
// ต้องอยู่เธรดเดียวกับตรรกะเตือน
struct HostCmd {
  uint32_t seq;
  char     type;
  uint8_t  station;
  char     args[CMD_ARGS_MAX];
};

HostCmd           cmdQueue[CMD_QUEUE_LEN];
uint8_t           cmdCount        = 0;       // เข้าถึงใต้ dataMux เท่านั้น
volatile uint32_t lastCmdEnqueued = 0;       // เลขที่ใหญ่ที่สุดที่รับเข้าคิวแล้ว (กันเข้าคิวซ้ำ)
volatile uint32_t lastCmdApplied  = 0;       // เลขที่ทำเสร็จแล้ว = ค่า ack ที่ส่งกลับพีซี
volatile uint32_t pendingEpoch    = 0;       // เวลาจากพีซีที่รอ loop() ไปตั้งนาฬิกา

// ถอดรหัส %XX กลับเป็นไบต์ ใช้กับชื่อผู้ป่วยซึ่งเป็น UTF-8
void urlDecode(const char *in, size_t inLen, char *out, size_t outCap) {
  size_t o = 0;
  for (size_t i = 0; i < inLen && o + 1 < outCap; i++) {
    char c = in[i];
    if (c == '%' && i + 2 < inLen && isxdigit((unsigned char)in[i + 1]) &&
        isxdigit((unsigned char)in[i + 2])) {
      char hex[3] = { in[i + 1], in[i + 2], 0 };
      out[o++] = (char)strtol(hex, NULL, 16);
      i += 2;
    } else {
      out[o++] = c;
    }
  }
  out[o] = '\0';
}

// แกะ args แบบ "key=value key=value ..." หาค่าของ key ที่ต้องการ คืน false ถ้าไม่มี
bool cmdArg(const char *args, const char *key, char *out, size_t outCap) {
  size_t klen = strlen(key);
  const char *p = args;
  while (*p) {
    while (*p == ' ') p++;
    const char *end = p;
    while (*end && *end != ' ') end++;
    if ((size_t)(end - p) > klen && strncmp(p, key, klen) == 0 && p[klen] == '=') {
      urlDecode(p + klen + 1, (size_t)(end - p) - klen - 1, out, outCap);
      return true;
    }
    p = end;
  }
  return false;
}

// ใส่คำสั่งเข้าคิว เรียกจากเธรดส่ง คืน false ถ้าคิวเต็ม (พีซีจะส่งซ้ำรอบหน้า)
bool enqueueCmd(char type, uint32_t seq, uint8_t station, const char *args) {
  bool ok = false;
  portENTER_CRITICAL(&dataMux);
  if (seq > lastCmdEnqueued && cmdCount < CMD_QUEUE_LEN) {
    HostCmd &c = cmdQueue[cmdCount++];
    c.seq = seq;
    c.type = type;
    c.station = station;
    strncpy(c.args, args, CMD_ARGS_MAX - 1);
    c.args[CMD_ARGS_MAX - 1] = '\0';
    lastCmdEnqueued = seq;
    ok = true;
  }
  portEXIT_CRITICAL(&dataMux);
  return ok;
}

// แกะคำตอบของพีซีทีละบรรทัด เรียกจากเธรดส่ง
void parsePcReply(const String &body) {
  int pos = 0;
  while (pos < (int)body.length()) {
    int nl = body.indexOf('\n', pos);
    if (nl < 0) nl = body.length();
    String line = body.substring(pos, nl);
    pos = nl + 1;
    line.trim();
    if (line.length() < 3) continue;
    char type = line[0];
    if (type == 'T') {
      uint32_t ep = (uint32_t)strtoul(line.c_str() + 2, NULL, 10);
      if (ep > 1600000000UL) pendingEpoch = ep;
      continue;
    }
    if (type != 'C' && type != 'R' && type != 'A' && type != 'N') continue;
    char *endp = NULL;
    const char *p = line.c_str() + 2;
    uint32_t seq = (uint32_t)strtoul(p, &endp, 10);
    if (endp == p || seq == 0) continue;
    p = endp;
    long st = strtol(p, &endp, 10);
    if (endp == p || st < 0 || st > MAX_SUPPORTED_STATIONS) continue;
    while (*endp == ' ') endp++;
    enqueueCmd(type, seq, (uint8_t)st, endp);
  }
}

// ---- ลงมือทำคำสั่ง (เรียกจาก loop() เท่านั้น) ----
// ตรวจช่วงค่าซ้ำอีกชั้นแม้พีซีตรวจมาแล้ว เพราะ Host เป็นด่านสุดท้ายก่อนค่าไปถึงเตียง
void applyStationConfig(int idx, const char *args) {
  char buf[96];
  BedConfig &c = stations[idx].cfg;
  float oldPlan = c.planVolumeMl;       // จำค่าเดิมไว้ก่อนเขียนทับ (บทเรียนจาก 4.7.5)
  bool nearChanged = false;

  if (cmdArg(args, "rate", buf, sizeof(buf))) {
    float r = atof(buf);
    if (r >= 0 && r <= 1000) c.targetRateHr = r;
  }
  if (cmdArg(args, "vol", buf, sizeof(buf))) {
    float v = atof(buf);
    if (v >= 0 && v <= 10000) c.planVolumeMl = v;
  }
  if (cmdArg(args, "df", buf, sizeof(buf))) {
    int df = atoi(buf);
    if (df == 10 || df == 15 || df == 20 || df == 60) c.dropFactor = (uint8_t)df;
  }
  if (cmdArg(args, "near", buf, sizeof(buf))) {
    int np = atoi(buf);
    if (np >= 50 && np <= 95) {
      if (np != stations[idx].nearEndPct) nearChanged = true;
      stations[idx].nearEndPct = (uint8_t)np;
    }
  }
  char nameBuf[sizeof(c.patientName)];
  if (cmdArg(args, "name", nameBuf, sizeof(nameBuf))) {
    strncpy(c.patientName, nameBuf, sizeof(c.patientName) - 1);
    c.patientName[sizeof(c.patientName) - 1] = '\0';
  }
  saveBedConfig(idx);

  portENTER_CRITICAL(&dataMux);
  StationData &s = stations[idx];
  s.totalVolumeMl = (float)s.totalDrops / (float)safeDropFactor(c.dropFactor);
  s.deviationSince = 0;
  if (nearChanged || c.planVolumeMl != oldPlan) { s.nearEndAck = false; s.nearNextChime = 0; }
  s.trialCase.active = c.planVolumeMl > 0;
  s.trialCase.planVolume = c.planVolumeMl;
  s.trialCase.targetRate = c.targetRateHr;
  portEXIT_CRITICAL(&dataMux);

  evaluateClinicalAlerts();
  requestSyncNow(idx);          // ส่งเฉพาะเตียงนี้ในช่องเวลาถัดไป
}

void applyStationReset(int idx) {
  String nowStr = getFormattedDateTime();
  portENTER_CRITICAL(&dataMux);
  StationData &s = stations[idx];
  s.cfg.resetSeq++;
  s.hasBaseline = false;          // แพ็กเก็ตถัดไปใช้เป็นฐานใหม่ ไม่นับเป็นหยดของนาทีนี้
  s.totalDrops = 0;
  s.totalVolumeMl = 0;
  s.flowRate_ml_hr = 0;
  s.msSinceLastDrop = 0;
  s.dropsInCurrentMinute = 0;
  s.est.clearAll();               // [1.0.0-TG] เพิ่ม: เริ่มถุงใหม่ = ล้างตัวคำนวณอัตราไหลด้วย
  s.rateValid = false;
  s.alertCode = ALERT_NONE;
  s.deviationSince = 0;
  s.nearEndAck = false;
  s.nearNextChime = 0;
  s.trialCase.caseNumber++;
  s.trialCase.actualVolume = 0;
  s.trialCase.errorMl = 0;
  s.trialCase.errorPercent = 0;
  portEXIT_CRITICAL(&dataMux);
  snprintf(stations[idx].trialCase.startTimeStr, sizeof(stations[idx].trialCase.startTimeStr), "%s", nowStr.c_str());

  saveBedConfig(idx);
  requestSyncNow(idx);          // Station ต้องเห็น resetSeq ใหม่ทันที
}

void applyStationAck(int idx) {
  stations[idx].nearEndAck = true;
  stations[idx].lostLinkAck = true;   // ปุ่มรับทราบเดิมใช้ปิดเสียงเตียงที่ขาดการติดต่อได้ด้วย
  requestSyncNow(idx);
}

void applyStationCount(int count) {
  if (count < 1 || count > MAX_SUPPORTED_STATIONS) return;
  activeStationCount = count;
  preferences.begin("sys-config", false);
  preferences.putUChar("bedCount", activeStationCount);
  preferences.end();
  if (manualFocusBed >= activeStationCount) manualFocusBed = -1;   // [1.0.0-TG] แก้: จอ TFT ใช้เตียงที่เน้น ไม่ใช่หมายเลขหน้า
  if (syncRoundIdx >= activeStationCount) syncRoundIdx = 0;
  requestSyncAll();             // เตียงที่เพิ่งเปิดใช้จะได้ค่าตั้งทันที
}

// ตั้งนาฬิกาตามพีซี (พีซีเป็นเจ้าของเวลา ไม่ต้องให้พยาบาลเปิดเว็บเพื่อซิงก์เวลาอีก)
void applyPcTime(uint32_t epoch) {
  time_t now;
  time(&now);
  long drift = (long)now - (long)epoch;
  if (drift < 0) drift = -drift;
  if (isTimeSynced && drift <= 2) return;      // ตรงอยู่แล้ว ไม่ต้องกระตุกนาฬิกา
  struct timeval tv = { (time_t)epoch, 0 };
  settimeofday(&tv, NULL);
  bool first = !isTimeSynced;
  isTimeSynced = true;
  isTimeApprox = false;
  if (first) { backupClockToNvs(); lastTimeBackup = millis(); }
}

// เรียกทุกรอบของ loop()
void serviceCommands() {
  uint32_t ep = pendingEpoch;
  if (ep) { pendingEpoch = 0; applyPcTime(ep); }

  for (;;) {
    HostCmd c;
    bool have = false;
    portENTER_CRITICAL(&dataMux);
    if (cmdCount > 0) {
      c = cmdQueue[0];
      memmove(&cmdQueue[0], &cmdQueue[1], sizeof(HostCmd) * (cmdCount - 1));
      cmdCount--;
      have = true;
    }
    portEXIT_CRITICAL(&dataMux);
    if (!have) break;

    int idx = (int)c.station - 1;
    bool bedOk = (idx >= 0 && idx < MAX_SUPPORTED_STATIONS);
    if      (c.type == 'C' && bedOk) applyStationConfig(idx, c.args);
    else if (c.type == 'R' && bedOk) applyStationReset(idx);
    else if (c.type == 'A' && bedOk) applyStationAck(idx);
    else if (c.type == 'N') {
      char buf[8];
      if (cmdArg(c.args, "count", buf, sizeof(buf))) applyStationCount(atoi(buf));
    }
    lastCmdApplied = c.seq;       // ack ไปกับสถานะรอบถัดไป แม้คำสั่งไม่ผ่านการตรวจก็ต้อง ack ไม่งั้นพีซีส่งซ้ำไม่จบ
  }
}

// ---------------------------------------------------------
// ลิงก์ขึ้นพีซี: เธรดแยก ไม่แตะ loop()
// ---------------------------------------------------------
// [1.0.0-TG] เพิ่ม: งานที่เคยกิน loop() ของ Host-OLED (สร้าง JSON ก้อนใหญ่ ตอบเว็บทีละเครื่อง)
// ย้ายมาอยู่เธรดนี้ทั้งหมด และเหลือแค่ก้อนเดียววินาทีละครั้ง ไม่ว่าจะมีกี่คนเปิดหน้าเว็บ
// เธรดนี้บล็อกบนเครือข่ายได้ (มี timeout) โดยไม่ทำให้การรับ ESP-NOW การเตือน หรือจอสะดุด
// เพราะ loop() อยู่คนละเธรดและไม่รอเธรดนี้เลย
void snapBeds(SnapBed *out, uint8_t n) {
  portENTER_CRITICAL(&dataMux);
  for (uint8_t i = 0; i < n; i++) {
    const StationData &s = stations[i];
    SnapBed &b = out[i];
    b.online          = isStationOnline(i);
    b.running         = s.isRunning;
    b.idConflict      = false;   // [1.0.0-TG] แก้: Host จอ TFT ยังไม่ตรวจเลขเตียงซ้ำ (ตามต้นฉบับ Host-TFT28)
    b.macKnown        = s.macKnown;
    memcpy(b.mac, s.srcMac, 6);
    b.rssi            = s.rssi;
    b.link            = s.linkPct;
    b.dropFactor      = safeDropFactor(s.cfg.dropFactor);
    b.alertCode       = s.alertCode;
    b.nearEndPct      = safeNearPct(s.nearEndPct);
    b.nearEndAck      = s.nearEndAck;
    b.caseActive      = s.trialCase.active;
    b.rxTotal         = s.rxTotal;
    b.totalDrops      = s.totalDrops;
    b.msSinceLastDrop = s.msSinceLastDrop;
    b.battery         = s.batteryVolts;
    b.batLevel        = s.bat.level;
    b.volumeMl        = s.totalVolumeMl;
    b.flowRateHr      = s.flowRate_ml_hr;
    b.targetRate      = s.cfg.targetRateHr;
    b.planVolume      = s.cfg.planVolumeMl;
    memcpy(b.patientName, s.cfg.patientName, sizeof(b.patientName));
    b.patientName[sizeof(b.patientName) - 1] = '\0';
    memcpy(b.caseStart, s.trialCase.startTimeStr, sizeof(b.caseStart));
    b.caseStart[sizeof(b.caseStart) - 1] = '\0';
  }
  portEXIT_CRITICAL(&dataMux);
}

// ชื่อฟิลด์ตรงกับที่หน้าเว็บเดิมของ Host-OLED อ่าน (พีซีส่งต่อให้หน้าเว็บได้เลย)
String buildUplinkJson() {
  uint8_t n = activeStationCount;
  if (n > MAX_SUPPORTED_STATIONS) n = MAX_SUPPORTED_STATIONS;
  SnapBed beds[MAX_SUPPORTED_STATIONS];
  snapBeds(beds, n);

  String json;
  json.reserve(3400);
  json  = "{\"v\":\"" APP_VERSION "\",";
  json += "\"host\":\"" + hostId + "\",";
  json += "\"ack\":" + String((uint32_t)lastCmdApplied) + ",";
  json += "\"up\":" + String((uint32_t)millis()) + ",";
  json += "\"resetReason\":\"" + String(resetReasonText(bootResetReason)) + "\",";
  json += "\"count\":" + String(n) + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"rssi\":" + String(lastStaRssi) + ",";
  json += "\"channel\":" + String(currentWifiChannel()) + ",";
  json += "\"bat\":" + String(hostBatteryVolts, 2) + ",";
  json += "\"batPct\":" + String(hostBatteryPct) + ",";
  json += "\"batLevel\":" + String(hostBat.level) + ",";
  json += "\"snooze\":" + String(isSnoozed() ? "true" : "false") + ",";
  json += "\"linkWeakPct\":" + String(LINK_WEAK_PCT) + ",";
  json += "\"syncFail\":" + String(syncSendFail) + ",";
  json += "\"heardBeyond\":0,";
  json += "\"pushFail\":" + String((uint32_t)pcPushFail) + ",";
  json += "\"stations\":[";
  for (uint8_t i = 0; i < n; i++) {
    const SnapBed &b = beds[i];
    char mac[12] = "-";
    if (b.macKnown) snprintf(mac, sizeof(mac), "%02X:%02X:%02X", b.mac[3], b.mac[4], b.mac[5]);
    if (i > 0) json += ",";
    json += "{";
    json += "\"id\":" + String(i + 1) + ",";
    json += "\"online\":" + String(b.online ? "true" : "false") + ",";
    json += "\"running\":" + String(b.running ? "true" : "false") + ",";
    json += "\"rssi\":" + String(b.online ? b.rssi : 0) + ",";
    json += "\"link\":" + String(b.link) + ",";
    json += "\"idConflict\":" + String(b.idConflict ? "true" : "false") + ",";
    json += "\"mac\":\"" + String(mac) + "\",";
    json += "\"rxTotal\":" + String(b.rxTotal) + ",";
    json += "\"battery\":" + String(b.battery, 2) + ",";
    json += "\"batLevel\":" + String(b.batLevel) + ",";
    json += "\"totalDrops\":" + String(b.totalDrops) + ",";
    json += "\"volumeMl\":" + String(b.volumeMl, 2) + ",";
    json += "\"flowRateHr\":" + String(b.flowRateHr, 2) + ",";
    json += "\"msSinceLastDrop\":" + String(b.msSinceLastDrop) + ",";
    json += "\"targetRate\":" + String(b.targetRate, 1) + ",";
    json += "\"planVolume\":" + String(b.planVolume, 0) + ",";
    json += "\"dropFactor\":" + String(b.dropFactor) + ",";
    json += "\"patientName\":\"" + jsonEscape(b.patientName) + "\",";
    json += "\"alertCode\":" + String(b.alertCode) + ",";
    json += "\"nearEndPct\":" + String(b.nearEndPct) + ",";
    json += "\"nearEndAck\":" + String(b.nearEndAck ? "true" : "false") + ",";
    json += "\"caseActive\":" + String(b.caseActive ? "true" : "false") + ",";
    json += "\"caseStart\":\"" + jsonEscape(b.caseStart) + "\"";
    json += "}";
  }
  json += "]}";
  return json;
}

// ส่งหนึ่งรอบ คืน true เมื่อพีซีตอบ 200 และแกะคำตอบแล้ว
bool pushToPc() {
  static WiFiClient uplinkClient;       // ใช้ซ้ำข้ามรอบ (keep-alive) เพื่อไม่ต้องจับมือ TCP ทุกวินาที
  static HTTPClient uplinkHttp;

  String url = "http://" + pcHost + ":" + String(pcPort) + "/api/ingest";
  uplinkHttp.setReuse(true);
  uplinkHttp.setConnectTimeout(PC_CONNECT_TIMEOUT_MS);
  uplinkHttp.setTimeout(PC_REPLY_TIMEOUT_MS);
  if (!uplinkHttp.begin(uplinkClient, url)) return false;
  uplinkHttp.addHeader("Content-Type", "application/json");
  if (pcToken.length() > 0) uplinkHttp.addHeader("X-Gateway-Token", pcToken);

  int code = uplinkHttp.POST(buildUplinkJson());
  bool ok = (code == 200);
  if (ok) parsePcReply(uplinkHttp.getString());
  uplinkHttp.end();
  return ok;
}

void uplinkTask(void *) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(PC_PUSH_INTERVAL_MS));
    if (portalActive || !pcConfigured || WiFi.status() != WL_CONNECTED) {
      pcLinkUp = false;
      continue;
    }
    if (pushToPc()) {
      pcPushOk++;
      lastPcOkAt = millis();
      pcLinkUp = true;
    } else {
      pcPushFail++;
      if (millis() - lastPcOkAt > PC_LINK_STALE_MS) pcLinkUp = false;
    }
  }
}


// ---------------------------------------------------------------------------
#include "HostScreen.h"

// ==========================================
//  Setup & Initialization
// ==========================================
void setup() {
  Serial.begin(115200);

  rtc_gpio_deinit((gpio_num_t)POWER_BTN_PIN);   // คืนขาจากโหมด RTC หลังตื่นจาก deep sleep
  pinMode(POWER_BTN_PIN, INPUT_PULLUP);
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  bool bootClearRequested = false;           // [1.2.0-TG] เพิ่ม: กดค้างครบเวลาตอนปลุกเครื่อง

  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0 || wakeup_reason == ESP_SLEEP_WAKEUP_GPIO) {
    unsigned long pressStart = millis();
    bool validHoldToTurnOn = false;

    while (digitalRead(POWER_BTN_PIN) == LOW) {
      feedWatchdog();
      if (millis() - pressStart >= 1500) {
        validHoldToTurnOn = true;
        break;
      }
      delay(10);
    }

    if (!validHoldToTurnOn) {
      while (digitalRead(POWER_BTN_PIN) == LOW) { feedWatchdog(); delay(10); }
      delay(100);
      enterDeepSleepWaitPowerButton();
    }

    // [1.2.0-TG] เพิ่ม: กดค้างต่อจนครบ SETUP_HOLD_MS ตอนปลุกเครื่อง = ขอล้างค่าเครือข่าย (ทำหลังจอพร้อมแล้ว)
    while (digitalRead(POWER_BTN_PIN) == LOW) {
      feedWatchdog();
      if (millis() - pressStart >= SETUP_HOLD_MS) bootClearRequested = true;
      delay(10);
    }
    delay(150);
  
  bootResetReason = esp_reset_reason();   // [4.9.0-TFT] เพิ่ม: จำไว้ว่ารีบูตครั้งล่าสุดเพราะอะไร
  Serial.printf("[boot] reset reason: %s\n", resetReasonText(bootResetReason));
  setupLoopWatchdog();
}

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(HOST_BAT_ADC_PIN, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

#if TFT_BLK >= 0
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);
#endif
  SPI_TFT.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(240, 320);              // ความละเอียดจริงของแผง ST7789V
  tft.setSPISpeed(40000000);
  tft.setRotation(TFT_ROTATION);   // หมุนเป็นแนวนอน 320x240
  tft.setTextWrap(false);
  tft.fillScreen(0x0000);

  hostBatteryVolts = readHostBattery();
  hostBatteryPct = calculateBatteryPct(hostBatteryVolts);

  preferences.begin("sys-config", true);
  activeStationCount = preferences.getUChar("bedCount", 5);
  uint32_t savedEpoch = preferences.getUInt("lastEpoch", 0);
  preferences.end();

  // เขตเวลาไทยถาวร แล้วกู้เวลาที่สำรองไว้ก่อนไฟดับ (จะถูกแทนที่ทันทีเมื่อเปิดหน้าเว็บ)
  setenv("TZ", TZ_THAILAND, 1);
  tzset();
  if (savedEpoch > 1600000000UL) {
    struct timeval tv = { (time_t)savedEpoch, 0 };
    settimeofday(&tv, NULL);
    isTimeApprox = true;
  }
  if (activeStationCount < 1 || activeStationCount > MAX_SUPPORTED_STATIONS) activeStationCount = 5;

  loadBedConfigs();

  // ---------------- Wi-Fi: เกาะเราเตอร์อย่างเดียว (ไม่มี AP สำรอง) ----------------
  // [1.0.0-TG] แก้: ผู้ใช้ดูข้อมูลที่พีซี ตัวเครื่องจึงไม่ต้องเป็น Access Point ให้ใคร
  // ESP-NOW ใช้ช่องของเราเตอร์ได้เลย เพราะ Station ไล่หาช่องเองอัตโนมัติ
  WiFi.persistent(false);

  // [1.2.0-TG] แก้: กดปุ่มค้างตอนเปิดเครื่อง SETUP_HOLD_MS (8 วินาที) = ล้างค่าเครือข่ายแล้วเข้าโหมดตั้งค่า
  // เป็นทางเดียวที่ตั้งค่าใหม่ได้โดยไม่ต้องต่อสาย เพราะเครื่องนี้ไม่มีหน้าเว็บตั้งค่าในโหมดปกติ
  // สองทาง: ปลุกจาก deep sleep (วัดไว้แล้วข้างบน) หรือเสียบไฟใหม่ขณะกดปุ่มค้าง (วัดตรงนี้)
  if (!bootClearRequested && digitalRead(POWER_BTN_PIN) == LOW) {
    unsigned long holdStart = millis();
    while (digitalRead(POWER_BTN_PIN) == LOW) {
      feedWatchdog();
      if (millis() - holdStart >= SETUP_HOLD_MS) { bootClearRequested = true; break; }
      delay(10);
    }
  }
  if (bootClearRequested) {
    clearWifiConfig();
    tft.fillScreen(0x0000);
    textAt("WIFI CLEARED", 40, 100, 2, C_YELLOW, 0x0000);
    textAt("RELEASE BUTTON", 40, 130, 2, C_WHITE, 0x0000);
    while (digitalRead(POWER_BTN_PIN) == LOW) { feedWatchdog(); delay(10); }
  }
  loadWifiConfig();

  uint8_t macBytes[6] = {0};
  WiFi.macAddress(macBytes);
  char macHex[13];
  snprintf(macHex, sizeof(macHex), "%02X%02X%02X%02X%02X%02X",
           macBytes[0], macBytes[1], macBytes[2], macBytes[3], macBytes[4], macBytes[5]);
  hostId = String(macHex);

  // เปิดวิทยุก่อน แล้วค่อยเลือกโหมด เพราะ attachEspNowPeer() ต้องใช้
  WiFi.mode(staConfigured ? WIFI_STA : WIFI_AP);
  forceRadioAwake();

  // ---------------- ESP-NOW ----------------
  if (esp_now_init() != ESP_OK) {
    Serial.println("[GW] ESP-NOW init FAILED");
  } else {
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  }

  // เลือกโหมดเครือข่าย (ทั้งสองทางจะผูก peer ของ ESP-NOW ให้ตรงอินเทอร์เฟซเอง)
  if (staConfigured) startStaMode();
  else               startSetupPortal();

  // [1.0.0-TG] เพิ่ม: เธรดส่งสถานะขึ้นพีซี ปักที่คอร์ 0 ข้างงาน Wi-Fi ปล่อยคอร์ 1 ให้ loop()
  xTaskCreatePinnedToCore(uplinkTask, "uplink", UPLINK_TASK_STACK, NULL, 1, NULL, 0);

  drawSplashScreen();
  lastUserActivityTime = millis();
  playWelcomeMelody();
  delay(1400);
  uiMode = -1;

  unsigned long nowMs = millis();
  lastCalcTime = nowMs;
  lastMinuteLogTime = nowMs;
  lastSyncBroadcastTime = nowMs;
  updateHostDisplay();
}

// ==========================================
//  Main Execution Loop
// ==========================================
void loop() {
  feedWatchdog();   // [4.9.0-TFT] เพิ่ม: บอกสุนัขเฝ้าบ้านว่ายังเดินอยู่
  if (portalActive) server.handleClient();   // [1.0.0-TG] แก้: เฉพาะโหมดตั้งค่า
  serviceCommands();                         // [1.0.0-TG] เพิ่ม: ทำคำสั่งที่เธรดส่งรับมาจากพีซี

  checkHostButton();                         // [1.2.0-TG] แก้: ปุ่มเดียวทำทุกอย่าง
  handleBuzzerAlarm();
  handleBatteryChime();                      // [1.1.0-TG] เพิ่ม: เสียงเตือนแบตเตอรี่ต่ำ

  unsigned long currentMillis = millis();

  // สำรองเวลาลง NVS เป็นระยะ เพื่อให้เวลายังใกล้เคียงเดิมหลังไฟดับ
  if (currentMillis - lastTimeBackup >= TIME_BACKUP_INTERVAL_MS) {
    lastTimeBackup = currentMillis;
    backupClockToNvs();
  }

  // ---- ดูแลการเชื่อมต่อเครือข่าย (ไม่บล็อก) ----
  serviceWifi(currentMillis);

  // ---- ประเมินเตือนทุก 1 วินาที (การส่งผลกลับให้ Station แยกไปอยู่ที่ตัวจัดคิวด้านล่าง) ----
  if (currentMillis - lastSyncBroadcastTime >= SYNC_INTERVAL_MS) {
    lastSyncBroadcastTime = currentMillis;
    evaluateClinicalAlerts();
  }

  // ---- ส่งผลคำนวณให้ Station ทีละใบแบบเฉลี่ยช่องเวลา + วัดคุณภาพลิงก์ ----
  serviceSyncScheduler();
  serviceLinkQuality();

  // ---- อัปเดตค่าที่คำนวณได้ทุก 2 วินาที ----
  if (currentMillis - lastCalcTime >= 2000) {
    hostBatteryVolts = readHostBattery();
    hostBatteryPct = calculateBatteryPct(hostBatteryVolts);
    serviceHostBattery(currentMillis);       // [1.1.0-TG] เพิ่ม: ตัดสินระดับแบตเตอรี่ของ Host เอง

    portENTER_CRITICAL(&dataMux);
    for (int i = 0; i < MAX_SUPPORTED_STATIONS; i++) {
      stations[i].totalVolumeMl = (float)stations[i].totalDrops / (float)safeDropFactor(stations[i].cfg.dropFactor);
      if (!stations[i].isRunning || !isStationOnline(i)) {
        stations[i].flowRate_ml_hr = 0.0;
      }
      // [1.0.0-TG] เพิ่ม: สายพับ = อัตราจริงเป็น 0 ล้างตัวคำนวณด้วย (Station รุ่นเดิมทำแบบเดียวกันในเครื่อง)
      if (stations[i].alertCode == ALERT_OCCLUSION || isStationOccluded(i)) {
        stations[i].flowRate_ml_hr = 0.0;
        stations[i].rateValid = false;
        stations[i].est.reset();
      }
    }
    portEXIT_CRITICAL(&dataMux);
    lastCalcTime = currentMillis;
  }

  // เลือกเตียงเองแล้วปล่อยไว้ครบเวลา -> กลับไปโหมดเลือกอัตโนมัติ
  if (manualFocusBed >= 0 && (long)(currentMillis - manualFocusUntil) >= 0) {   // [4.9.0-TFT] แก้: เทียบผลต่างแบบมีเครื่องหมาย จึงทนการวนรอบของ millis()
    manualFocusBed = -1;
    uiMode = -1;
  }

  unsigned long oledRefreshInterval = displaySleeping ? 1000 : 500;
  if (currentMillis - lastOledUpdateTime >= oledRefreshInterval) {
    updateHostDisplay();
    lastOledUpdateTime = currentMillis;
  }

  // ---- ตัวนับนาที ----
  // [1.0.0-TG] ย้าย: ล็อกรายนาทีไปเก็บที่พีซี เหลือตัวนับนาทีไว้เป็นป้ายแทนเวลาตอนนาฬิกายังไม่ถูกตั้ง
  if (currentMillis - lastMinuteLogTime >= 60000) {
    globalMinuteCounter++;
    portENTER_CRITICAL(&dataMux);
    for (int i = 0; i < MAX_SUPPORTED_STATIONS; i++) stations[i].dropsInCurrentMinute = 0;
    portEXIT_CRITICAL(&dataMux);
    lastMinuteLogTime += 60000;
  }
}
