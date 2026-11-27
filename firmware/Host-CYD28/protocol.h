/**
 * @file      protocol.h
 * @brief     โครงสร้างแพ็กเก็ต ESP-NOW Protocol v3 และรหัสเตือน
 * @version   2.0.4-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * struct_message (Station ส่งหา Host) และ struct_host_sync (Host ตอบ Station) ต้องเหมือนทุกไบต์กับ Station ทุกสาย ตรวจด้วย bash tools/protocol-test/run.sh ห้ามเปลี่ยนลำดับหรือชนิดฟิลด์ รหัสเตือน 6-7 ใช้ภายใน Host เท่านั้น ไม่ส่งออก
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.4-CYD | 2026-11-27 | เพิ่ม MSG_FLAG_NO_SIGNAL_OK (บิต 0x02 ของ struct_message.flags: Station ประกาศว่ารู้จักรหัส 6) และ alertCodeForBed(): ส่งรหัส NO DROP ให้เฉพาะ Station ที่ประกาศ ส่วนรุ่นเก่าได้ 0 เหมือนเดิม |
 * | 2.0.1-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 2 (แกนเครือข่าย) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <stdint.h>

typedef struct __attribute__((packed)) struct_message {
  uint8_t  stationId;
  uint8_t  isRunning;
  uint32_t totalDrops;
  uint32_t periodDrops;
  float    flowRateHr;       // อัตราไหลที่ Station คำนวณจากช่วงห่างระหว่างหยด (mL/h)
  uint32_t msSinceLastDrop;  // มิลลิวินาทีนับจากหยดล่าสุด (0 = ยังไม่มีหยด)
  float    batteryVolts;     // 0 = ไม่มีวงจรวัดแบตเตอรี่
  uint8_t  flags;            // bit0 = พยาบาลกดรับทราบเตือนใกล้หมดที่เตียง
                             // bit1 = Station รู้จักรหัสเตือน 6 (NO DROP)
} struct_message;            // 23 bytes

typedef struct __attribute__((packed)) struct_host_sync {
  uint32_t epochTime;
  uint8_t  isSynced;
  uint8_t  stationId;
  float    targetRateHr;
  float    totalPlanMl;
  uint8_t  alertCode;
  uint8_t  dropFactor;
  uint8_t  resetSeq;         // เปลี่ยนค่าเมื่อกด "เริ่มถุงใหม่" -> Station รีเซ็ตตัวนับ
  uint8_t  hostChannel;
  uint8_t  nearEndPct;       // % ของปริมาตรตามแผนที่เริ่มเตือนใกล้หมด
  uint8_t  flags;            // bit0 = รับทราบเตือนใกล้หมดแล้ว
                             // bit1 = Host ได้ยินเตียงนี้อยู่
                             // bit2 = Host รุ่นนี้รายงาน bit1 รุ่นเก่าเป็น 0 เสมอ
} struct_host_sync;          // 20 bytes

static_assert(sizeof(struct_message) == 23, "struct_message must be 23 bytes (match Station)");
static_assert(sizeof(struct_host_sync) == 20, "struct_host_sync must be 20 bytes (match Station)");

enum AlertCode : uint8_t {
  ALERT_NONE      = 0,
  ALERT_TOO_FAST  = 1,
  ALERT_TOO_SLOW  = 2,
  ALERT_NEAR_END  = 3,   // ให้ไปแล้วถึง % ที่ตั้ง (เตือนเบา ไม่ใช่เหตุวิกฤต)
  ALERT_COMPLETE  = 4,   // ให้ครบตามแผนแล้ว
  ALERT_OCCLUSION = 5,   // สายพับ / หยุดไหล
  ALERT_NO_SIGNAL = 6,   // ภายใน Host: ติดต่อได้แต่เซนเซอร์ไม่เคยจับหยด
  ALERT_LOST_LINK = 7    // ภายใน Host: เตียงที่กำลังให้น้ำเกลือหายจากอากาศ
};

constexpr uint8_t SYNC_FLAG_NEAR_END_ACK = 0x01;
constexpr uint8_t SYNC_FLAG_HEARD        = 0x02;
constexpr uint8_t SYNC_FLAG_REPORTS      = 0x04;
constexpr uint8_t MSG_FLAG_NEAR_END_ACK  = 0x01;
constexpr uint8_t MSG_FLAG_NO_SIGNAL_OK  = 0x02;   // [2.0.4-CYD] เพิ่ม: Station รุ่นที่รู้จักรหัส 6 (NO DROP) ใส่บิตนี้ทุกแพ็กเก็ต

// Station รุ่นเก่าไม่รู้จักรหัส 6-7 จึงต้องแปลงเป็น NONE ก่อนส่งออกเสมอ
inline uint8_t alertCodeForStation(uint8_t code) {
  return (code == ALERT_NO_SIGNAL || code == ALERT_LOST_LINK) ? (uint8_t)ALERT_NONE : code;
}

// [2.0.4-CYD] เพิ่ม: รหัสที่จะส่งให้ Station เฉพาะเตียง NO DROP (6) ส่งได้เมื่อ Station ประกาศรองรับ ส่วน LOST_LINK (7) เป็นรหัสของ Host เสมอ
inline uint8_t alertCodeForBed(uint8_t code, bool stationKnowsNoSignal) {
  if (code == ALERT_NO_SIGNAL && stationKnowsNoSignal) return ALERT_NO_SIGNAL;
  return alertCodeForStation(code);
}

inline bool isCriticalAlert(uint8_t c) {
  return c == ALERT_TOO_FAST || c == ALERT_TOO_SLOW || c == ALERT_COMPLETE || c == ALERT_OCCLUSION ||
         c == ALERT_NO_SIGNAL || c == ALERT_LOST_LINK;
}

constexpr float MAX_PLAUSIBLE_RATE_HR = 3000.0f;   // ชุดให้สารน้ำมาตรฐานไหลได้ไม่ถึงเท่านี้
constexpr float MAX_PLAUSIBLE_BATT_V  = 6.0f;      // แบตลิเธียมเซลล์เดียวไม่ถึง 6 โวลต์

// ปฏิเสธค่าที่เป็นไปไม่ได้ทางกายภาพ (ESP-NOW ไม่มีลายเซ็น อุปกรณ์อื่นขนาดเท่ากันอาจหลุดเข้ามา)
// เขียนกลับด้านเพื่อให้ NaN ถูกปฏิเสธไปในตัว
inline bool packetPlausible(const struct_message &m, int maxBeds) {
  if (m.stationId < 1 || m.stationId > maxBeds) return false;
  if (m.isRunning > 1) return false;
  if (!(m.flowRateHr >= 0.0f && m.flowRateHr <= MAX_PLAUSIBLE_RATE_HR)) return false;
  if (!(m.batteryVolts >= 0.0f && m.batteryVolts <= MAX_PLAUSIBLE_BATT_V)) return false;
  return true;
}
