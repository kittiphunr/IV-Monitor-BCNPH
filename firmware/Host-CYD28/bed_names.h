/**
 * @file      bed_names.h
 * @brief     ชื่อ/ป้ายผู้ป่วยต่อเตียง: ตรวจค่า เก็บใน NVS
 * @version   2.1.0-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * ชื่อเก็บได้ถึง 63 ไบต์ UTF-8 (ภาษาไทยได้ประมาณ 21 ตัวอักษร) ตรวจเข้มงวด: ต้องเป็น UTF-8 ที่ถูกต้อง (ไม่รับลำดับไบต์ผิดรูป ตัวเข้ารหัสยาวเกิน เครื่องหมายคู่ใน UTF-16 หรือเกิน U+10FFFF) ไม่มีอักขระควบคุม เพราะชื่อถูกส่งต่อใน JSON และแสดงบนหน้าเว็บ จอ TFT ของ Host ใช้ฟอนต์ ASCII เท่านั้น จึงแสดงชื่อบนจอเฉพาะชื่อที่เป็น ASCII ล้วน (ชื่อไทยแสดงบนหน้าเว็บ) ควรใช้ป้ายที่ไม่ระบุตัวตนเท่าที่จำเป็น เช่น เลขเตียงหรือรหัสผู้ป่วย
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.1.0-CYD | 2026-11-27 | เพิ่มชื่อผู้ป่วยและหน้าตั้งค่า Wi-Fi/รหัสผ่านผ่านเว็บ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <stdint.h>
#include <string.h>
#include <Preferences.h>
#include "bed_model.h"

constexpr size_t BED_NAME_MAX = 63;                       // ไบต์ (ไม่รวมตัวจบสตริง) ภาษาไทยได้ราว 21 ตัวอักษร (ตัวละ 3 ไบต์)

// ตรวจว่าข้อความเป็น UTF-8 ที่ถูกต้องตามมาตรฐาน (RFC 3629) คืนความยาวไบต์ หรือ -1 ถ้าผิด
inline int utf8Validate(const char *s) {
  const unsigned char *p = (const unsigned char *)s;
  int n = 0;
  while (*p) {
    const unsigned char c = *p;
    int extra;
    uint32_t cp;
    if (c < 0x80) { extra = 0; cp = c; }
    else if (c >= 0xC2 && c <= 0xDF) { extra = 1; cp = c & 0x1F; }       // 0xC0 0xC1 เป็นตัวเข้ารหัสยาวเกิน
    else if (c >= 0xE0 && c <= 0xEF) { extra = 2; cp = c & 0x0F; }
    else if (c >= 0xF0 && c <= 0xF4) { extra = 3; cp = c & 0x07; }
    else return -1;
    for (int i = 1; i <= extra; i++) {
      if ((p[i] & 0xC0) != 0x80) return -1;                             // ตัวต่อต้องเป็น 10xxxxxx (ตัวจบสตริงก็ผิดตรงนี้)
      cp = (cp << 6) | (p[i] & 0x3F);
    }
    if (extra == 2 && cp < 0x800) return -1;                            // ยาวเกิน
    if (extra == 3 && (cp < 0x10000 || cp > 0x10FFFF)) return -1;
    if (cp >= 0xD800 && cp <= 0xDFFF) return -1;                        // เครื่องหมายคู่ของ UTF-16 ใช้ใน UTF-8 ไม่ได้
    p += extra + 1;
    n += extra + 1;
  }
  return n;
}

// ตรวจและทำความสะอาดชื่อ: ตัดช่องว่างหัวท้าย ต้อง UTF-8 ถูกต้อง ไม่มีอักขระควบคุม (< 0x20, 0x7F) ยาวไม่เกิน BED_NAME_MAX ไบต์
// ชื่อว่างใช้ได้ (หมายถึงล้างชื่อ) คืน true และเขียนผลลง out (ขนาดอย่างน้อย BED_NAME_MAX + 1)
inline bool nameClean(const char *in, char *out) {
  if (!in) return false;
  while (*in == ' ') in++;
  size_t n = strlen(in);
  while (n > 0 && in[n - 1] == ' ') n--;
  if (n > BED_NAME_MAX) return false;
  char tmp[BED_NAME_MAX + 1];
  memcpy(tmp, in, n);
  tmp[n] = 0;
  for (size_t i = 0; i < n; i++) if ((unsigned char)tmp[i] < 0x20 || (unsigned char)tmp[i] == 0x7F) return false;
  if (utf8Validate(tmp) < 0) return false;
  memcpy(out, tmp, n + 1);
  return true;
}

inline bool nameIsAscii(const char *s) {
  for (; *s; s++) if ((unsigned char)*s >= 0x80) return false;
  return true;
}

struct StoredNames { uint32_t magic; char name[MAX_BEDS][BED_NAME_MAX + 1]; };
static const uint32_t NAMES_MAGIC = 0x4E414D31;     // "NAM1"
static Preferences namePrefs;

inline void saveNames(const Bed *beds) {
  StoredNames s = {};
  s.magic = NAMES_MAGIC;
  for (int i = 0; i < MAX_BEDS; i++) strncpy(s.name[i], beds[i].name, BED_NAME_MAX);
  namePrefs.begin("names", false);
  namePrefs.putBytes("all", &s, sizeof(s));
  namePrefs.end();
}

// โหลดชื่อ ค่าเสียถูกล้างเป็นชื่อว่างรายเตียง (ไม่ทิ้งทั้งชุด)
inline void loadNames(Bed *beds) {
  StoredNames s;
  namePrefs.begin("names", true);
  const bool ok = namePrefs.getBytesLength("all") == sizeof(s) && namePrefs.getBytes("all", &s, sizeof(s)) == sizeof(s);
  namePrefs.end();
  if (!ok || s.magic != NAMES_MAGIC) return;
  for (int i = 0; i < MAX_BEDS; i++) {
    s.name[i][BED_NAME_MAX] = 0;
    char clean[BED_NAME_MAX + 1];
    if (nameClean(s.name[i], clean)) strcpy(beds[i].name, clean); else beds[i].name[0] = 0;
  }
}
