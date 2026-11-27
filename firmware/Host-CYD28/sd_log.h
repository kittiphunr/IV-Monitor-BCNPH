/**
 * @file      sd_log.h
 * @brief     บันทึกเหตุการณ์ลงการ์ด SD เป็นไฟล์ CSV
 * @version   2.0.3-CYD
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
