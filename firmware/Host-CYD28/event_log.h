/**
 * @file      event_log.h
 * @brief     ประวัติเหตุการณ์ในหน่วยความจำ (วงแหวน 48 รายการ) และบรรทัด CSV สำหรับการ์ด SD
 * @version   2.0.3-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * เก็บเหตุที่พยาบาลต้องย้อนดู: เตือนเริ่ม/หาย เตียงออนไลน์/ออฟไลน์ เริ่มถุงใหม่ แก้ค่า รับทราบ พักเสียง รีบูต ตรรกะล้วน ทดสอบบน PC ได้ เวลาจริง (epoch) เป็น 0 ถ้า Host ยังไม่ได้ตั้งนาฬิกา แล้วใช้เวลานับจากเปิดเครื่องแทน
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.3-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 4 (เว็บ นาฬิกา SD ประวัติเหตุการณ์) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include "protocol.h"

enum EvType : uint8_t { EV_BOOT = 0, EV_ALERT, EV_CLEAR, EV_ONLINE, EV_OFFLINE, EV_NEWBAG, EV_CONFIG, EV_ACK, EV_SILENCE };

struct EventRec {
  uint32_t upMs;      // เวลานับจากเปิดเครื่อง
  uint32_t epoch;     // เวลาจริง (0 = ยังไม่ได้ตั้งนาฬิกา)
  uint8_t  bed;       // 1..8 หรือ 0 = ทั้งระบบ
  uint8_t  type;
  uint8_t  code;      // รหัสเตือน (เฉพาะ EV_ALERT / EV_CLEAR)
};

inline const char *alertName(uint8_t code) {
  switch (code) {
    case ALERT_TOO_FAST:  return "TOO_FAST";
    case ALERT_TOO_SLOW:  return "TOO_SLOW";
    case ALERT_NEAR_END:  return "NEAR_END";
    case ALERT_COMPLETE:  return "COMPLETE";
    case ALERT_OCCLUSION: return "OCCLUSION";
    case ALERT_NO_SIGNAL: return "NO_SIGNAL";
    case ALERT_LOST_LINK: return "LOST_LINK";
    default:              return "NONE";
  }
}

inline const char *eventTypeName(uint8_t t) {
  switch (t) {
    case EV_BOOT:    return "BOOT";
    case EV_ALERT:   return "ALERT";
    case EV_CLEAR:   return "CLEAR";
    case EV_ONLINE:  return "ONLINE";
    case EV_OFFLINE: return "OFFLINE";
    case EV_NEWBAG:  return "NEWBAG";
    case EV_CONFIG:  return "CONFIG";
    case EV_ACK:     return "ACK";
    case EV_SILENCE: return "SILENCE";
    default:         return "?";
  }
}

class EventLog {
 public:
  static const int CAP = 48;
  void add(uint32_t upMs, uint32_t epoch, uint8_t bed, uint8_t type, uint8_t code = 0) {
    _r[_head] = { upMs, epoch, bed, type, code };
    _head = (uint8_t)((_head + 1) % CAP);
    if (_n < CAP) _n++;
  }
  int size() const { return _n; }
  // i = 0 คือรายการใหม่สุด
  const EventRec &newest(int i) const { return _r[(_head + CAP - 1 - i) % CAP]; }
 private:
  EventRec _r[CAP];
  uint8_t _head = 0;
  uint8_t _n = 0;
};

// บรรทัด CSV: epoch,upSec,bed,EVENT,detail\n  คืนความยาว (0 ถ้าบัฟเฟอร์เล็กเกิน)
inline int eventCsvLine(char *out, size_t cap, const EventRec &e) {
  const char *detail = (e.type == EV_ALERT || e.type == EV_CLEAR) ? alertName(e.code) : "";
  const int n = snprintf(out, cap, "%lu,%lu,%u,%s,%s\n", (unsigned long)e.epoch, (unsigned long)(e.upMs / 1000), (unsigned)e.bed,
                         eventTypeName(e.type), detail);
  return (n > 0 && (size_t)n < cap) ? n : 0;
}
