/**
 * @file      bed_store.h
 * @brief     เก็บค่าตั้งของเตียงใน NVS
 * @version   2.0.1-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * เก็บเป้าหมายอัตราไหล ปริมาตรตามแผน drop factor เกณฑ์ใกล้หมด จำนวนเตียงที่เปิดใช้ และ resetSeq เป็นก้อนเดียวพร้อมเลขยืนยัน resetSeq ต้องเก็บถาวร ไม่เช่นนั้นเมื่อ Host รีบูต Station จะเห็นเลขเปลี่ยนแล้วล้างตัวนับเอง
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.1-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 2 (แกนเครือข่าย) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <Preferences.h>
#include "bed_model.h"

static const uint32_t BEDS_MAGIC = 0x42454432;      // "BED2"
constexpr uint8_t DEFAULT_ACTIVE_BEDS = 5;

struct StoredBeds {
  uint32_t  magic;
  uint8_t   count;
  BedConfig cfg[MAX_BEDS];
};

static Preferences bedPrefs;

static void saveBeds(const Bed *beds, uint8_t count) {
  StoredBeds s = {};
  s.magic = BEDS_MAGIC;
  s.count = count;
  for (int i = 0; i < MAX_BEDS; i++) s.cfg[i] = beds[i].cfg;
  bedPrefs.begin("beds", false);
  bedPrefs.putBytes("all", &s, sizeof(s));
  bedPrefs.end();
}

// คืนจำนวนเตียงที่เปิดใช้ ค่าที่อ่านมาถูกตรวจช่วงทุกตัว ค่าเสียถูกแทนด้วยค่าปริยาย
static uint8_t loadBeds(Bed *beds) {
  StoredBeds s;
  bedPrefs.begin("beds", true);
  const bool ok = bedPrefs.getBytesLength("all") == sizeof(s) && bedPrefs.getBytes("all", &s, sizeof(s)) == sizeof(s);
  bedPrefs.end();
  if (!ok || s.magic != BEDS_MAGIC) return DEFAULT_ACTIVE_BEDS;
  for (int i = 0; i < MAX_BEDS; i++) {
    BedConfig c = s.cfg[i];
    if (!(c.targetRateHr >= 0.0f && c.targetRateHr <= MAX_PLAUSIBLE_RATE_HR)) c.targetRateHr = 0;
    if (!(c.planVolumeMl >= 0.0f && c.planVolumeMl <= 10000.0f)) c.planVolumeMl = 0;
    c.dropFactor = safeDropFactor(c.dropFactor);
    c.nearEndPct = safeNearPct(c.nearEndPct);
    beds[i].cfg = c;
  }
  return (s.count >= 1 && s.count <= MAX_BEDS) ? s.count : DEFAULT_ACTIVE_BEDS;
}
