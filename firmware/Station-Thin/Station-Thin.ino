/**
 * @file      Station-Thin.ino
 * @brief     เครื่องประจำเตียงแบบบาง ส่งแค่จำนวนหยด แล้วแสดงผลที่ Host คำนวณส่งกลับมา
 * @version   1.1.0-TH
 * @date      2026-11-20
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 * อาจารย์ที่ปรึกษา ดร.กรรณิการ์ กาศสมบูรณ์
 *
 * @par Hardware
 * ESP32-S3 Super Mini + จอ TFT 1.47" (ST7789 SPI 172x320) + Passive Buzzer
 * เซนเซอร์หยด TCRT5000 แบบสะท้อนแสง ผังขาเหมือน Station ทุกขา
 *
 * @par Description
 * แตกจาก Station 7.10.0 ตามแนวคิดให้เครื่องข้างเตียงเป็นแค่ "ตา" และ "จอ"
 *   ตา  : ตรวจจับหยดด้วยอัลกอริทึมเดิม (drop_detector.h ไม่แก้) ส่งจำนวนหยดสะสมกับเวลาจากหยดล่าสุด
 *   จอ  : แสดงผลที่ Host คำนวณส่งกลับมา ทั้งอัตราไหล ปริมาตร % เวลาที่เหลือ และรหัสเตือน
 * เครื่องนี้ไม่มีสูตรทางคลินิกเลย ผลคำนวณอยู่ที่ Host จุดเดียว (ดู protocol_v4.h)
 *
 * ใช้คู่กับ Host-TFT-Gateway เท่านั้น เพราะใช้ Protocol v4 ซึ่งคนละชนิดกับ Protocol v3 ของ Host รุ่นอื่น
 * แพ็กเก็ตสองโปรโตคอลแยกจากกันด้วยความยาวและรหัสนำหน้า จึงไม่ตีความข้ามรุ่น แต่ก็คุยกันไม่ได้
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 1.1.0-TH | 2026-11-20 | ตรวจระดับแบตเตอรี่ของตัวเอง: ต่ำ/วิกฤตขึ้นไอคอนที่หัวจอ เสียงเตือนเบา ๆ เว้นช่วงนาน ไม่ปนกับเหตุทางคลินิก |
 * | 1.0.0-TH | 2026-11-19 | รุ่นแรก แตกจาก Station 7.10.0 ตัดการคำนวณอัตราไหล ปริมาตร และการตัดสินเตือนออก ใช้ผลจาก Host ผ่าน Protocol v4 |
 * | ↓ | ↓ | ป้ายรุ่นด้านล่างสืบมาจากโค้ดของ Station ที่ยกมาโดยไม่แก้ ใช้ตามรอยเหตุผลได้ใน Station.ino |
 * | 7.10.0 | 2026-09-28 | ไฟสถานะการเชื่อมต่อดูสองทาง ขึ้น ONE-WAY เมื่อได้ยิน Host แต่ Host ไม่ได้ยินเตียง |
 * | 7.9.0 | 2026-09-23 | จำนวนหยดสะสมรอดการรีบูตและไฟดับแล้ว เพิ่มสุนัขเฝ้าบ้าน และแก้การนับเวลาที่พังเมื่อ millis() วนรอบ |
 *
 * @warning  ไฟล์วาดจอถูก `#include` ท้ายไฟล์นี้ก่อน `setup()` ห้ามย้ายขึ้นไปบนสุด
 *           เพราะโค้ดในไฟล์นั้นใช้ตัวแปรและฟังก์ชันช่วยที่ประกาศไว้ด้านบน
 *
 * @warning  `protocol_v4.h` ต้องเหมือนกันทุกไบต์กับของ Host-TFT-Gateway
 *           ตรวจด้วย `bash tools/v4-test/run.sh` ก่อน commit ทุกครั้งที่แตะ
 *
 * @warning  เมื่อ Host หาย (ดับ ลิงก์ขาด สัญญาณอ่อน) จอจะไม่มีอัตราไหล เปอร์เซ็นต์ เวลาที่เหลือ และรหัสเตือน
 *           เหลือจำนวนหยดสะสม ปริมาตรที่ให้แล้ว (หยด/drop factor) และสถานะ NO HOST สีส้ม
 *           นี่คือข้อแลกเปลี่ยนโดยตั้งใจของรุ่นนี้
 *           เพราะเครื่องไม่คำนวณเองแล้ว ถ้าต้องการให้จอทำงานต่อได้ลำพังให้ใช้ Station สายหลัก
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 *
 * @par บันทึกการเปลี่ยนแปลงโดยละเอียด
 * ประวัติก่อน 1.0.0-TH อยู่ใน Station.ino ซึ่งยังคงอยู่ครบ ที่นี่ไม่คัดลอกมาซ้ำ
 *
 *  เปลี่ยนใน V1.0.0-TH: ตัดการคำนวณทั้งหมดออกจากเครื่องข้างเตียง
 *   1) ส่ง Protocol v4 ขึ้น Host: จำนวนหยดสะสม + มิลลิวินาทีจากหยดล่าสุด + ธงสถานะ + แบตเตอรี่ (14 ไบต์)
 *   2) รับ Protocol v4 ลงจาก Host: อัตราไหล ปริมาตรที่ให้แล้ว % นาทีที่เหลือ รหัสเตือน (32 ไบต์) แล้วแสดง
 *   3) ตัดหน้าต่างคำนวณอัตราไหล 8 หยดออก (ย้ายไปอยู่ rate_estimator.h ของ Host ผลต่างจากเดิมไม่เกิน 0.4%)
 *   4) ตัดการตัดสินสายพับในเครื่องและการตัดสินใกล้หมด/ครบแผนในเครื่องออก รหัสเตือนมาจาก Host อย่างเดียว
 *   5) ยังเก็บยอดหยดสะสมลง NVS เหมือนเดิม รีบูตแล้วส่งยอดต่อได้ Host คำนวณต่อจากยอดนั้น
 *   6) ตัวตรวจจับหยด การเรียนรู้รูปคลื่น ปุ่ม จอ เสียง และไฟสถานะ ไม่เปลี่ยน
 *
 *  เปลี่ยนใน V1.1.0-TH: ตรวจระดับแบตเตอรี่ของเครื่องเอง (battery_check.h ใช้ร่วมกับ Host)
 *   - ต่ำ (< 3.50 V) ไอคอนเหลืองที่หัวจอ + เสียงสองครั้งทุก 5 นาที
 *   - วิกฤต (< 3.35 V) ไอคอนแดงกะพริบ + เสียงสามครั้งทุก 1 นาที
 *   - ต้องต่ำต่อเนื่อง 30 วินาทีจึงเตือน (แรงดันตกตอนบัซเซอร์ดังไม่ทำให้เตือนผิด) และมีฮิสเทอรีซิส 50 mV
 *   - ไม่มีวงจรวัด (STATION_BAT_ADC_PIN = -1) หรือใช้ไฟ USB = ไม่ตรวจ ไม่เตือน
 *   - เสียงเตือนแบตเงียบเมื่อมีเหตุทางคลินิกหรือเตือนใกล้หมดอยู่ ไม่แย่งเสียงสำคัญ
 *   - Host ได้แรงดันของเตียงนี้ในแพ็กเก็ตขึ้นอยู่แล้ว และตรวจซ้ำอีกชั้นที่ฝั่ง Host
 */

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <time.h>
#include <sys/time.h>
#include <esp_task_wdt.h>          // [7.9.0] เพิ่ม: สุนัขเฝ้าบ้าน กันเครื่องค้างเงียบ
#include <esp_system.h>            // [7.9.0] เพิ่ม: อ่านสาเหตุการรีบูตครั้งล่าสุด

// ----------------------------------------------------------------------------
// สุนัขเฝ้าบ้านและสาเหตุการรีบูต ([7.9.0] เพิ่มทั้งหมด)
//
// เครื่องนี้ทำหน้าที่เตือนภัย การค้างแบบเงียบจึงอันตรายกว่าการรีบูต เพราะจอยัง
// ค้างภาพเดิมไว้ พยาบาลจึงเข้าใจว่าระบบยังเฝ้าอยู่ ทั้งที่หยุดไปแล้ว
// ตั้งไว้ 8 วินาที ซึ่งยาวกว่ารอบ loop() ปกติหลายเท่า จึงไม่รีบูตเพราะงานหนักชั่วคราว
// ----------------------------------------------------------------------------
#define LOOP_WDT_TIMEOUT_S      8

esp_reset_reason_t bootResetReason = ESP_RST_UNKNOWN;


#include "battery_check.h"      // [1.1.0-TH] เพิ่ม: ตัวตรวจระดับแบตเตอรี่ ใช้ร่วมกับ Host
#include "protocol_v4.h"      // [1.0.0-TH] เพิ่ม: Protocol v4 ใช้ร่วมกับ Host-TFT-Gateway
#include "drop_detector.h"   // อัลกอริทึมตรวจจับหยด (ทดสอบบน PC ได้: tools/detector-test)

#define APP_VERSION         "1.1.0-TH"

#define TFT_CS              9
#define TFT_DC              10
#define TFT_RST             11
#define TFT_MOSI            12
#define TFT_SCLK            13
#define TFT_BLK             8

// ---- ทิศการแสดงผลของจอ ----
// จอ ST7789 172x320 หมุนได้ 2 ทิศที่ยังเป็นแนวตั้งขนาดเดิม จึงใช้พิกัดชุดเดียวกัน
#define TFT_ROT_NORMAL        0   // สาย USB อยู่ด้านล่าง (ค่ามาตรฐาน)
#define TFT_ROT_FLIP180       2   // กลับหัว 180 องศา สาย USB อยู่ด้านบน
#define TFT_ROTATION_DEFAULT  TFT_ROT_NORMAL   // ค่าเริ่มต้นเมื่อยังไม่เคยตั้งที่หน้าเครื่อง

#define SENSOR_AO_PIN       4
#define BUZZER_PIN          3
#define BTN_PIN             2
#define RGB_LED_PIN         48

// วงจรวัดแบตเตอรี่ (ถ้ามี): ใส่หมายเลขขา ADC ที่ต่อผ่านตัวต้านทานแบ่งแรงดัน 1:1 เช่น 1
// -1 = ไม่มีวงจร -> ส่งค่า 0 และหน้าเว็บ/Host แสดง "N/A"
#define STATION_BAT_ADC_PIN -1
#define BAT_DIVIDER_RATIO   2.0f

// ----------------------------------------------------------------------------
// ค่าคงที่ของระบบสื่อสารและการแจ้งเตือน (ต้องตรงกับ Host)
// ----------------------------------------------------------------------------
#define DEFAULT_ESPNOW_CHANNEL  1
#define MAX_WIFI_CHANNEL        13
#define HOST_TIMEOUT_MS         5000
#define CHANNEL_SCAN_DWELL_MS   2500     // Host ส่ง Sync ทุก 1 วินาที -> รอแต่ละช่อง 2.5 วินาที
#define CHANNEL_SCAN_START_MS   6000     // หลังบูตรอช่องที่จำไว้ก่อน 6 วินาที
#define SEND_INTERVAL_MS        1000     // คาบพื้นฐานของการส่งข้อมูลให้ Host
#define SEND_JITTER_MS          60       // สุ่มบวกลบรอบคาบ กันสองสเตชันส่งชนกันค้างนาน
                                         // (ดูคำอธิบายเหตุผลในหัวไฟล์ V7.7.2)
#define MIN_OCCLUSION_MS        8000
#define MAX_OCCLUSION_MS        300000
#define SCREENSAVER_IDLE_MS     180000
#define SNOOZE_MS               120000
#define NEAR_END_REMIND_MS      300000   // เตือนใกล้หมดซ้ำทุก 5 นาทีจนกว่าจะรับทราบ
#define DEFAULT_NEAR_END_PCT    80

#define ALERT_NONE        0
#define ALERT_TOO_FAST    1
#define ALERT_TOO_SLOW    2
#define ALERT_NEAR_END    3   // ให้ไปแล้วถึง % ที่ตั้ง (เตรียมถุงใหม่)
#define ALERT_COMPLETE    4
#define ALERT_OCCLUSION   5

#define THEME_BG            0x0000
#define THEME_SKYBLUE       0x5DFF
#define THEME_CYAN          0x07FF
#define THEME_PINK          0xFD79
#define THEME_NAVY          0x09CD
#define THEME_DARKBOX       0x18C5
#define THEME_MAROON        0x6000

#define COLOR_WHITE         0xFFFF
#define COLOR_GREEN         0x07E0
#define COLOR_RED           0xF800
#define COLOR_ORANGE        0xFD20
#define COLOR_YELLOW        0xFFE0

// ชุดสีของหน้าจอรุ่นใหม่ (พื้นดำ การ์ดเทาเข้ม ตัวอักษรรองสีเทา)
#define UI_CARD             0x18C5   // พื้นการ์ด
#define UI_BAR              0x0A49   // แถบบนสุด
#define UI_LINE             0x39E7   // เส้นคั่น/แถบว่าง
#define UI_DIM              0x8410   // ข้อความรอง
#define UI_GREEN            0x2FEB   // เขียวอ่านง่ายบนพื้นดำ

#ifndef ST77XX_LIGHTGREY
  #define ST77XX_LIGHTGREY  0xC618
#endif
#ifndef ST77XX_DARKGREY
  #define ST77XX_DARKGREY   0x39E7
#endif

SPIClass SPI_TFT(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI_TFT, TFT_CS, TFT_DC, TFT_RST);
Preferences stationPrefs;

enum AppState {
  STATE_NORMAL_VIEW,
  STATE_EMERGENCY,
  STATE_SCREENSAVER,
  STATE_CREDIT,
  STATE_CONFIG_ID,
  STATE_NEAR_END_NOTICE
};

// ---- สถานะที่ใช้เลือกสีและคำบนจอ ----
// ต้องประกาศไว้ตอนต้นไฟล์ เพราะ Arduino IDE แทรก prototype ของฟังก์ชันไว้ก่อนส่วนแสดงผล
enum UiStatus { UI_OK = 0, UI_FAST, UI_SLOW, UI_NOFLOW, UI_NEAREND, UI_DONE, UI_PAUSED, UI_NOHOST };   // [1.0.0-TH] เพิ่ม: UI_NOHOST

AppState currentState       = STATE_NORMAL_VIEW;
int currentNursePage        = 1;
uint8_t currentStationId    = 1;
uint8_t tempConfigStationId = 1;
uint8_t screenRotation      = TFT_ROTATION_DEFAULT;   // 0 = ปกติ, 2 = กลับหัว 180 องศา
unsigned long configAutoSaveTimeout = 0;
unsigned long lastUserActivityTime  = 0;

bool isRunning              = true;
bool isClockSynced          = false;
volatile bool isHostOnline  = false;

// ---- สถานะการเชื่อมต่อแบบสองทาง ([7.10.0] เพิ่ม) ----
// isHostOnline บอกได้แค่ว่า "เราได้ยิน Host" เพราะ Host กระจาย Sync ให้ทุกเลขเตียง
// ที่เปิดไว้ ไม่ว่าจะได้ยินเตียงนั้นหรือไม่ เดิมจอจึงขึ้น ONLINE ได้ทั้งที่ Host ขึ้น OFF
// ให้เตียงนี้อยู่ พยาบาลเข้าใจว่าระบบเฝ้าอยู่ทั้งที่ไม่ได้เฝ้า — ความล้มเหลวแบบเงียบ
#define LINK_NONE             0        // ไม่ได้ยิน Host เลย
#define LINK_OK               1        // สองทางปกติ (หรือ Host รุ่นเก่าที่บอกไม่ได้)
#define LINK_NOT_HEARD        2        // ได้ยิน Host แต่ Host บอกว่าไม่ได้ยินเรา
#define LINK_NOT_LISTED       3        // ได้ยิน Host แต่ไม่มี Sync ของเตียงเราเลย
                                       // = เลขเตียงเกินจำนวนเตียงที่ Host เปิดไว้
volatile unsigned long hostOnlineSince     = 0;
volatile unsigned long lastOwnSyncTime     = 0;
volatile unsigned long lastHeardReportTime = 0;
volatile int lastHostRssi   = -100;
volatile unsigned long lastHostRecvTime = 0;
bool stateNeedsRedraw       = true;

// ----------------------------------------------------------------------------
// โครงสร้างข้อมูลรับ-ส่ง ESP-NOW (Protocol v2) — ต้องเหมือนกับ Host ทุกไบต์
// ----------------------------------------------------------------------------
struct_up_v4 myData;                 // [1.0.0-TH] แก้: แพ็กเก็ตขึ้น Protocol v4
uint8_t upSeq = 0;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint32_t totalDrops                 = 0;
uint32_t periodDropsCounter         = 0;

float currentFlowRate_ml_hr         = 0.0;
float currentGttMin                 = 0.0;
float totalVolumeMl                 = 0.0;

// ค่าจาก Host (0 = Host ยังไม่ได้ตั้งค่า)
volatile float targetRateHr         = 0.0;
volatile float totalPlanMl          = 0.0;
volatile uint8_t hostAlertCode      = ALERT_NONE;
volatile uint8_t dropFactor         = 20;
volatile uint8_t hostReportedChannel = 0;
// [1.0.0-TH] เพิ่ม: ผลคำนวณที่ Host ส่งมา (เครื่องนี้ไม่คำนวณเอง)
volatile uint16_t hostMinutesLeft   = V4_MINUTES_UNKNOWN;
volatile uint8_t  hostInfusedPct    = 0;
volatile bool     hostPlanSet       = false;
volatile bool     hostRateOk        = false;
volatile bool pendingCounterReset   = false;
volatile uint8_t nearEndPct         = DEFAULT_NEAR_END_PCT;
volatile bool hostNearEndAck        = false;   // Host ยืนยันว่ารับทราบแล้ว
bool nearEndAckRequest              = false;   // กดรับทราบที่เตียง รอ Host ยืนยัน
unsigned long nearNextChimeTime     = 0;
uint8_t nearChimeRemaining          = 0;
int16_t knownResetSeq               = -1;

uint8_t espnowChannel               = DEFAULT_ESPNOW_CHANNEL;
uint8_t savedChannel                = DEFAULT_ESPNOW_CHANNEL;
unsigned long lastChannelHopTime    = 0;

unsigned long lastDropTimestamp     = 0;
bool hasFirstDropOccurred           = false;

unsigned long lastSendTime          = 0;
unsigned long nextSendDelay         = SEND_INTERVAL_MS;   // คาบของรอบถัดไป (สุ่มใหม่ทุกครั้ง)
unsigned long lastDebounceTime      = 0;
unsigned long snoozeUntilTime       = 0;
const unsigned long minDropInterval = 55;

unsigned long buzzerBeepUntil       = 0;

// ---- ตรวจระดับแบตเตอรี่ของเครื่องเอง ([1.1.0-TH] เพิ่ม) ----
#define BAT_CHIME_LOW_MS        300000UL   // ต่ำ: เตือนทุก 5 นาที
#define BAT_CHIME_CRIT_MS       60000UL    // วิกฤต: เตือนทุก 1 นาที
BatteryMonitor selfBat;
unsigned long  lastBatCheck       = 0;
uint8_t        batChimeRemaining  = 0;
unsigned long  batChimeNextBeep   = 0;
unsigned long  batChimeNextRound  = 0;
uint8_t        batChimeLevelSeen  = BAT_NONE;   // ระดับที่เสียงเตือนรอบล่าสุดอิงอยู่ ใช้ตรวจว่าแย่ลงหรือยัง
unsigned long buzzerNextBeepTime    = 0;

bool isDropFalling                  = false;
unsigned long dropFallStartTime     = 0;
int animDropY                       = 190;
int animDropPrevY                   = 190;
int8_t animModeDrawn                = -1;
unsigned long lastAnimFrame         = 0;
unsigned long poolFlashUntil        = 0;
const int CHAMBER_NOZZLE_Y          = 190;   // ตำแหน่งปลายหยดในกระเปาะ (หน้าจอใหม่)
const int CHAMBER_POOL_Y            = 204;   // ผิวน้ำในกระเปาะ
const unsigned long DROP_FALL_DURATION = 160;

bool isHoldUiActive                 = false;
bool holdBeep3sDone                 = false;
bool holdBeep5sDone                 = false;
unsigned long lastHoldRenderTime    = 0;

// ---- รหัสเตือนที่หน้าจอใช้ (อัปเดตทุกรอบใน loop) ----
uint8_t uiAlertCode = ALERT_NONE;

// ---- แคชของหน้าจอ: วาดใหม่เฉพาะเมื่อค่าที่แสดงเปลี่ยนจริง ----
String cacheClock = "";
int    cacheBars = -9999;
int    cacheBatKey = -9999;     // [1.1.0-TH] เพิ่ม: ไอคอนแบตเตอรี่ที่หัวจอ
int    cacheStatusBar = -1;
bool   cacheBedChip = false;
int    cacheRate = -9999, cacheTarget = -9999, cacheLeftMl = -9999;
int    cachePct = -999, cacheBagPct = -999;
String cacheTimeLeft = "", cacheEndClock = "", cacheStatusWord = "";
String cacheP2a = "", cacheP2b = "", cacheP2c = "", cacheP2d = "", cacheP2foot = "";
String cacheP3a = "", cacheP3b = "", cacheP3c = "";
String cacheP4 = "", cacheNear = "", cacheConfig = "";
uint8_t lastEmergencyCode = 255;

// ---- กราฟแนวโน้มอัตราไหล: 30 แท่ง แท่งละ 1 นาที = ย้อนหลัง 30 นาที ----
#define TREND_BARS   30
#define TREND_BIN_MS 60000UL
float    trendBin[TREND_BARS];
bool     trendHas[TREND_BARS];
int      trendHead      = 0;
float    trendSum       = 0.0f;
int      trendSamples   = 0;
float    trendLive      = 0.0f;
float    trendScale     = 100.0f;
bool     trendNeedsRedraw = true;
unsigned long trendBinStart   = 0;
unsigned long trendLastSample = 0;

// ---- ประกาศล่วงหน้า (ฟังก์ชันที่ถูกเรียกก่อนบรรทัดที่นิยามไว้) ----
// ฟังก์ชันวาดจอที่ย้ายไป StationScreen.h แล้ว แต่ถูกเรียกจากโค้ดเหนือบรรทัด include
void drawTopBar(bool force);
void drawProgressBar(int x, int y, int w, int h, int pct, uint16_t fill);
void drawStatCard(int y, const char* label, const String &value, uint16_t valueColor, bool labelOnly);
void drawTrendChart();
void drawTrendLiveBar();
void applyScreenRotation(uint8_t rot);
void beepNonBlocking(uint16_t freq, uint16_t durationMs);
void textAt(const String &s, int x, int y, uint8_t size, uint16_t col, uint16_t bg);
String padTo(const String &s, unsigned int width);
void textAt(const String &s, int x, int y, uint8_t size, uint16_t col, uint16_t bg);
String padTo(const String &s, unsigned int width);
void textCenterIn(const String &s, int x, int w, int y, uint8_t size, uint16_t col, uint16_t bg);
void resetTrendData();
void updateTrendAccumulator(unsigned long now);

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

// ----------------------------------------------------------------------------
// เก็บจำนวนหยดสะสมให้รอดการรีบูต ([7.9.0] เพิ่มทั้งหมด)
//
// เดิมตัวนับอยู่ใน RAM อย่างเดียว ไฟตกครึ่งวินาทีเดียวก็กลับเป็นศูนย์ ตัวเลข
// "ให้ไปแล้วกี่ mL" จึงผิดไปทั้งถุง และการเตือนใกล้หมด/ให้ครบตามแผนจะมาช้ากว่า
// ความจริงเท่ากับปริมาตรที่หายไป ซึ่งมีผลกับผู้ป่วยโดยตรง
//
// เก็บสองชั้นเพราะข้อจำกัดคนละแบบ
//   RTC — เขียนได้ไม่จำกัด จึงเขียนทุกรอบ รอดรีเซ็ตซอฟต์แวร์ วอตช์ด็อก ไฟตกชั่วขณะ
//   NVS — รอดไฟดับสนิท แต่แฟลชมีอายุการเขียนจำกัด จึงเขียนห่าง ๆ ทุก 5 นาที
// ตอนบูตใช้ค่าที่มากกว่าของสองชั้น และต้องเป็นของเตียงเดียวกันเท่านั้น
// ----------------------------------------------------------------------------
#define TALLY_NVS_SAVE_MS   300000UL
#define TALLY_RTC_MAGIC     0x49565431UL      // 'IVT1' ลายเซ็นบอกว่าค่าใน RTC ใช้ได้

RTC_DATA_ATTR uint32_t rtcTallyMagic = 0;
RTC_DATA_ATTR uint32_t rtcTallyDrops = 0;
RTC_DATA_ATTR uint32_t rtcTallyBed   = 0;

uint32_t      tallySavedToNvs   = 0;
unsigned long tallyNextNvsSave  = 0;
bool          tallyRestoredBoot = false;      // บูตนี้กู้ยอดเก่ากลับมา (ไว้บอกพยาบาล)

void tallyRemember(uint32_t drops, uint8_t bed) {
  rtcTallyMagic = TALLY_RTC_MAGIC;
  rtcTallyDrops = drops;
  rtcTallyBed   = bed;
}

void tallyWriteNvs(uint32_t drops, uint8_t bed) {
  stationPrefs.begin("st_tally", false);
  stationPrefs.putUInt("drops", drops);
  stationPrefs.putUChar("bed", bed);
  stationPrefs.end();
  tallySavedToNvs = drops;
}

// เริ่มถุงใหม่ = ล้างทั้งสองชั้นทันที ไม่งั้นบูตครั้งหน้าจะกู้ยอดของถุงเก่ากลับมา
void tallyClear(uint8_t bed) {
  tallyRemember(0, bed);
  tallyWriteNvs(0, bed);
  tallyRestoredBoot = false;
}

uint32_t tallyRestore(uint8_t bed) {
  uint32_t best = 0;
  if (rtcTallyMagic == TALLY_RTC_MAGIC && rtcTallyBed == bed) best = rtcTallyDrops;
  stationPrefs.begin("st_tally", true);
  uint32_t nv    = stationPrefs.getUInt("drops", 0);
  uint8_t  nvBed = stationPrefs.getUChar("bed", 0);
  stationPrefs.end();
  if (nvBed == bed && nv > best) best = nv;      // RTC หายเพราะไฟดับสนิท จึงถอยมาใช้ NVS
  tallySavedToNvs   = best;
  tallyRestoredBoot = (best > 0);
  tallyNextNvsSave  = millis() + TALLY_NVS_SAVE_MS;
  return best;
}

void serviceTallySave(unsigned long now, uint32_t drops, uint8_t bed) {
  tallyRemember(drops, bed);                     // ชั้นเร็ว ไม่กินอายุแฟลช
  if ((long)(now - tallyNextNvsSave) < 0) return;
  tallyNextNvsSave = now + TALLY_NVS_SAVE_MS;
  if (drops != tallySavedToNvs) tallyWriteNvs(drops, bed);
}

uint8_t safeDropFactor(uint8_t df) {
  return (df == 10 || df == 15 || df == 20 || df == 60) ? df : 20;
}

bool isCriticalAlert(uint8_t code) {
  return code == ALERT_TOO_FAST || code == ALERT_TOO_SLOW ||
         code == ALERT_COMPLETE || code == ALERT_OCCLUSION;
}

uint8_t safeNearPct(uint8_t p) {
  return (p >= 50 && p <= 95) ? p : DEFAULT_NEAR_END_PCT;
}

// [1.0.0-TH] แก้: ใกล้หมดตัดสินที่ Host อย่างเดียว (เดิมคำนวณจากปริมาตรในเครื่อง)
bool isNearEndReached() {
  return hostAlertCode == ALERT_NEAR_END;
}

bool isNearEndAcknowledged() {
  return hostNearEndAck || nearEndAckRequest;
}

// [7.10.0] เพิ่ม: สถานะการเชื่อมต่อที่รวมขาขึ้นด้วย ต้องผิดปกติต่อเนื่องเกิน HOST_TIMEOUT_MS
// จึงจะขึ้นเตือน กันจอกะพริบตอนเพิ่งบูตที่ Host ยังไม่ทันได้ยินเรา
uint8_t linkState() {
  if (!isHostOnline) return LINK_NONE;
  unsigned long now = millis();
  unsigned long ref = hostOnlineSince;
  if (lastOwnSyncTime > ref) ref = lastOwnSyncTime;
  if (now - ref > HOST_TIMEOUT_MS) return LINK_NOT_LISTED;
  {   // [1.0.0-TH] แก้: Protocol v4 รายงานขาขึ้นเสมอ ไม่มี Host รุ่นเก่าที่ไม่รายงานอีกแล้ว
    unsigned long heard = hostOnlineSince;
    if (lastHeardReportTime > heard) heard = lastHeardReportTime;
    if (now - heard > HOST_TIMEOUT_MS) return LINK_NOT_HEARD;
  }
  return LINK_OK;
}

// รหัสเตือนที่ใช้จริงในเครื่องนี้
// [1.0.0-TH] แก้: มาจาก Host อย่างเดียว ไม่ตัดสินสายพับ ครบแผน หรือใกล้หมดเองอีกแล้ว
// Host หายก็ไม่มีรหัสเตือน (จอขึ้นสถานะขาดการติดต่อแทน ผ่าน linkState())
uint8_t effectiveAlertCode() {
  if (!isRunning) return ALERT_NONE;
  if (!isHostOnline) return ALERT_NONE;
  return hostAlertCode;
}

bool isNearEndPending(uint8_t code) {
  return code == ALERT_NEAR_END && !isNearEndAcknowledged();
}

void acknowledgeNearEnd() {
  nearEndAckRequest = true;
  nearChimeRemaining = 0;
  noTone(BUZZER_PIN);
  beepNonBlocking(2600, 40);
}

float readStationBattery() {
#if STATION_BAT_ADC_PIN >= 0
  long sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(STATION_BAT_ADC_PIN);
  float v = ((sum / 8.0f) / 4095.0f) * 3.3f * BAT_DIVIDER_RATIO;
  return (v < 2.5f) ? 0.0f : v;
#else
  return 0.0f;
#endif
}

void drawCenteredString(const String &str, int y, uint8_t size, uint16_t color, uint16_t bg) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.setTextSize(size);
  tft.getTextBounds(str, 0, y, &x1, &y1, &w, &h);
  int x = (172 - (int)w) / 2;
  if (x < 0) x = 0;
  tft.setTextColor(color, bg);
  tft.setCursor(x, y);
  tft.print(str);
}

void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
  neopixelWrite(RGB_LED_PIN, r, g, b);
}

void beepNonBlocking(uint16_t freq, uint16_t durationMs) {
  tone(BUZZER_PIN, freq);
  buzzerBeepUntil = millis() + durationMs;
}

void modalSafeBeep(uint16_t freq, uint16_t durationMs) {
  tone(BUZZER_PIN, freq);
  delay(durationMs);
  noTone(BUZZER_PIN);
  buzzerBeepUntil = 0;
}

void soundClick()       { beepNonBlocking(2600, 20); }
void soundStart()       { beepNonBlocking(2000, 70); }
void soundStop()        { beepNonBlocking(1000, 140); }
void soundScreensaver() { beepNonBlocking(1800, 40); }

void playWelcomeMelody() {
  int notes[] = { 1046, 1318, 1568, 2093 };
  for (int i = 0; i < 4; i++) {
    modalSafeBeep(notes[i], 80);
    delay(25);
  }
}

void handlePassiveBuzzerEngine(uint8_t code) {
  unsigned long now = millis();
  if (buzzerBeepUntil > 0 && now >= buzzerBeepUntil) {
    noTone(BUZZER_PIN);
    buzzerBeepUntil = 0;
  }

  // เสียงเตือนใกล้หมด: 3 ครั้งสั้น ๆ แล้วเงียบ 5 นาที (ดังเฉพาะเมื่อยังไม่มีเหตุวิกฤต)
  if (!isCriticalAlert(code)) {
    if (!isNearEndPending(code)) { nearChimeRemaining = 0; nearNextChimeTime = 0; return; }
    if (nearChimeRemaining == 0 && (nearNextChimeTime == 0 || now >= nearNextChimeTime)) {
      nearChimeRemaining = 3;
      buzzerNextBeepTime = now;
      nearNextChimeTime = now + NEAR_END_REMIND_MS;
    }
    if (nearChimeRemaining > 0 && now >= buzzerNextBeepTime) {
      beepNonBlocking(2000, 70);
      buzzerNextBeepTime = now + 220;
      nearChimeRemaining--;
    }
    return;
  }

  if (now < snoozeUntilTime) return;
  if (now < buzzerNextBeepTime) return;

  switch (code) {
    case ALERT_OCCLUSION: beepNonBlocking(1800, 150); buzzerNextBeepTime = now + 900;  break;
    case ALERT_TOO_FAST:  beepNonBlocking(2200, 80);  buzzerNextBeepTime = now + 700;  break;
    case ALERT_TOO_SLOW:  beepNonBlocking(1200, 120); buzzerNextBeepTime = now + 1200; break;
    case ALERT_COMPLETE:  beepNonBlocking(1500, 250); buzzerNextBeepTime = now + 1500; break;
  }
}

// [1.1.0-TH] เพิ่ม: อ่านแรงดันของเครื่องเองแล้วตัดสินระดับ เรียกวินาทีละครั้ง
// volts = 0 คือไม่มีวงจรวัด/ใช้ไฟ USB (readStationBattery คืน 0) ระดับเป็น BAT_NONE ไม่เตือน
void serviceBattery(unsigned long now, float volts) {
  uint16_t mv = (volts > 0.0f) ? (uint16_t)(volts * 1000.0f + 0.5f) : 0;
  selfBat.update((uint32_t)now, mv);
}

// [1.1.0-TH] เพิ่ม: เสียงเตือนแบตเตอรี่เบา ๆ เว้นช่วงนาน เป็นเรื่องบำรุงรักษา ไม่ใช่เหตุทางคลินิก
// จึงเงียบเมื่อมีเหตุวิกฤต เตือนใกล้หมดกำลังดัง หรือเสียงอื่นกำลังเล่นอยู่ ไม่แย่งเสียงที่สำคัญกว่า
void serviceBatteryChime(unsigned long now, uint8_t alertCode) {
  if (!selfBat.isLow()) { batChimeRemaining = 0; batChimeNextRound = 0; batChimeLevelSeen = BAT_NONE; return; }
  // ระดับเปลี่ยน (โดยเฉพาะต่ำ -> วิกฤต) ต้องเตือนทันที ไม่รอรอบเดิมที่นับตามช่วงของระดับก่อนหน้าซึ่งนานสุด 5 นาที
  if (selfBat.level != batChimeLevelSeen) { batChimeLevelSeen = selfBat.level; batChimeNextRound = 0; }
  if (isCriticalAlert(alertCode) || isNearEndPending(alertCode) || nearChimeRemaining > 0) {
    batChimeRemaining = 0;
    return;
  }
  if (batChimeRemaining == 0 && (batChimeNextRound == 0 || now >= batChimeNextRound)) {
    batChimeRemaining = (selfBat.level == BAT_CRIT) ? 3 : 2;
    batChimeNextBeep  = now;
    batChimeNextRound = now + ((selfBat.level == BAT_CRIT) ? BAT_CHIME_CRIT_MS : BAT_CHIME_LOW_MS);
  }
  if (batChimeRemaining > 0 && now >= batChimeNextBeep && buzzerBeepUntil == 0) {
    beepNonBlocking(1500, 70);
    batChimeNextBeep = now + 250;
    batChimeRemaining--;
  }
}

void updateStatusLed(uint8_t code) {
  static int lastLed = -1;
  int led;
  if (currentState == STATE_CONFIG_ID) led = 4;
  else if (isCriticalAlert(code) && millis() >= snoozeUntilTime) led = 0;
  else if (isNearEndPending(code)) led = 5;
  else if (!isRunning) led = 1;
  else if (linkState() != LINK_OK) led = 2;   // [7.10.0] แก้: ขาขึ้นขาดก็ต้องเตือน
  else led = 3;

  if (led == lastLed) return;
  lastLed = led;
  switch (led) {
    case 0: setLedColor(90, 0, 0);  break;
    case 1: setLedColor(60, 40, 0); break;
    case 2: setLedColor(0, 0, 60);  break;
    case 3: setLedColor(0, 30, 0);  break;
    case 4: setLedColor(50, 0, 50); break;
    case 5: setLedColor(90, 35, 0); break;
  }
}

void drawHoldProgressHUD(unsigned long holdDur) {
  int boxX = 6, boxY = 100, boxW = 160, boxH = 100;
  tft.fillRoundRect(boxX, boxY, boxW, boxH, 8, THEME_NAVY);
  tft.drawRoundRect(boxX, boxY, boxW, boxH, 8, THEME_CYAN);

  int barX = boxX + 15;
  int barY = boxY + 48;
  int barW = 130;
  int barH = 12;

  if (holdDur < 3000) {
    int fill = map(holdDur, 400, 3000, 0, barW);
    if (fill < 0) fill = 0;
    if (fill > barW) fill = barW;

    drawCenteredString("BUTTON HOLDING", boxY + 12, 1, THEME_SKYBLUE, THEME_NAVY);
    drawCenteredString("-> SENSOR CHECK (3s)", boxY + 28, 1, COLOR_YELLOW, THEME_NAVY);
    tft.drawRect(barX, barY, barW, barH, COLOR_WHITE);
    tft.fillRect(barX + 1, barY + 1, fill, barH - 2, THEME_PINK);
    drawCenteredString("Release <3s = Cancel", boxY + 82, 1, ST77XX_LIGHTGREY, THEME_NAVY);
  }
  else if (holdDur < 5000) {
    int fill = map(holdDur, 3000, 5000, 0, barW);
    if (fill < 0) fill = 0;
    if (fill > barW) fill = barW;

    drawCenteredString("RELEASE = CHECK", boxY + 12, 1, COLOR_GREEN, THEME_NAVY);
    drawCenteredString("-> CREDITS (5s)", boxY + 28, 1, THEME_PINK, THEME_NAVY);
    tft.drawRect(barX, barY, barW, barH, COLOR_WHITE);
    tft.fillRect(barX + 1, barY + 1, fill, barH - 2, THEME_CYAN);
  }
  else {
    drawCenteredString(">> CREDITS READY <<", boxY + 14, 1, COLOR_GREEN, THEME_NAVY);
    drawCenteredString("RELEASE TO VIEW", boxY + 30, 1, COLOR_YELLOW, THEME_NAVY);
    tft.drawRect(barX, barY, barW, barH, COLOR_WHITE);
    tft.fillRect(barX + 1, barY + 1, barW - 2, barH - 2, COLOR_GREEN);
  }
}

// ============================================================================
// เชื่อมตัวตรวจจับหยด (drop_detector.h) เข้ากับส่วนอื่นของเฟิร์มแวร์
// ตัวอัลกอริทึมทั้งหมดอยู่ในไฟล์นั้น ไฟล์นี้ทำแค่ป้อนค่า ADC เข้าไปและรับผลออกมา
// ============================================================================
iv::DropDetector detector;

int  sensorRaw      = 0;     // ค่าดิบล่าสุด (ไว้แสดงผล)
bool sensorHasDrops = false; // เคยจับหยดได้แล้วอย่างน้อย 1 หยด


// [1.0.0-TH] ย้าย: หน้าต่างคำนวณอัตราไหล 8 หยดไปอยู่ rate_estimator.h ของ Host
// เครื่องนี้เหลือแค่นับหยดกับจับเวลาหยดล่าสุด

// นับหยด 1 หยด: จำนวนสะสม ภาพหยดตก และเวลาของหยดล่าสุด (ส่งขึ้น Host เป็น msSinceLastDrop)
void onDropDetected() {
  unsigned long now = millis();
  totalDrops++;
  periodDropsCounter++;
  sensorHasDrops = true;

  isDropFalling = true;
  dropFallStartTime = now;
  animDropY = CHAMBER_NOZZLE_Y;

  hasFirstDropOccurred = true;
  lastDropTimestamp = now;
}

// เรียกให้ถี่ที่สุดเท่าที่ทำได้ — countDrops = false ตอนหยุดชั่วคราวหรืออยู่หน้าตรวจเซนเซอร์
bool serviceDropSensor(bool countDrops) {
  sensorRaw = analogRead(SENSOR_AO_PIN);
  bool got = detector.update(sensorRaw, micros());
  if (got && countDrops) onDropDetected();
  return got;
}

const iv::DetectorStatus &det() { return detector.status(); }

// ข้อความสถานะที่พยาบาลอ่านแล้วรู้ว่าต้องทำอะไรต่อ — มีแค่ 4 แบบเท่านั้น
const char* sensorStateWord() {
  switch (det().lock) {
    case iv::Lock::Ready:    return "READY";
    case iv::Lock::Learning: return "LEARNING";
    case iv::Lock::NoDrops:  return "CHECK SENSOR";
    default:                 return "STARTING";
  }
}

uint16_t sensorStateColor() {
  switch (det().lock) {
    case iv::Lock::Ready:    return UI_GREEN;
    case iv::Lock::Learning: return COLOR_YELLOW;
    case iv::Lock::NoDrops:  return COLOR_RED;
    default:                 return THEME_SKYBLUE;
  }
}

// บันทึก/อ่านสิ่งที่เครื่องเรียนรู้ไว้ เพื่อให้เปิดเครื่องครั้งถัดไปพร้อมใช้เร็วขึ้น
void saveLearning() {
  stationPrefs.begin("st_cal", false);
  stationPrefs.putInt("lamp", det().learnedAmp);
  stationPrefs.putInt("lwid", det().learnedWidthMs);
  stationPrefs.putInt("pol",  detector.polarity());
  stationPrefs.end();
}

void loadLearning() {
  stationPrefs.begin("st_cal", true);
  int amp = stationPrefs.getInt("lamp", 0);
  int wid = stationPrefs.getInt("lwid", 0);
  int pol = stationPrefs.getInt("pol", 0);
  stationPrefs.end();
  if (pol == 1 || pol == -1) detector.setPolarity((int8_t)pol);
  else                       detector.setPolarityAuto();
  detector.seedLearning(amp, wid);
}

// ============================================================================
// หน้าตรวจเซนเซอร์ (กดปุ่มค้าง 3 วินาที) — ใช้ตอนติดตั้งหรือเวลาสงสัยว่าผิดปกติ
// ไม่มีขั้นตอนให้ทำตาม ไม่ต้องหยุดน้ำ ไม่ต้องกดยืนยันอะไร ดูอย่างเดียวแล้วกดออก
// ============================================================================
#define SCOPE_X            7
#define SCOPE_W            158
#define SCOPE_H            72
#define SCOPE_MS_PER_COL   16
#define SCOPE_TOP          64

int16_t scopeCol[SCOPE_W];
uint8_t scopeHead = 0;
int scopeColMax = 0;
int scopeFull = 200;
unsigned long scopeColStart = 0;

void scopeReset() {
  for (int i = 0; i < SCOPE_W; i++) scopeCol[i] = 0;
  scopeHead = 0; scopeColMax = 0; scopeColStart = millis();
}

void scopePush(int v) {
  if (v < 0) v = 0;
  if (v > scopeColMax) scopeColMax = v;
  unsigned long now = millis();
  if (now - scopeColStart < SCOPE_MS_PER_COL) return;
  scopeColStart = now;
  scopeCol[scopeHead] = (int16_t)scopeColMax;
  scopeHead = (uint8_t)((scopeHead + 1) % SCOPE_W);
  scopeColMax = v;
}

int scopeYof(int y, int value) {
  if (scopeFull < 20) scopeFull = 20;
  long h = (long)value * (SCOPE_H - 1) / scopeFull;
  if (h < 0) h = 0;
  if (h > SCOPE_H - 1) h = SCOPE_H - 1;
  return y + SCOPE_H - 1 - (int)h;
}

void scopeDraw(int y) {
  int thr = det().threshold;
  int peak = thr * 2;
  for (int i = 0; i < SCOPE_W; i++) if (scopeCol[i] > peak) peak = scopeCol[i];
  scopeFull = (peak * 12) / 10;
  if (scopeFull < 40) scopeFull = 40;
  int yThr = scopeYof(y, thr);

  for (int i = 0; i < SCOPE_W; i++) {
    // การวาดทั้งแถบใช้เวลาหลายสิบมิลลิวินาทีบนจอจริง ต้องแทรกการอ่านเซนเซอร์ไว้ด้วย
    // ไม่งั้นการวาดจอจะทำให้การอ่านขาดช่วงจนตรวจจับหยดไม่ได้
    if ((i % 12) == 0) serviceDropSensor(false);
    int x = SCOPE_X + i;
    int v = scopeCol[(scopeHead + i) % SCOPE_W];
    int yTop = scopeYof(y, v);
    if (yTop > y) tft.drawFastVLine(x, y, yTop - y, THEME_BG);
    if (yTop < y + SCOPE_H)
      tft.drawFastVLine(x, yTop, y + SCOPE_H - yTop,
                        (v >= thr) ? COLOR_YELLOW : THEME_SKYBLUE);
    if ((i % 4) < 2 && thr < scopeFull && yThr < yTop) tft.drawPixel(x, yThr, COLOR_ORANGE);
  }
}

void checkRow(int y, const String &label, const String &value, uint16_t col) {
  tft.fillRect(10, y, 152, 14, THEME_NAVY);
  textAt(label, 12, y, 1, UI_DIM, THEME_NAVY);
  textAt(value, 84, y, 1, col, THEME_NAVY);
}

void executeButtonCalibrationWizard() {
  noTone(BUZZER_PIN);
  buzzerBeepUntil = 0;

  tft.fillScreen(THEME_BG);
  tft.fillRect(0, 0, 172, 28, THEME_PINK);
  drawCenteredString("SENSOR CHECK", 8, 1, COLOR_WHITE, THEME_PINK);
  tft.fillRoundRect(6, 34, 160, 24, 6, THEME_NAVY);
  tft.drawRect(SCOPE_X - 1, SCOPE_TOP - 1, SCOPE_W + 2, SCOPE_H + 2, UI_LINE);
  tft.fillRoundRect(6, 146, 160, 140, 6, THEME_NAVY);
  drawCenteredString("CLICK = EXIT", 300, 1, UI_DIM, THEME_BG);
  scopeReset();

  unsigned long lastDraw = 0;
  String cacheState = "";
  uint32_t dropsAtEntry = det().drops;

  while (digitalRead(BTN_PIN) == HIGH) {
      feedWatchdog();
    serviceDropSensor(false);
    scopePush(det().edge);

    if (millis() - lastDraw >= 150) {
      lastDraw = millis();
      scopeDraw(SCOPE_TOP);

      String st = String(sensorStateWord());
      if (st != cacheState) {
        cacheState = st;
        tft.fillRect(8, 36, 156, 20, THEME_NAVY);
        drawCenteredString(st, 40, 2, sensorStateColor(), THEME_NAVY);
      }
      const iv::DetectorStatus &s = det();
      checkRow(152, "SIGNAL",  String(s.edge) + " / " + String(s.threshold), COLOR_WHITE);
      checkRow(170, "NOISE",   String(s.noiseSigma) + " adc  thr " + String(s.threshold), COLOR_WHITE);
      checkRow(188, "SNR",     String(s.snrX10 / 10) + "." + String(s.snrX10 % 10) + " x",
               (s.snrX10 >= 60) ? COLOR_GREEN : (s.snrX10 >= 30) ? COLOR_YELLOW : COLOR_ORANGE);
      checkRow(206, "DROP",    String(s.learnedAmp) + " adc  " + String(s.learnedWidthMs) + " ms", COLOR_WHITE);
      checkRow(224, "STEADY",  String(s.confidencePct) + " %", COLOR_WHITE);
      checkRow(242, "COUNTED", String(s.drops - dropsAtEntry), COLOR_WHITE);
      checkRow(260, "DIR",     String(detector.polarity() < 0 ? "drop = LOWER" : "drop = HIGHER"),
               detector.polarityLocked() ? UI_DIM : COLOR_YELLOW);
      checkRow(268 + 10, "ERR", "rej " + String(s.rejects) + "  gap " + String(s.gaps), UI_DIM);
    }
    delay(2);
  }

  modalSafeBeep(2400, 40);
  while (digitalRead(BTN_PIN) == LOW) { feedWatchdog(); delay(10); }
  delay(60);

  saveLearning();
  if (hasFirstDropOccurred) { lastDropTimestamp = millis(); }   // [1.0.0-TH] แก้: ไม่มี skipNextInterval แล้ว
  lastUserActivityTime = millis();
  stateNeedsRedraw = true;
}

// ----------------------------------------------------------------------------
// ESP-NOW Receive Callback (รับ Sync จาก Host)
// ----------------------------------------------------------------------------
#if defined(ESP_IDF_VERSION) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void OnHostDisplayRecv(const esp_now_recv_info_t *info, const uint8_t *incomingDataBytes, int len) {
  int rssi = (info && info->rx_ctrl) ? info->rx_ctrl->rssi : -55;
#else
void OnHostDisplayRecv(const uint8_t *mac, const uint8_t *incomingDataBytes, int len) {
  int rssi = -55;
#endif
  // [1.0.0-TH] แก้: รับ Protocol v4 เท่านั้น แพ็กเก็ต v3 (20 ไบต์) และของเครื่องอื่นถูกกรองออกด้วยความยาวกับรหัส
  if (len != sizeof(struct_down_v4)) return;
  if (incomingDataBytes[0] != V4_MAGIC_DOWN) return;

  struct_down_v4 d;
  memcpy(&d, incomingDataBytes, sizeof(d));
  lastHostRssi = rssi;
  lastHostRecvTime = millis();
  if (!isHostOnline) hostOnlineSince = millis();
  isHostOnline = true;
  hostReportedChannel = d.hostChannel;

  if (d.epochTime > 1600000000) {
    struct timeval tv = { (time_t)d.epochTime, 0 };
    settimeofday(&tv, NULL);
    isClockSynced = (d.flags & V4_DOWN_TIME_OK) != 0;
  }

  if (d.stationId == currentStationId) {
    // ค่าตั้งและรหัสเตือน
    targetRateHr  = d.targetRateHr;
    totalPlanMl   = d.planMl;
    hostAlertCode = d.alertCode;
    dropFactor    = safeDropFactor(d.dropFactor);
    nearEndPct    = safeNearPct(d.nearEndPct);
    hostNearEndAck = (d.flags & V4_DOWN_NEAR_ACK) != 0;

    // ผลคำนวณของ Host (ใช้แสดงผลอย่างเดียว)
    bool rateOk = (d.flags & V4_DOWN_RATE_OK) != 0;
    hostRateOk = rateOk;
    currentFlowRate_ml_hr = (rateOk && isRunning) ? d.rateHr : 0.0f;
    currentGttMin = (currentFlowRate_ml_hr * (float)dropFactor) / 60.0f;
    totalVolumeMl = d.infusedMl;
    hostMinutesLeft = d.minutesLeft;
    hostInfusedPct = d.infusedPct;
    hostPlanSet = (d.flags & V4_DOWN_PLAN_SET) != 0;

    lastOwnSyncTime = millis();
    if (d.flags & V4_DOWN_HEARS_YOU) lastHeardReportTime = millis();
    if (hostNearEndAck) nearEndAckRequest = false;   // Host ยืนยันแล้ว

    if (knownResetSeq < 0) {
      knownResetSeq = d.resetSeq;                 // ครั้งแรกหลังบูต: รับค่าโดยไม่รีเซ็ต
    } else if (d.resetSeq != (uint8_t)knownResetSeq) {
      knownResetSeq = d.resetSeq;
      pendingCounterReset = true;                  // ให้ loop() เป็นผู้รีเซ็ต
    }
  }
}

void applyCounterReset() {
  pendingCounterReset = false;
  totalDrops = 0;
  tallyClear(currentStationId);   // [7.9.0] เพิ่ม: เริ่มถุงใหม่ = ล้างยอดที่เก็บไว้ด้วย
  periodDropsCounter = 0;
  totalVolumeMl = 0;
  currentFlowRate_ml_hr = 0;
  currentGttMin = 0;
  hostMinutesLeft = V4_MINUTES_UNKNOWN;
  hostInfusedPct = 0;
  hasFirstDropOccurred = false;
  isDropFalling = false;
  snoozeUntilTime = 0;
  hostAlertCode = ALERT_NONE;
  hostNearEndAck = false;
  nearEndAckRequest = false;
  nearNextChimeTime = 0;
  nearChimeRemaining = 0;
  if (currentState == STATE_EMERGENCY || currentState == STATE_NEAR_END_NOTICE) currentState = STATE_NORMAL_VIEW;
  resetTrendData();                 // เริ่มถุงใหม่ = เริ่มกราฟแนวโน้มใหม่
  stateNeedsRedraw = true;
  beepNonBlocking(2400, 120);
}

// ----------------------------------------------------------------------------
// ค้นหาช่องสัญญาณของ Host
// ----------------------------------------------------------------------------
void setEspNowChannel(uint8_t ch) {
  if (esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE) == ESP_OK) {
    espnowChannel = ch;
  }
}

void manageChannelHunting(unsigned long now) {
  if (isHostOnline) {
    if (espnowChannel != savedChannel) {
      savedChannel = espnowChannel;
      stationPrefs.begin("st_cfg", false);
      stationPrefs.putUChar("ch", savedChannel);
      stationPrefs.end();
    }
    return;
  }
  if (now < CHANNEL_SCAN_START_MS) return;
  if (currentState == STATE_CONFIG_ID) return;

  if (now - lastChannelHopTime >= CHANNEL_SCAN_DWELL_MS) {
    lastChannelHopTime = now;
    uint8_t next = (espnowChannel % MAX_WIFI_CHANNEL) + 1;
    setEspNowChannel(next);
  }
}

void togglePause() {
  isRunning = !isRunning;
  currentFlowRate_ml_hr = 0.0f;
  currentGttMin = 0.0f;
  if (isRunning) {
    soundStart();
    // เริ่มนับเวลาใหม่หลังกลับมาทำงาน ไม่ให้ช่วงที่หยุดถูกนับเป็นสายพับหรือใช้คำนวณ rate
    if (hasFirstDropOccurred) {
      lastDropTimestamp = millis();   // ช่วงที่หยุดไม่นับเป็นเวลาจากหยดล่าสุด ไม่งั้น Host เห็นเป็นสายพับ
    }
  } else {
    soundStop();
    noTone(BUZZER_PIN);
  }
  stateNeedsRedraw = true;
}

String getStationClockStr() {
  if (!isClockSynced) return String("--:--");
  time_t now;
  time(&now);
  struct tm *timeinfo = localtime(&now);
  char buf[12];
  strftime(buf, sizeof(buf), "%H:%M", timeinfo);
  return String(buf);
}

// ============================================================================
// ส่วนแสดงผล (UI v2) — เน้นอ่านง่ายจากปลายเตียง ใช้ข้อความเท่าที่จำเป็น
//   หน้า 1 IV BAG  : ภาพถุงน้ำเกลือ + กระเปาะหยด + ตัวเลขสำคัญ 4 ค่า
//   หน้า 2 PLAN    : แผนการให้สารน้ำและเวลา (ตัวเลขใหญ่ 4 ค่า)
//   หน้า 3 LINK    : สถานะการเชื่อมต่อและเซนเซอร์
//   หน้า 4 TREND   : กราฟแท่งแนวโน้มอัตราไหลย้อนหลัง 30 นาที
// ทุกหน้าวาดเฉพาะส่วนที่ค่าเปลี่ยน เพื่อไม่ให้ SPI แย่งเวลาการอ่านเซนเซอร์
// ============================================================================

UiStatus uiStatusOf(uint8_t code) {
  if (!isRunning) return UI_PAUSED;
  // [1.0.0-TH] เพิ่ม: ไม่มีข้อมูลจาก Host ห้ามขึ้น NORMAL สีเขียว เพราะไม่มีใครยืนยันว่าปกติ
  if (!isHostOnline) return UI_NOHOST;
  switch (code) {
    case ALERT_OCCLUSION: return UI_NOFLOW;
    case ALERT_TOO_FAST:  return UI_FAST;
    case ALERT_TOO_SLOW:  return UI_SLOW;
    case ALERT_COMPLETE:  return UI_DONE;
    case ALERT_NEAR_END:  return UI_NEAREND;
    default:              return UI_OK;
  }
}

uint16_t uiStatusColor(UiStatus s) {
  switch (s) {
    case UI_OK:      return UI_GREEN;
    case UI_FAST:    return COLOR_ORANGE;
    case UI_SLOW:    return COLOR_YELLOW;
    case UI_NOFLOW:  return COLOR_RED;
    case UI_NEAREND: return COLOR_ORANGE;
    case UI_DONE:    return THEME_CYAN;
    case UI_NOHOST:  return COLOR_ORANGE;
    default:         return THEME_SKYBLUE;
  }
}

const char* uiStatusWord(UiStatus s) {
  switch (s) {
    case UI_OK:      return "NORMAL";
    case UI_FAST:    return "TOO FAST";
    case UI_SLOW:    return "TOO SLOW";
    case UI_NOFLOW:  return "NO FLOW";
    case UI_NEAREND: return "NEXT BAG";
    case UI_DONE:    return "COMPLETE";
    case UI_NOHOST:  return "NO HOST";
    default:         return "PAUSED";
  }
}


void textRight(const String &s, int xRight, int y, uint8_t size, uint16_t col, uint16_t bg) {
  textAt(s, xRight - (int)s.length() * 6 * size, y, size, col, bg);
}

void textCenterIn(const String &s, int x, int w, int y, uint8_t size, uint16_t col, uint16_t bg) {
  textAt(s, x + (w - (int)s.length() * 6 * size) / 2, y, size, col, bg);
}

// เติมช่องว่างท้ายข้อความให้ยาวคงที่ เพื่อเขียนทับของเดิมได้โดยไม่ต้องล้างพื้นที่ (ไม่กะพริบ)
String padTo(const String &s, unsigned int width) {
  String out = s;
  while (out.length() < width) out += ' ';
  return out;
}

// ---- ค่าที่ใช้ร่วมกันหลายหน้า ----
int remainingMlInt() {
  float r = totalPlanMl - totalVolumeMl;
  if (r < 0) r = 0;
  return (int)(r + 0.5f);
}

// [1.0.0-TH] แก้: เปอร์เซ็นต์และนาทีที่เหลือมาจาก Host (เดิมคำนวณในเครื่อง)
int infusedPct() {
  if (!hostPlanSet || !isHostOnline) return -1;
  return hostInfusedPct;
}

// นาทีที่เหลือจนหมดถุง (-1 = คำนวณไม่ได้)
int minutesLeft() {
  if (!isRunning || !isHostOnline || hostMinutesLeft == V4_MINUTES_UNKNOWN) return -1;
  return (int)hostMinutesLeft;
}

String timeLeftText(bool compact) {
  int m = minutesLeft();
  if (m < 0) return compact ? String("--h--m") : String("--h --m");
  char b[16];
  snprintf(b, sizeof(b), compact ? "%dh%02dm" : "%dh %02dm", m / 60, m % 60);
  return String(b);
}

String endClockText() {
  int m = minutesLeft();
  if (!isClockSynced || m < 0) return String("--:--");
  time_t now;
  time(&now);
  time_t fin = now + ((time_t)m * 60);
  char buf[8];
  strftime(buf, sizeof(buf), "%H:%M", localtime(&fin));
  return String(buf);
}





// ============================================================================
// หน้า 1: IV BAG — ภาพถุงน้ำเกลือ + กระเปาะหยด + ตัวเลขสำคัญ
// ============================================================================
#define BAG_X       12
#define BAG_Y       34
#define BAG_W       58
#define BAG_H       124
#define CH_X        26
#define CH_Y        176
#define CH_W        30
#define CH_H        46
#define DROP_X      41
#define RIGHT_X     84








void updatePage2Dynamic() {
  drawTopBar(false);

  String v1 = (targetRateHr > 0) ? String((int)(targetRateHr + 0.5f)) : String("--");
  String v2 = (totalPlanMl > 0) ? String((int)totalPlanMl) : String("--");
  String v3 = String((int)totalVolumeMl);
  String v4 = endClockText();

  if (v1 != cacheP2a) { cacheP2a = v1; drawStatCard(30,  "", v1, THEME_SKYBLUE, false); }
  if (v2 != cacheP2b) { cacheP2b = v2; drawStatCard(84,  "", v2, COLOR_WHITE,   false); }
  if (v3 != cacheP2c) { cacheP2c = v3; drawStatCard(138, "", v3, COLOR_WHITE,   false); }
  if (v4 != cacheP2d) { cacheP2d = v4; drawStatCard(192, "", v4, UI_GREEN,      false); }

  String foot = timeLeftText(false) + " left";
  String foot2 = (totalPlanMl > 0)
      ? ("NEXT BAG AT " + String(safeNearPct(nearEndPct)) + "%")
      : String("NEXT BAG AT --");
  String key = foot + foot2;
  if (key != cacheP2foot) {
    cacheP2foot = key;
    textCenterIn(padTo(foot, 12), 6, 160, 248, 2, COLOR_WHITE, THEME_BG);
    textCenterIn(padTo(foot2, 20), 6, 160, 274, 1, COLOR_ORANGE, THEME_BG);
  }
}



// ============================================================================
// หน้า 4: TREND — กราฟแท่งอัตราไหลย้อนหลัง 30 นาที (แท่งละ 1 นาที)
// ============================================================================
#define CHART_X    12
#define CHART_BARW 5
#define CHART_TOP  56
#define CHART_BOT  196

float trendValueAt(int i) {
  int idx = (trendHead + i) % TREND_BARS;
  return trendHas[idx] ? trendBin[idx] : 0.0f;
}

bool trendValidAt(int i) {
  int idx = (trendHead + i) % TREND_BARS;
  return trendHas[idx];
}

float niceScale(float v) {
  float s = 20.0f;
  while (s < v && s < 400.0f) s += (s < 100.0f) ? 10.0f : (s < 200.0f ? 20.0f : 50.0f);
  return s;
}




void updatePage4Dynamic() {
  drawTopBar(false);

  float sum = 0.0f;
  int cnt = 0;
  for (int i = 1; i < TREND_BARS; i++) {
    if (!trendValidAt(i)) continue;
    sum += trendValueAt(i);
    cnt++;
  }
  int avg = (cnt > 0) ? (int)(sum / cnt + 0.5f) : -1;
  int now = isRunning ? (int)(currentFlowRate_ml_hr + 0.5f) : -1;

  String key = String(now) + ":" + String(avg);
  if (key != cacheP4) {
    cacheP4 = key;
    textAt(padTo(now >= 0 ? String(now) : String("--"), 4), 14, 234, 3, COLOR_WHITE, UI_CARD);
    textAt(padTo(avg >= 0 ? String(avg) : String("--"), 4), 100, 238, 2, THEME_CYAN, UI_CARD);
  }
  if (trendNeedsRedraw) drawTrendChart();
  else                  drawTrendLiveBar();
}

// ============================================================================
// เก็บข้อมูลกราฟแนวโน้ม (ทำงานตลอดเวลา ไม่ว่าจะเปิดหน้าไหนอยู่)
// ============================================================================
void resetTrendData() {
  for (int i = 0; i < TREND_BARS; i++) { trendBin[i] = 0.0f; trendHas[i] = false; }
  trendHead = 0;
  trendSum = 0.0f;
  trendSamples = 0;
  trendLive = 0.0f;
  trendBinStart = millis();
  trendNeedsRedraw = true;
}

void updateTrendAccumulator(unsigned long now) {
  if (now - trendLastSample >= 1000) {
    trendLastSample = now;
    trendSum += isRunning ? currentFlowRate_ml_hr : 0.0f;
    trendSamples++;
    trendLive = (trendSamples > 0) ? (trendSum / trendSamples) : 0.0f;
  }
  if (now - trendBinStart >= TREND_BIN_MS) {
    trendBinStart = now;
    trendBin[trendHead] = (trendSamples > 0) ? (trendSum / trendSamples) : 0.0f;
    trendHas[trendHead] = true;
    trendHead = (trendHead + 1) % TREND_BARS;
    trendSum = 0.0f;
    trendSamples = 0;
    trendLive = 0.0f;
    trendNeedsRedraw = true;
  }
}




void updateNearEndNoticeDynamic() {
  int pct = infusedPct();
  int left = remainingMlInt();
  String end = endClockText();
  String key = String(pct) + ":" + String(left) + ":" + end;
  if (key == cacheNear) return;
  cacheNear = key;

  textCenterIn(padTo("INFUSED " + String(pct < 0 ? 0 : pct) + "%", 12), 12, 148, 126, 2, COLOR_WHITE, THEME_BG);
  textCenterIn(padTo(String(left) + " mL LEFT", 12), 12, 148, 152, 2, COLOR_ORANGE, THEME_BG);
  textCenterIn(padTo("EMPTY AT " + end, 18), 12, 148, 180, 1, COLOR_YELLOW, THEME_BG);
  drawProgressBar(26, 196, 120, 10, (pct < 0) ? 0 : pct, COLOR_ORANGE);
}





void saveScreenRotation() {
  stationPrefs.begin("st_cfg", false);
  stationPrefs.putUChar("rot", screenRotation);
  stationPrefs.end();
}

const char *screenRotationLabel() {
  return (screenRotation == TFT_ROT_FLIP180) ? "FLIP 180" : "NORMAL";
}



// ----------------------------------------------------------------------------
// ปุ่มกด
// ----------------------------------------------------------------------------
void handleButton() {
  static unsigned long btnPressStart = 0;
  static bool isHolding = false;
  static int clickCount = 0;
  static unsigned long lastReleaseTime = 0;

  bool reading = (digitalRead(BTN_PIN) == LOW);
  unsigned long now = millis();

  if (reading && !isHolding) {
    btnPressStart = now;
    isHolding = true;
    isHoldUiActive = false;
    holdBeep3sDone = false;
    holdBeep5sDone = false;
    lastUserActivityTime = now;
  }
  else if (reading && isHolding) {
    unsigned long holdDur = now - btnPressStart;
    if (holdDur >= 400 && currentState != STATE_CONFIG_ID) {
      isHoldUiActive = true;
      if (holdDur >= 3000 && !holdBeep3sDone) {
        beepNonBlocking(2200, 50);
        holdBeep3sDone = true;
      }
      if (holdDur >= 5000 && !holdBeep5sDone) {
        beepNonBlocking(2600, 80);
        holdBeep5sDone = true;
      }
      if (now - lastHoldRenderTime >= 35) {
        lastHoldRenderTime = now;
        drawHoldProgressHUD(holdDur);
      }
    }
  }
  else if (!reading && isHolding) {
    unsigned long pressDur = now - btnPressStart;
    isHolding = false;
    lastUserActivityTime = now;

    if (isHoldUiActive) {
      isHoldUiActive = false;
      clickCount = 0;
      if (pressDur >= 5000) {
        soundScreensaver();
        currentState = STATE_CREDIT;
        stateNeedsRedraw = true;
      } else if (pressDur >= 3000) {
        executeButtonCalibrationWizard();
      } else {
        soundClick();
        stateNeedsRedraw = true;
      }
      return;
    }
    else if (pressDur > 30 && pressDur < 400) {
      clickCount++;
      lastReleaseTime = now;
    }
  }

  if (clickCount > 0 && !isHolding && (now - lastReleaseTime > 280)) {
    int clicks = clickCount;
    clickCount = 0;

    if (currentState == STATE_CONFIG_ID) {
      soundClick();
      if (clicks >= 2) {
        // ดับเบิลคลิก = สลับจอกลับหัว 180 องศา เห็นผลทันที แล้วจำค่าไว้เลย
        applyScreenRotation(screenRotation == TFT_ROT_FLIP180 ? TFT_ROT_NORMAL
                                                              : TFT_ROT_FLIP180);
        saveScreenRotation();
      } else {
        tempConfigStationId = (tempConfigStationId % 8) + 1;
      }
      configAutoSaveTimeout = now + 4000;
      return;
    }

    if (currentState == STATE_EMERGENCY) {
      if (clicks >= 2) {
        togglePause();                         // หยุดนับ เช่น ขณะเปลี่ยนถุง/แก้สายพับ
      } else {
        snoozeUntilTime = now + SNOOZE_MS;
        noTone(BUZZER_PIN);
        buzzerBeepUntil = 0;
        soundClick();
      }
      currentState = STATE_NORMAL_VIEW;
      stateNeedsRedraw = true;
      return;
    }

    if (currentState == STATE_NEAR_END_NOTICE) {
      acknowledgeNearEnd();                    // คลิกกี่ครั้งก็ได้ = รับทราบ
      currentState = STATE_NORMAL_VIEW;
      currentNursePage = 1;
      stateNeedsRedraw = true;
      return;
    }

    if (currentState == STATE_CREDIT) {
      soundClick();
      currentState = STATE_NORMAL_VIEW;
      stateNeedsRedraw = true;
      return;
    }

    if (clicks >= 3) {
      soundClick();
      noTone(BUZZER_PIN);
      tempConfigStationId = currentStationId;
      configAutoSaveTimeout = now + 4000;
      currentState = STATE_CONFIG_ID;
      stateNeedsRedraw = true;
    }
    else if (clicks == 2) {
      togglePause();
      if (currentState == STATE_SCREENSAVER) currentState = STATE_NORMAL_VIEW;
    }
    else {
      soundClick();
      if (currentState == STATE_SCREENSAVER) {
        currentState = STATE_NORMAL_VIEW;
      } else {
        currentNursePage = (currentNursePage % 4) + 1;
      }
      stateNeedsRedraw = true;
    }
  }
}

// ---------------------------------------------------------------------------
// โค้ดวาดจอทั้งหมดถูกแยกไปไว้ที่ไฟล์นี้ เพื่อให้ไฟล์หลักสั้นลงและหาของเจอเร็วขึ้น
// ต้อง #include ตรงนี้เท่านั้น คือหลังตัวแปรและฟังก์ชันช่วยทั้งหมด แต่ก่อน setup()
// ห้ามย้ายขึ้นไปบนสุด เพราะโค้ดในไฟล์นั้นใช้ตัวแปรและฟังก์ชันที่ประกาศไว้ด้านบน
// ---------------------------------------------------------------------------
#include "StationScreen.h"

// ----------------------------------------------------------------------------
// Setup
// ----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  setenv("TZ", "ICT-7", 1);
  tzset();

  pinMode(BUZZER_PIN, OUTPUT); noTone(BUZZER_PIN);
  pinMode(TFT_BLK, OUTPUT); digitalWrite(TFT_BLK, HIGH);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(SENSOR_AO_PIN, INPUT);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  iv::DetectorConfig dcfg;              // ค่าเริ่มต้นเหมาะกับเซนเซอร์ IR ของโครงการนี้
  detector.begin(dcfg);
  detector.reset(analogRead(SENSOR_AO_PIN));

  SPI_TFT.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(172, 320);
  tft.setSPISpeed(40000000);
  tft.setRotation(TFT_ROTATION_DEFAULT);
  tft.setTextWrap(false);

  stationPrefs.begin("st_cfg", false);
  currentStationId = stationPrefs.getUChar("id", 1);
  savedChannel     = stationPrefs.getUChar("ch", DEFAULT_ESPNOW_CHANNEL);
  uint8_t savedRot = stationPrefs.getUChar("rot", TFT_ROTATION_DEFAULT);
  stationPrefs.end();
  applyScreenRotation(savedRot);   // ทิศจอที่เคยตั้งไว้ที่หน้าเครื่อง
  if (currentStationId < 1 || currentStationId > 8) currentStationId = 1;
  // [7.9.0] เพิ่ม: กู้จำนวนหยดสะสมของเตียงนี้กลับมา ไฟดับแล้วยอดต้องไม่หาย
  totalDrops    = tallyRestore(currentStationId);
  totalVolumeMl = (float)totalDrops / (float)safeDropFactor(dropFactor);

  // ---- กันสเตชันหลายตัวส่งชนกัน ----
  // เมล็ดสุ่มมาจาก MAC (ไม่ซ้ำกันทุกบอร์ด) และรอบแรกเหลื่อมกันตามเลขเตียง
  // ทำให้แพ็กเก็ตของแต่ละเตียงกระจายกันทั้งวินาทีตั้งแต่บูต ไม่ต้องรอให้ jitter ค่อย ๆ แยก
  {
    uint8_t mac[6] = {0};
    WiFi.macAddress(mac);
    randomSeed(((uint32_t)mac[5] << 16) ^ ((uint32_t)mac[4] << 8) ^ (uint32_t)mac[3]
               ^ ((uint32_t)currentStationId << 24) ^ (uint32_t)micros());
    lastSendTime  = millis();
    nextSendDelay = (unsigned long)currentStationId * (SEND_INTERVAL_MS / 9);
  
  bootResetReason = esp_reset_reason();   // [7.9.0] เพิ่ม: จำไว้ว่ารีบูตครั้งล่าสุดเพราะอะไร
  Serial.printf("[boot] reset reason: %s\n", resetReasonText(bootResetReason));
  setupLoopWatchdog();
}
  if (savedChannel < 1 || savedChannel > MAX_WIFI_CHANNEL) savedChannel = DEFAULT_ESPNOW_CHANNEL;

  loadLearning();   // ใส่สิ่งที่เคยเรียนรู้ไว้กลับเข้าไป ให้พร้อมใช้งานเร็วขึ้น

  setLedColor(0, 40, 80);
  resetTrendData();
  trendLastSample = millis();

  tft.fillScreen(THEME_BG);
  drawCenteredString("SMART IV", 60, 3, THEME_SKYBLUE, THEME_BG);
  drawCenteredString("ALERT", 92, 2, THEME_PINK, THEME_BG);

  char bedBuf[12];
  snprintf(bedBuf, sizeof(bedBuf), "BED %02d", currentStationId);
  tft.fillRoundRect(26, 140, 120, 48, 8, THEME_SKYBLUE);
  drawCenteredString(String(bedBuf), 154, 3, THEME_BG, THEME_SKYBLUE);

  drawCenteredString("Bed Station Node", 214, 1, UI_DIM, THEME_BG);
  drawCenteredString("FW v" APP_VERSION, 232, 1, THEME_SKYBLUE, THEME_BG);
  drawCenteredString("BCN Phrae Innovation", 256, 1, UI_DIM, THEME_BG);

  playWelcomeMelody();
  delay(600);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setSleep(false);
  setEspNowChannel(savedChannel);

  if (esp_now_init() != ESP_OK) {
    drawCenteredString("ESP-NOW INIT FAILED", 270, 1, COLOR_RED, THEME_BG);
    Serial.println("[STATION] ESP-NOW init FAILED");
  } else {
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnHostDisplayRecv));

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;              // 0 = ใช้ช่องปัจจุบัน (รองรับการเปลี่ยนช่องระหว่างค้นหา Host)
    peerInfo.ifidx = WIFI_IF_STA;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }

  currentState = STATE_NORMAL_VIEW;
  currentNursePage = 1;
  stateNeedsRedraw = true;
  lastUserActivityTime = millis();
  lastChannelHopTime = millis();
}

// ----------------------------------------------------------------------------
// Loop
// ----------------------------------------------------------------------------
void loop() {
  feedWatchdog();   // [7.9.0] เพิ่ม: บอกสุนัขเฝ้าบ้านว่ายังเดินอยู่
  serviceDropSensor(isRunning);   // อ่านเซนเซอร์ + เดินอัลกอริทึม (ไม่นับหยดตอนหยุดชั่วคราว)
  handleButton();

  unsigned long currentMillis = millis();

  if (pendingCounterReset) applyCounterReset();

  // ตรวจสอบการเชื่อมต่อกับ Host หากเกิน 5 วินาทีให้ถือว่าหลุด
  unsigned long lr = lastHostRecvTime;
  if (lr == 0 || (currentMillis - lr > HOST_TIMEOUT_MS)) {   // [7.9.0] แก้: ตัด currentMillis > lr ทิ้ง เพราะทำให้การวนรอบของ millis() พัง
    isHostOnline = false;
  }
  manageChannelHunting(currentMillis);

  // [1.0.0-TH] แก้: ไม่ตัดสินสายพับและไม่คำนวณปริมาตรในเครื่องแล้ว (ใช้ค่าจาก Host)
  // Host หาย = ไม่มีผลคำนวณให้แสดง ไม่ค้างค่าเก่าที่ไม่มีใครยืนยัน
  //   อัตราไหลเป็น 0 (จอขึ้น --) ส่วนปริมาตรที่ให้แล้วใช้ "หยดสะสม / drop factor" ซึ่งเป็นข้อเท็จจริง
  //   ล้วน ๆ ไม่ใช่การประเมินทางคลินิก (เปอร์เซ็นต์ นาทีที่เหลือ และรหัสเตือนยังต้องรอ Host)
  if (!isHostOnline) {
    currentFlowRate_ml_hr = 0.0f;
    currentGttMin = 0.0f;
    totalVolumeMl = (float)totalDrops / (float)safeDropFactor(dropFactor);
  }
  serviceTallySave(currentMillis, totalDrops, currentStationId);   // [7.9.0] เพิ่ม

  uint8_t alertCode = effectiveAlertCode();
  uiAlertCode = alertCode;                                // ให้ทุกหน้าจอใช้รหัสเดียวกัน
  updateTrendAccumulator(currentMillis);                  // เก็บข้อมูลกราฟตลอดเวลา
  if (!isCriticalAlert(alertCode)) snoozeUntilTime = 0;   // เหตุการณ์หายไปแล้ว ยกเลิก snooze
  handlePassiveBuzzerEngine(alertCode);
  if (currentMillis - lastBatCheck >= 1000) {        // [1.1.0-TH] เพิ่ม: ตรวจแบตเตอรี่วินาทีละครั้ง
    lastBatCheck = currentMillis;
    serviceBattery(currentMillis, readStationBattery());
  }
  serviceBatteryChime(currentMillis, alertCode);
  updateStatusLed(alertCode);

  // ---- ส่งข้อมูลให้ Host ทุก 1 วินาที (ส่งแม้อยู่ในหน้าตั้งค่า) ----
  if (currentMillis - lastSendTime >= nextSendDelay) {
    // [1.0.0-TH] แก้: Protocol v4 ขึ้น Host ส่งแค่ข้อเท็จจริงจากเซนเซอร์
    myData.magic           = V4_MAGIC_UP;
    myData.stationId       = currentStationId;
    myData.flags           = (isRunning ? V4_UP_RUNNING : 0) |
                             (nearEndAckRequest ? V4_UP_NEAR_ACK : 0) |
                             (sensorHasDrops ? V4_UP_SENSOR_SEEN : 0) |
                             (det().lock == iv::Lock::NoDrops ? V4_UP_SENSOR_FAULT : 0);
    myData.seq             = upSeq++;
    myData.totalDrops      = totalDrops;
    myData.msSinceLastDrop = hasFirstDropOccurred ? (uint32_t)(currentMillis - lastDropTimestamp) : 0;
    if (myData.msSinceLastDrop == 0 && hasFirstDropOccurred) myData.msSinceLastDrop = 1;
    {
      float bv = readStationBattery();
      myData.batteryMv = (bv > 0.0f) ? (uint16_t)(bv * 1000.0f + 0.5f) : 0;
    }

    if (esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData)) == ESP_OK) {
      periodDropsCounter = 0;
    }
    lastSendTime = currentMillis;
    // สุ่มคาบถัดไปใหม่เสมอ สองสเตชันที่บังเอิญส่งพร้อมกันจึงแยกจากกันในรอบถัดไปทันที
    nextSendDelay = (unsigned long)(SEND_INTERVAL_MS - SEND_JITTER_MS)
                  + (unsigned long)random(0, 2 * SEND_JITTER_MS + 1);
  }

  if (currentState == STATE_CONFIG_ID) {
    if (stateNeedsRedraw) drawStationIdConfigFramework();
    updateStationIdConfigDynamic();

    if (currentMillis >= configAutoSaveTimeout) {
      if (tempConfigStationId != currentStationId) {
        currentStationId = tempConfigStationId;
        stationPrefs.begin("st_cfg", false);
        stationPrefs.putUChar("id", currentStationId);
        stationPrefs.end();
        // [7.9.0] เพิ่ม: ย้ายยอดสะสมไปอยู่ใต้เลขเตียงใหม่ ไม่งั้นบูตหน้าจะกู้ไม่เจอ
        tallyWriteNvs(totalDrops, currentStationId);
        // ล้างค่าของเตียงเดิม รอค่าของเตียงใหม่จาก Host
        targetRateHr = 0;
        totalPlanMl = 0;
        hostAlertCode = ALERT_NONE;
        knownResetSeq = -1;
      }
      soundStart();
      currentState = STATE_NORMAL_VIEW;
      stateNeedsRedraw = true;
    }
    return;
  }

  // ---- จัดการสถานะเตือน ----
  bool critical = isCriticalAlert(alertCode);
  if (critical && currentMillis >= snoozeUntilTime) {
    if (currentState != STATE_EMERGENCY && !isHoldUiActive) {
      currentState = STATE_EMERGENCY;
      stateNeedsRedraw = true;
    } else if (currentState == STATE_EMERGENCY && alertCode != lastEmergencyCode) {
      stateNeedsRedraw = true;
    }
  } else if (currentState == STATE_EMERGENCY) {
    currentState = STATE_NORMAL_VIEW;
    stateNeedsRedraw = true;
  }

  // ---- เตือนใกล้หมด (ไม่ใช่เหตุวิกฤต) ----
  if (!critical && isNearEndPending(alertCode) && !isHoldUiActive &&
      (currentState == STATE_NORMAL_VIEW || currentState == STATE_SCREENSAVER)) {
    currentState = STATE_NEAR_END_NOTICE;
    stateNeedsRedraw = true;
  } else if (currentState == STATE_NEAR_END_NOTICE && (critical || !isNearEndPending(alertCode))) {
    currentState = critical ? STATE_EMERGENCY : STATE_NORMAL_VIEW;
    stateNeedsRedraw = true;
  }

  // ---- Screensaver เมื่อไม่มีการใช้งาน ----
  if (currentState == STATE_NORMAL_VIEW && !isHoldUiActive &&
      currentMillis - lastUserActivityTime >= SCREENSAVER_IDLE_MS) {
    currentState = STATE_SCREENSAVER;
    stateNeedsRedraw = true;
  }

  if (!isHoldUiActive) {
    if (currentState == STATE_EMERGENCY) {
      if (stateNeedsRedraw) { drawEmergencyScreen(alertCode); updateEmergencyDynamic(); }
    }
    else if (currentState == STATE_NORMAL_VIEW) {
      if (currentNursePage == 1) {
        if (stateNeedsRedraw) { drawPage1Framework(); updatePage1Dynamic(); }
        updateDripAnimation();
      } else if (currentNursePage == 2) {
        if (stateNeedsRedraw) { drawPage2Framework(); updatePage2Dynamic(); }
      } else if (currentNursePage == 3) {
        if (stateNeedsRedraw) { drawPage3Framework(); updatePage3Dynamic(); }
      } else if (currentNursePage == 4) {
        if (stateNeedsRedraw) { drawPage4Framework(); updatePage4Dynamic(); }
      }
    }
    else if (currentState == STATE_SCREENSAVER) {
      if (stateNeedsRedraw) { drawScreensaverFramework(); updateScreensaverDynamic(); }
    }
    else if (currentState == STATE_NEAR_END_NOTICE) {
      if (stateNeedsRedraw) { drawNearEndNoticeFramework(); updateNearEndNoticeDynamic(); }
    }
    else if (currentState == STATE_CREDIT) {
      if (stateNeedsRedraw) drawDeveloperCreditScreen();
    }
  }

  static unsigned long lastDynamicUpdate = 0;
  if (currentMillis - lastDynamicUpdate >= 1000) {
    lastDynamicUpdate = currentMillis;

    if (!isHoldUiActive) {
      if (currentState == STATE_NORMAL_VIEW) {
        if (currentNursePage == 1) updatePage1Dynamic();
        else if (currentNursePage == 2) updatePage2Dynamic();
        else if (currentNursePage == 3) updatePage3Dynamic();
        else if (currentNursePage == 4) updatePage4Dynamic();
      } else if (currentState == STATE_SCREENSAVER) {
        updateScreensaverDynamic();
      } else if (currentState == STATE_EMERGENCY) {
        updateEmergencyDynamic();
      } else if (currentState == STATE_NEAR_END_NOTICE) {
        updateNearEndNoticeDynamic();
      }
    }
  }
}
