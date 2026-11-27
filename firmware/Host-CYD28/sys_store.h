/**
 * @file      sys_store.h
 * @brief     เก็บค่าระบบ (ความสว่าง เสียง) ใน NVS
 * @version   2.0.2-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * ความสว่างไฟหลังจอและสวิตช์เสียงเตือนของ Host เก็บเป็นก้อนเดียวพร้อมเลขยืนยัน ค่านอกช่วงถูกแทนด้วยค่าปริยาย
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.2-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 3 (หน้าจอและเสียง) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <Preferences.h>

struct SysSettings {
  uint32_t magic;
  uint8_t  brightness;     // 10-100
  uint8_t  buzzerOn;       // 0/1
};
static const uint32_t SYS_MAGIC = 0x53595332;     // "SYS2"
static Preferences sysPrefs;

static SysSettings loadSys() {
  SysSettings s = {SYS_MAGIC, 100, 1};
  SysSettings r;
  sysPrefs.begin("sys", true);
  const bool ok = sysPrefs.getBytesLength("cfg") == sizeof(r) && sysPrefs.getBytes("cfg", &r, sizeof(r)) == sizeof(r);
  sysPrefs.end();
  if (ok && r.magic == SYS_MAGIC) {
    s.brightness = (r.brightness >= 10 && r.brightness <= 100) ? r.brightness : 100;
    s.buzzerOn = r.buzzerOn ? 1 : 0;
  }
  return s;
}

static void saveSys(const SysSettings &s) {
  SysSettings w = s;
  w.magic = SYS_MAGIC;
  sysPrefs.begin("sys", false);
  sysPrefs.putBytes("cfg", &w, sizeof(w));
  sysPrefs.end();
}
