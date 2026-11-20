/**
 * @file      protocol_v4.h
 * @brief     โครงสร้างแพ็กเก็ต ESP-NOW Protocol v4 (Station แบบบาง) ใช้ร่วมกันสองสเก็ตช์
 * @version   1.0.0-V4
 * @date      2026-11-19
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * Protocol v4 แบ่งงานใหม่ระหว่าง Station กับ Host
 *   Station (ขึ้น) ส่งแค่ข้อเท็จจริงจากเซนเซอร์ คือจำนวนหยดสะสมกับเวลาที่ผ่านมาจากหยดล่าสุด
 *   Host    (ลง)   ส่งผลคำนวณทั้งหมดกลับไปให้ Station แสดง: อัตราไหล ปริมาตร % เวลาที่เหลือ รหัสเตือน
 * Station จึงไม่มีสูตรทางคลินิกเลย และผลคำนวณอยู่ที่ Host จุดเดียว
 *
 * ทำไมต้องส่งเวลาจากหยดล่าสุดด้วย (ไม่ใช่แค่จำนวนหยด)
 * Station ส่งวินาทีละใบ ถ้า Host เห็นแค่จำนวนหยดต่อวินาที อัตราจะหยาบและแกว่ง (ที่ 3 mL/h คือหนึ่งหยดต่อนาที
 * จึงเห็นเป็น 0 บ้าง 1 บ้าง) รุ่นก่อน 7.7.3 เคยเป็นแบบนี้และเตือนผิดบ่อย เวลาจากหยดล่าสุดทำให้ Host รู้
 * "เวลาที่แน่นอนของหยดหนึ่ง" ทุกใบที่ได้รับ ผลต่างจำนวนหยดระหว่างสองหยดที่รู้เวลาแน่นอนจึงแม่นเท่ากับนับ
 * หยดเองที่ Station (ดู rate_estimator.h)
 *
 * แพ็กเก็ตทั้งสองทิศมีไบต์แรกเป็นรหัสประจำชนิด และขนาดต่างจาก Protocol v3 (23 และ 20 ไบต์)
 * อุปกรณ์สองรุ่นที่อยู่ในช่องเดียวกันจึงกรองแพ็กเก็ตของกันออกด้วยความยาวและรหัส ไม่ตีความผิด
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 1.0.0-V4 | 2026-11-19 | รุ่นแรก |
 *
 * @warning  ไฟล์นี้ต้องเหมือนกันทุกไบต์ในโฟลเดอร์ Station-Thin และ Host-TFT-Gateway
 *           ตรวจด้วย `bash tools/v4-test/run.sh`
 *
 * @note     ลำดับไบต์เป็น little-endian แบบเดียวกับชิป ESP32 ทั้งสองฝั่ง ไม่มีการแปลง
 */
#pragma once
#include <stdint.h>

#define V4_MAGIC_UP    0xA4   // Station -> Host
#define V4_MAGIC_DOWN  0xA5   // Host -> Station

// ธงของแพ็กเก็ตขึ้น (Station -> Host)
#define V4_UP_RUNNING      0x01   // กำลังนับ (ไม่ได้หยุดชั่วคราว)
#define V4_UP_NEAR_ACK     0x02   // พยาบาลกดรับทราบเตือนใกล้หมดที่เตียง
#define V4_UP_SENSOR_SEEN  0x04   // เซนเซอร์เคยจับหยดได้อย่างน้อยหนึ่งหยดนับจากเปิดเครื่อง
#define V4_UP_SENSOR_FAULT 0x08   // อัลกอริทึมตรวจจับรายงานว่าเซนเซอร์ยังจับหยดไม่ได้ (CHECK SENSOR)

// ธงของแพ็กเก็ตลง (Host -> Station)
#define V4_DOWN_NEAR_ACK   0x01   // Host ยืนยันว่ารับทราบเตือนใกล้หมดแล้ว
#define V4_DOWN_HEARS_YOU  0x02   // Host ได้ยินเตียงนี้อยู่ (ลิงก์ขาขึ้นปกติ)
#define V4_DOWN_TIME_OK    0x04   // epochTime เป็นเวลาจริง
#define V4_DOWN_PLAN_SET   0x08   // Host มีปริมาตรตามแผนของเตียงนี้ (infusedPct ใช้ได้)
#define V4_DOWN_RATE_OK    0x10   // rateHr เป็นค่าที่คำนวณได้จริง (ไม่ใช่ 0 เพราะยังไม่มีข้อมูล)

#define V4_MINUTES_UNKNOWN 0xFFFF // minutesLeft: คำนวณไม่ได้

typedef struct __attribute__((packed)) struct_up_v4 {
  uint8_t  magic;             // V4_MAGIC_UP
  uint8_t  stationId;
  uint8_t  flags;             // V4_UP_*
  uint8_t  seq;               // เพิ่มทีละ 1 ทุกใบ ไว้วัดการสูญหายของแพ็กเก็ต
  uint32_t totalDrops;        // จำนวนหยดสะสมตั้งแต่เริ่มถุง (เก็บลง NVS กู้ได้หลังรีบูต)
  uint32_t msSinceLastDrop;   // มิลลิวินาทีนับจากหยดล่าสุด ณ ตอนส่ง (0 = ยังไม่มีหยด)
  uint16_t batteryMv;         // 0 = ไม่มีวงจรวัดแบตเตอรี่
} struct_up_v4;               // 14 bytes

typedef struct __attribute__((packed)) struct_down_v4 {
  uint8_t  magic;             // V4_MAGIC_DOWN
  uint8_t  stationId;
  uint8_t  flags;             // V4_DOWN_*
  uint8_t  alertCode;         // รหัสเตือนที่ Station รู้จัก (0-5) รหัสภายใน Host ถูกแปลงเป็น 0 แล้ว
  uint8_t  resetSeq;          // เปลี่ยนค่าเมื่อกด "เริ่มถุงใหม่" -> Station ล้างยอดหยด
  uint8_t  dropFactor;
  uint8_t  nearEndPct;
  uint8_t  hostChannel;
  uint32_t epochTime;         // UTC (0 = Host ยังไม่รู้เวลา)
  float    rateHr;            // อัตราไหลที่ Host คำนวณ (mL/h)
  float    targetRateHr;
  float    planMl;
  float    infusedMl;         // ปริมาตรที่ให้ไปแล้ว คำนวณจากหยดสะสมกับ dropFactor
  uint16_t minutesLeft;       // V4_MINUTES_UNKNOWN = คำนวณไม่ได้
  uint8_t  infusedPct;        // 0-100 (ใช้ได้เมื่อมีธง PLAN_SET)
  uint8_t  spare;
} struct_down_v4;             // 32 bytes

static_assert(sizeof(struct_up_v4) == 14, "struct_up_v4 must be 14 bytes");
static_assert(sizeof(struct_down_v4) == 32, "struct_down_v4 must be 32 bytes");
