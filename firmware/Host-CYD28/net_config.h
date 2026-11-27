/**
 * @file      net_config.h
 * @brief     ตั้งค่า Wi-Fi จุดเชื่อมต่อและบัญชีเว็บ: ตรวจค่า เก็บใน NVS และคืนค่าโรงงาน
 * @version   2.1.0-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * ค่าปริยายมาจาก board.h (CYD_AP_SSID ฯลฯ) ผู้ใช้เปลี่ยนผ่านหน้าเว็บได้ ค่าใหม่มีผลหลังรีสตาร์ท ตรวจค่าเข้มงวดเพราะค่าผิดทำให้เข้าเครื่องไม่ได้: ชื่อ Wi-Fi 1-31 ตัวอักษร [A-Za-z0-9 _.-] (ไม่ขึ้นต้น/ลงท้ายด้วยช่องว่าง) รหัส Wi-Fi 8-63 ตัวอักษร ASCII ที่พิมพ์ได้ ชื่อผู้ใช้เว็บ 3-16 ตัว [A-Za-z0-9_.-] รหัสเว็บ 6-32 ตัว ASCII ที่พิมพ์ได้ ลืมรหัส: กด BOOT ค้างตอนเปิดเครื่องหรือพิมพ์ wifireset ทาง Serial เพื่อคืนค่าปริยาย (ต้องอยู่หน้าเครื่องจริง)
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
#include "board.h"

constexpr size_t NET_SSID_MAX = 31, NET_PASS_MIN = 8, NET_PASS_MAX = 63, NET_USER_MIN = 3, NET_USER_MAX = 16, NET_WEBPASS_MIN = 6, NET_WEBPASS_MAX = 32;

struct NetConfig {
  char ssid[NET_SSID_MAX + 1];
  char wifiPass[NET_PASS_MAX + 1];
  char webUser[NET_USER_MAX + 1];
  char webPass[NET_WEBPASS_MAX + 1];
};

struct StoredNet { uint32_t magic; NetConfig cfg; };
static const uint32_t NET_MAGIC = 0x4E455432;      // "NET2"

inline NetConfig netDefaults() {
  NetConfig c = {};
  strncpy(c.ssid, CYD_AP_SSID, NET_SSID_MAX);
  strncpy(c.wifiPass, CYD_AP_PASS, NET_PASS_MAX);
  strncpy(c.webUser, CYD_WEB_USER, NET_USER_MAX);
  strncpy(c.webPass, CYD_WEB_PASS, NET_WEBPASS_MAX);
  return c;
}

inline bool netIsNameChar(char ch) {
  return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '.' || ch == '-';
}
inline bool netIsPrintable(char ch) { return ch >= 0x20 && ch <= 0x7E; }

// กฎของแต่ละฟิลด์ (คืน true ถ้าใช้ได้)
inline bool netValidSsid(const char *s) {
  if (!s) return false;
  const size_t n = strlen(s);
  if (n < 1 || n > NET_SSID_MAX || s[0] == ' ' || s[n - 1] == ' ') return false;
  for (size_t i = 0; i < n; i++) if (!(netIsNameChar(s[i]) || s[i] == ' ')) return false;
  return true;
}
inline bool netValidWifiPass(const char *s) {
  if (!s) return false;
  const size_t n = strlen(s);
  if (n < NET_PASS_MIN || n > NET_PASS_MAX) return false;
  for (size_t i = 0; i < n; i++) if (!netIsPrintable(s[i])) return false;
  return true;
}
inline bool netValidWebUser(const char *s) {
  if (!s) return false;
  const size_t n = strlen(s);
  if (n < NET_USER_MIN || n > NET_USER_MAX) return false;
  for (size_t i = 0; i < n; i++) if (!netIsNameChar(s[i])) return false;
  return true;
}
inline bool netValidWebPass(const char *s) {
  if (!s) return false;
  const size_t n = strlen(s);
  if (n < NET_WEBPASS_MIN || n > NET_WEBPASS_MAX) return false;
  for (size_t i = 0; i < n; i++) if (!netIsPrintable(s[i]) || s[i] == ' ') return false;     // รหัสเว็บห้ามมีช่องว่าง (ส่งผ่าน Basic auth)
  return true;
}

// ตรวจชุดค่าทั้งก้อน (ใช้ตอนโหลดจาก NVS กันข้อมูลเสีย)
inline bool netConfigValid(const NetConfig &c) {
  return netValidSsid(c.ssid) && netValidWifiPass(c.wifiPass) && netValidWebUser(c.webUser) && netValidWebPass(c.webPass);
}

static Preferences netPrefs;

inline NetConfig netLoad() {
  StoredNet s;
  netPrefs.begin("net", true);
  const bool ok = netPrefs.getBytesLength("cfg") == sizeof(s) && netPrefs.getBytes("cfg", &s, sizeof(s)) == sizeof(s);
  netPrefs.end();
  if (ok && s.magic == NET_MAGIC && netConfigValid(s.cfg)) return s.cfg;
  return netDefaults();
}

inline void netSave(const NetConfig &c) {
  StoredNet s = {};
  s.magic = NET_MAGIC;
  s.cfg = c;
  netPrefs.begin("net", false);
  netPrefs.putBytes("cfg", &s, sizeof(s));
  netPrefs.end();
}

inline void netReset() {
  netPrefs.begin("net", false);
  netPrefs.remove("cfg");
  netPrefs.end();
}
