/**
 * @file      sd_log.h
 * @brief     บันทึกเหตุการณ์ลงการ์ด SD เป็นไฟล์ CSV
 * @version   2.4.0-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * เปิดการ์ดบนบัส VSPI แยกจากจอ ถ้าไม่มีการ์ดหรือเขียนล้มเหลวติดกันสามครั้งจะเลิกใช้การ์ดเงียบๆ ไม่ให้กระทบการเตือน ไฟล์ /events.csv คอลัมน์ epoch,upSec,bed,event,detail (epoch เป็น 0 ถ้ายังไม่ได้ตั้งนาฬิกา)
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.4.0-CYD | 2026-11-27 | คิวเหตุการณ์เขียนเป็นก้อน (เปิดไฟล์ครั้งเดียวต่อรอบ ไม่ใช่ต่อเหตุการณ์) · หมุน events.csv เป็น events.old.csv เมื่อเกิน 1 MB · ต่อการ์ดใหม่ทุก 60 วินาทีถ้าเคยใช้ได้แต่หลุด · ตัดการเรียก exists() ต่อนาทีของไฟล์แนวโน้ม |
 * | 2.2.0-CYD | 2026-11-27 | เขียนตัวอย่างรายนาทีลง /trend/<วัน>.csv (วันตามเขตเวลาที่เบราว์เซอร์ส่งมา ไม่มีนาฬิกา = unsync.csv) พร้อมหัวคอลัมน์เมื่อสร้างไฟล์ใหม่ · รายชื่อวันที่มีไฟล์ · ขนาดไฟล์เหตุการณ์ |
 * | 2.0.3-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 4 (เว็บ นาฬิกา SD ประวัติเหตุการณ์) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include "board.h"
#include "event_log.h"
#include "trend.h"

static SPIClass spiSd(VSPI);
static bool sdOk = false;
static bool sdWasOk = false;                          // เคยใช้การ์ดได้ในรอบนี้: ถ้าหลุดกลางทางจะลองต่อใหม่เป็นระยะ (ไม่ลองถ้าไม่เคยมีการ์ด จะได้ไม่หน่วงลูปเปล่า ๆ)
static uint8_t sdFails = 0;

// [2.4.0-CYD] เพิ่ม: คิวเหตุการณ์ + เขียนเป็นก้อน + หมุนไฟล์ + ต่อการ์ดใหม่
// เดิมเปิด-ปิดไฟล์ต่อเหตุการณ์ ซึ่งบล็อกลูปหลายสิบ ms ต่อครั้ง (เตียงขาดสัญญาณพร้อมกันหลายเตียง = ลูปค้างเป็นร้อย ms ทัชหน่วง แพ็กเก็ต ESP-NOW ล้นคิว)
constexpr int SD_EVQ_CAP = 16;                        // เหตุการณ์ที่รอเขียนได้สูงสุด (เต็มก่อนถึงเวลาจะเขียนทันที)
constexpr uint32_t SD_EV_FLUSH_MS = 1000;             // รอเก็บเป็นก้อนอย่างน้อยเท่านี้ก่อนเขียน
constexpr uint32_t SD_EVENTS_MAX_BYTES = 1048576;     // events.csv โตเกินนี้ย้ายเป็น events.old.csv (เก็บ 1 รุ่น) กันการ์ดเต็ม
constexpr uint32_t SD_RETRY_MS = 60000;               // ต่อการ์ดใหม่ทุกกี่ ms หลังหลุด
static char sdEvQ[SD_EVQ_CAP][64];
static uint8_t sdEvN = 0;
static uint32_t sdEvFirstMs = 0;
static uint32_t sdEvLost = 0;                         // เหตุการณ์ที่ทิ้งเพราะเขียนไม่ได้ (นับไว้ดู ไม่โชว์ในจอ)
static uint32_t sdLastRetryMs = 0;

static bool sdLogBegin() {
  spiSd.begin(cyd::SD_SCK, cyd::SD_MISO, cyd::SD_MOSI, cyd::SD_CS);
  sdOk = SD.begin(cyd::SD_CS, spiSd, 4000000);
  if (sdOk && !SD.exists("/events.csv")) {
    File f = SD.open("/events.csv", FILE_APPEND);
    if (f) { f.print("epoch,upSec,bed,event,detail\n"); f.close(); } else sdOk = false;
  }
  if (sdOk && !SD.exists("/trend")) SD.mkdir("/trend");
  if (sdOk) { sdWasOk = true; sdFails = 0; }
  return sdOk;
}

static const char *sdStatusText() { return sdOk ? "OK" : (sdWasOk ? "RETRY" : "NO"); }

// เขียนเหตุการณ์ที่ค้างในคิวทั้งหมดด้วยการเปิดไฟล์ครั้งเดียว
static void sdFlushEvents() {
  if (sdEvN == 0) return;
  if (!sdOk) { sdEvLost += sdEvN; sdEvN = 0; return; }
  File f = SD.open("/events.csv", FILE_APPEND);
  if (f && f.size() > SD_EVENTS_MAX_BYTES) {          // หมุนไฟล์: ย้ายของเดิมเป็น .old แล้วเริ่มไฟล์ใหม่พร้อมหัวคอลัมน์
    f.close();
    SD.remove("/events.old.csv");
    SD.rename("/events.csv", "/events.old.csv");
    f = SD.open("/events.csv", FILE_APPEND);
    if (f) f.print("epoch,upSec,bed,event,detail\n");
  }
  if (!f) { sdEvLost += sdEvN; sdEvN = 0; if (++sdFails >= 3) sdOk = false; return; }   // การ์ดหลุด/เต็ม: เลิกใช้ ไม่ให้ลูปช้าลง
  for (uint8_t i = 0; i < sdEvN; i++) f.print(sdEvQ[i]);
  f.close();
  sdEvN = 0;
  sdFails = 0;
}

static void sdLogEvent(const EventRec &e) {
  if (!sdOk) return;
  if (sdEvN >= SD_EVQ_CAP) sdFlushEvents();           // คิวเต็ม: เขียนเดี๋ยวนี้ (กรณีเหตุการณ์ถี่ผิดปกติ)
  if (!sdOk) return;
  if (!eventCsvLine(sdEvQ[sdEvN], sizeof(sdEvQ[0]), e)) return;
  if (sdEvN == 0) sdEvFirstMs = millis();
  sdEvN++;
}

// เรียกจากลูปหลัก: เขียนคิวเมื่อครบเวลา และลองต่อการ์ดใหม่ถ้าเคยใช้ได้แต่หลุด (ครั้งละไม่เกินหนึ่งงาน)
static void sdLogService(uint32_t now) {
  if (sdOk) {
    if (sdEvN > 0 && (uint32_t)(now - sdEvFirstMs) >= SD_EV_FLUSH_MS) sdFlushEvents();
    return;
  }
  if (!sdWasOk || (uint32_t)(now - sdLastRetryMs) < SD_RETRY_MS) return;
  sdLastRetryMs = now;
  SD.end();
  sdLogBegin();                                       // สำเร็จจะ sdOk = true และกลับมาเขียนต่อได้เอง
}

// เส้นทางไฟล์แนวโน้มของวันนั้น (epoch = 0 หรือไม่มีนาฬิกา -> unsync.csv)
static void sdTrendPath(char *out, size_t cap, uint32_t epoch, int tzMin) {
  if (epoch == 0) { snprintf(out, cap, "/trend/unsync.csv"); return; }
  char day[9];
  trendDayName(epoch, tzMin, day);
  snprintf(out, cap, "/trend/%s.csv", day);
}

// เขียนตัวอย่างรายนาทีของทุกเตียงลงไฟล์ของวันนั้น (เฉพาะนาทีที่มีข้อมูล) เปิดไฟล์ครั้งเดียวต่อนาที
static void sdLogTrend(const TrendSample *samples, int active, uint32_t epoch, uint32_t upMin, int tzMin) {
  if (!sdOk) return;
  char path[32];
  sdTrendPath(path, sizeof(path), epoch, tzMin);
  File f = SD.open(path, FILE_APPEND);                 // [2.4.0-CYD] แก้: ไฟล์ว่าง = ไฟล์ใหม่ ไม่ต้อง exists() แยก (ลดการเข้าถึงการ์ดหนึ่งครั้งต่อนาที)
  if (!f) { if (++sdFails >= 3) sdOk = false; return; }
  if (f.size() == 0) f.print(TREND_CSV_HEADER);
  char line[96];
  for (int i = 0; i < active; i++) {
    if (!(samples[i].flags & TS_VALID)) continue;
    if (trendCsvLine(line, sizeof(line), epoch, upMin, i + 1, samples[i])) f.print(line);
  }
  f.close();
  sdFails = 0;
}

// รายชื่อวันที่มีไฟล์แนวโน้ม (ชื่อไฟล์ 8 หลักหรือ unsync) คืนจำนวน ชื่อเก็บใน days[i] (ยาว 9 ไบต์รวมตัวจบ) ทนทั้งชื่อเต็มพาธและชื่อสั้นของแต่ละเวอร์ชัน core
static int sdListDays(char days[][9], int maxDays) {
  if (!sdOk) return 0;
  File dir = SD.open("/trend");
  if (!dir) return 0;
  int n = 0;
  for (File f = dir.openNextFile(); f && n < maxDays; f = dir.openNextFile()) {
    const char *nm = f.name();
    const char *slash = strrchr(nm, '/');
    if (slash) nm = slash + 1;
    const size_t len = strlen(nm);
    if (!strcmp(nm, "unsync.csv") && n < maxDays) { strcpy(days[n], "unsync"); n++; continue; }
    if (len == 12 && !strcmp(nm + 8, ".csv")) { bool digits = true; for (int i = 0; i < 8; i++) if (nm[i] < '0' || nm[i] > '9') digits = false; if (digits) { memcpy(days[n], nm, 8); days[n][8] = 0; n++; } }
  }
  return n;
}
