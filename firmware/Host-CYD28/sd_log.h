/**
 * @file      sd_log.h
 * @brief     บันทึกเหตุการณ์ลงการ์ด SD เป็นไฟล์ CSV
 * @version   2.2.0-CYD
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
static uint8_t sdFails = 0;

static bool sdLogBegin() {
  spiSd.begin(cyd::SD_SCK, cyd::SD_MISO, cyd::SD_MOSI, cyd::SD_CS);
  sdOk = SD.begin(cyd::SD_CS, spiSd, 4000000);
  if (sdOk && !SD.exists("/events.csv")) {
    File f = SD.open("/events.csv", FILE_APPEND);
    if (f) { f.print("epoch,upSec,bed,event,detail\n"); f.close(); } else sdOk = false;
  }
  if (sdOk && !SD.exists("/trend")) SD.mkdir("/trend");
  return sdOk;
}

static void sdLogEvent(const EventRec &e) {
  if (!sdOk) return;
  char line[64];
  const int n = eventCsvLine(line, sizeof(line), e);
  File f = n ? SD.open("/events.csv", FILE_APPEND) : File();
  if (f) { f.print(line); f.close(); sdFails = 0; return; }
  if (++sdFails >= 3) sdOk = false;                   // การ์ดหลุด/เต็ม: เลิกใช้ ไม่ให้ลูปช้าลง
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
  const bool isNew = !SD.exists(path);
  File f = SD.open(path, FILE_APPEND);
  if (!f) { if (++sdFails >= 3) sdOk = false; return; }
  if (isNew) f.print(TREND_CSV_HEADER);
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
