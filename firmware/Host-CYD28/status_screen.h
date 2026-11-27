/**
 * @file      status_screen.h
 * @brief     ชื่อและสีสถานะเตียง (ใช้ร่วมกับตาราง Serial และหน้าจอ)
 * @version   2.0.2-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * แปลงสถานะเตียงเป็นข้อความและสีเดียวกันทั้งหน้าจอ (ui.h) และตาราง Serial
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.2-CYD | 2026-11-27 | ตัดตารางตัวอักษรชั่วคราวออก เหลือชื่อและสีสถานะ หน้าจอจริงอยู่ที่ ui.h |
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
