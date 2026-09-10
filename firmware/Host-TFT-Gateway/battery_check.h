/**
 * @file      battery_check.h
 * @brief     ตัวตรวจระดับแบตเตอรี่ที่ใช้ร่วมกันสองฝั่ง (เครื่องเตียงตรวจตัวเอง Host ตรวจทั้งตัวเองและทุกเตียง)
 * @version   1.1.0-BAT
 * @date      2026-11-20
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * แปลงแรงดันแบตเตอรี่ลิเธียมเซลล์เดียวเป็นระดับ 4 ขั้น: ไม่มีวงจรวัด / ปกติ / ต่ำ / วิกฤต
 * เกณฑ์ ต่ำ = 3.50 V (ราว 30%) วิกฤต = 3.35 V (ราว 15%) ตามสมการเดียวกับ calculateBatteryPct() ของ Host
 *
 * ระดับแบตเตอรี่เป็น "เรื่องบำรุงรักษา" ไม่ใช่เหตุทางคลินิก จึงไม่ปนกับรหัสเตือน ALERT_* และไม่ถูกนับเป็น
 * เหตุวิกฤตที่ต้องพักเสียง ผู้ใช้เห็นผ่านไอคอนและข้อความ และได้ยินเสียงเตือนเบา ๆ ที่เว้นช่วงนาน
 *
 * กันการเตือนเพี้ยนด้วยสามชั้น
 *   1) ต้องอยู่ในระดับใหม่ต่อเนื่อง BAT_HOLD_MS (30 วินาที) จึงเปลี่ยนระดับ แรงดันตกชั่วขณะตอนบัซเซอร์ดัง
 *      หรือจอกระพริบสว่างไม่ทำให้เตือนผิด (ยกเว้นการอ่านครั้งแรกหลังเปิดเครื่อง ตัดสินทันที)
 *   2) ฮิสเทอรีซิส BAT_HYST_MV (50 mV) ขาขึ้น แรงดันที่แกว่งรอบเส้นแบ่งไม่ทำให้ระดับสลับไปมา
 *   3) แรงดัน 0 หรือต่ำกว่า 2.5 V คือ "ไม่มีวงจรวัด / ใช้ไฟ USB" ไม่ใช่แบตหมด จึงไม่เตือน
 *
 * ไฟล์นี้ไม่พึ่งไลบรารีใด ทดสอบบนเครื่อง PC ได้ตรง ๆ ด้วย `bash tools/v4-test/run.sh`
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 1.1.0-BAT | 2026-11-20 | รุ่นแรก |
 *
 * @warning  ไฟล์นี้ต้องเหมือนกันทุกไบต์ในโฟลเดอร์ Station-Thin และ Host-TFT-Gateway
 *           เกณฑ์สองฝั่งจึงตรงกันเสมอ ตรวจด้วย `bash tools/gateway-sync/run.sh`
 *
 * @warning  ห้ามเรียก millis() ในไฟล์นี้ ต้องส่งเวลาเข้ามาทุกครั้ง เพื่อให้ทดสอบด้วยนาฬิกาจำลองได้
 */
#pragma once
#include <stdint.h>

#define BAT_LOW_MV      3500      // ต่ำกว่านี้ = ต่ำ (ราว 30%)
#define BAT_CRIT_MV     3350      // ต่ำกว่านี้ = วิกฤต (ราว 15%)
#define BAT_HYST_MV     50        // ขาขึ้นต้องเกินเส้นแบ่งเท่านี้จึงยกระดับกลับ
#define BAT_HOLD_MS     30000UL   // ต้องอยู่ระดับใหม่ต่อเนื่องเท่านี้จึงเปลี่ยน
#define BAT_NO_CIRCUIT_MV 2500    // ต่ำกว่านี้ถือว่าไม่มีวงจรวัด

enum BatLevel : uint8_t { BAT_NONE = 0, BAT_OK = 1, BAT_LOW = 2, BAT_CRIT = 3 };

// เปอร์เซ็นต์ตามสมการของ Host (3.2 V = 0%, 4.2 V = 100%) ไว้แสดงผล ไม่ใช้ตัดสินระดับ
static inline int batteryPctFromMv(uint16_t mv) {
  int pct = ((int)mv - 3200) / 10;
  if (pct > 100) pct = 100;
  if (pct < 0) pct = 0;
  return pct;
}

static inline const char* batLevelWord(uint8_t level) {
  switch (level) {
    case BAT_LOW:  return "LOW BATT";
    case BAT_CRIT: return "BATT CRIT";
    default:       return "";
  }
}

struct BatteryMonitor {
  uint8_t  level      = BAT_NONE;   // ระดับที่ประกาศอยู่ตอนนี้
  uint8_t  candidate  = BAT_NONE;   // ระดับที่การอ่านล่าสุดชี้ไปทาง
  uint32_t candSince  = 0;          // เวลาที่เริ่มชี้ไปทางระดับนี้ต่อเนื่อง
  uint16_t lastMv     = 0;

  // ระดับที่แรงดันนี้ชี้ไป โดยคำนึงถึงฮิสเทอรีซิสจากระดับปัจจุบัน
  uint8_t classify(uint16_t mv) const {
    uint16_t lowUp  = BAT_LOW_MV + BAT_HYST_MV;
    uint16_t critUp = BAT_CRIT_MV + BAT_HYST_MV;
    switch (level) {
      case BAT_CRIT:   // ออกจากวิกฤตต้องเกินเส้น + ฮิสเทอรีซิส
        if (mv < critUp) return BAT_CRIT;
        return (mv < lowUp) ? BAT_LOW : BAT_OK;
      case BAT_LOW:    // ออกจากต่ำต้องเกินเส้น + ฮิสเทอรีซิส
        if (mv < BAT_CRIT_MV) return BAT_CRIT;
        return (mv < lowUp) ? BAT_LOW : BAT_OK;
      default:         // ปกติ/ยังไม่รู้: ลงเมื่อต่ำกว่าเส้น
        if (mv < BAT_CRIT_MV) return BAT_CRIT;
        return (mv < BAT_LOW_MV) ? BAT_LOW : BAT_OK;
    }
  }

  // เรียกทุกครั้งที่ได้ค่าใหม่ (mv = 0 คือไม่มีวงจรวัด) คืนระดับที่ประกาศ
  uint8_t update(uint32_t nowMs, uint16_t mv) {
    lastMv = mv;
    if (mv < BAT_NO_CIRCUIT_MV) {            // ไม่มีวงจรวัด / ใช้ไฟ USB: ล้างทุกอย่าง ไม่เตือน
      level = candidate = BAT_NONE;
      return level;
    }
    uint8_t c = classify(mv);
    if (level == BAT_NONE) {                 // อ่านครั้งแรกที่เชื่อถือได้: ตัดสินทันที ไม่ต้องรอ
      level = candidate = c;
      candSince = nowMs;
      return level;
    }
    if (c == level) {                        // ยังอยู่ระดับเดิม ล้างตัวนับการเปลี่ยน
      candidate = level;
      candSince = nowMs;
      return level;
    }
    if (c != candidate) {                    // เพิ่งชี้ไปทางระดับใหม่ เริ่มนับเวลา
      candidate = c;
      candSince = nowMs;
      return level;
    }
    if ((uint32_t)(nowMs - candSince) >= BAT_HOLD_MS) {   // ชี้ทางเดิมครบเวลา จึงเปลี่ยนจริง
      level = c;
      candSince = nowMs;
    }
    return level;
  }

  bool isLow() const { return level == BAT_LOW || level == BAT_CRIT; }
};
