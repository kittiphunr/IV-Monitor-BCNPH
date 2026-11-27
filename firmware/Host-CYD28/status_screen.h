/**
 * @file      status_screen.h
 * @brief     หน้าสถานะเตียงแบบตาราง (ชั่วคราวจนกว่าด่านที่ 3)
 * @version   2.0.1-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * แสดงสถานะ อัตราไหล ปริมาตร คุณภาพลิงก์ และ RSSI ของทุกเตียงที่เปิดใช้เป็นตารางตัวอักษร ใช้ตรวจแกนเครือข่ายบนบอร์ดจริงก่อนมีหน้าจอสุดท้าย พร้อมฟังก์ชันตั้งชื่อสถานะที่ทดสอบได้บน PC
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.1-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 2 (แกนเครือข่าย) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <stdio.h>
#include "display.h"
#include "colors.h"
#include "bed_model.h"

static const char *bedStateText(const Bed &b, uint32_t now) {
  if (!b.seen) return "NO DATA";
  if (!bedOnline(b, now)) return bedLostLink(b, now) ? "LOST" : "OFFLINE";
  if (!b.running) return "PAUSED";
  switch (b.alert) {
    case ALERT_TOO_FAST:  return "TOO FAST";
    case ALERT_TOO_SLOW:  return "TOO SLOW";
    case ALERT_NEAR_END:  return "NEAR END";
    case ALERT_COMPLETE:  return "DONE";
    case ALERT_OCCLUSION: return "NO FLOW";
    case ALERT_NO_SIGNAL: return "NO DROP";
    default:              return "OK";
  }
}

static uint16_t bedStateColor(const Bed &b, uint32_t now) {
  if (!b.seen || !bedOnline(b, now)) return C_GRAY;
  if (!b.running) return C_CYAN;
  if (isCriticalAlert(b.alert)) return C_RED;
  if (b.alert == ALERT_NEAR_END) return C_YELLOW;
  return C_GREEN;
}

static void statusDrawHeader() {
  tft.fillScreen(C_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(C_WHITE, C_BLACK);
  tft.setCursor(2, 2);
  tft.print("BED STATE     RATE   VOL  LINK RSSI");
  tft.drawFastHLine(0, 12, tft.width(), C_GRAY);
}

// วาดทุกแถว (ข้อความยาวคงที่ จึงทับของเดิมได้โดยไม่กะพริบ)
static void statusDrawRows(const Bed *beds, int active, uint32_t now, uint8_t channel, uint32_t dropped, uint32_t bad) {
  tft.setTextSize(1);
  for (int i = 0; i < active && i < MAX_BEDS; i++) {
    const Bed &b = beds[i];
    char line[56];
    snprintf(line, sizeof(line), "%d  %-8s %5.0f %5.0f %3d%% %4d%s ", i + 1, bedStateText(b, now),
             (double)b.flowRateHr, (double)bedVolumeMl(b), (int)b.linkPct, (int)b.rssi, b.idConflict ? " ID!" : "    ");
    tft.setTextColor(bedStateColor(b, now), C_BLACK);
    tft.setCursor(2, 18 + i * 14);
    tft.print(line);
  }
  char foot[56];
  snprintf(foot, sizeof(foot), "ch %u  drop %lu  bad %lu    ", (unsigned)channel, (unsigned long)dropped, (unsigned long)bad);
  tft.setTextColor(C_GRAY, C_BLACK);
  tft.setCursor(2, tft.height() - 12);
  tft.print(foot);
}
