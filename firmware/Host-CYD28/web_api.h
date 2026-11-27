/**
 * @file      web_api.h
 * @brief     ตรรกะล้วนของ API เว็บ: สร้าง JSON และตรวจค่าที่รับเข้ามา
 * @version   2.1.0-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * แยกจากตัวเซิร์ฟเวอร์เพื่อทดสอบบน PC ได้ทุกกรณี ค่าที่รับจากเบราว์เซอร์ทุกตัวถูกตรวจเข้มงวด (ตัวเลขต้องเป็นตัวเลขล้วน ไม่มีขยะต่อท้าย อยู่ในช่วงที่ปลอดภัย) และใช้ทั้งชุดหรือไม่ใช้เลย ไม่มีการแก้ค่าครึ่งๆ กลางๆ เมื่อมีฟิลด์ใดผิด JSON สร้างด้วยบัฟเฟอร์ขนาดคงที่และตรวจล้น ไม่ใช้ String เพื่อไม่ให้หน่วยความจำแตกกระจาย
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.1.0-CYD | 2026-11-27 | state JSON มี name ต่อเตียง · apiParseName · apiParseWifiUpdate (ตรวจทั้งชุดหรือไม่ใช้เลย) · apiWifiJson (ไม่เปิดเผยรหัสผ่าน และบอกว่ายังใช้รหัสปริยายอยู่หรือไม่) |
 * | 2.0.4-CYD | 2026-11-27 | state JSON เพิ่มฟิลด์ noDropSync (Station เตียงนั้นรู้จัก NO DROP หรือไม่) |
 * | 2.0.3-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 4 (เว็บ นาฬิกา SD ประวัติเหตุการณ์) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bed_model.h"
#include "status_screen.h"
#include "event_log.h"
#include "clock_sync.h"
#include "bed_names.h"
#include "net_config.h"

enum ApiErr : uint8_t { API_OK = 0, API_BAD_BED, API_BAD_VALUE, API_NO_FIELDS, API_BAD_TIME, API_NEED_CONFIRM, API_OVERFLOW };

inline const char *apiErrText(ApiErr e) {
  switch (e) {
    case API_OK:           return "ok";
    case API_BAD_BED:      return "bad bed";
    case API_BAD_VALUE:    return "bad value";
    case API_NO_FIELDS:    return "no fields";
    case API_BAD_TIME:     return "bad time";
    case API_NEED_CONFIRM: return "confirm required";
    default:               return "overflow";
  }
}

// ---- JSON ที่เขียนลงบัฟเฟอร์คงที่ ----------------------------------------------------------
struct JsonOut {
  char *b; size_t cap; size_t len; bool over;
  JsonOut(char *buf, size_t c) : b(buf), cap(c), len(0), over(false) { if (cap) b[0] = 0; }
  void fmt(const char *f, ...) {
    if (over) return;
    va_list ap; va_start(ap, f);
    const int n = vsnprintf(b + len, cap - len, f, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= cap - len) { over = true; return; }
    len += (size_t)n;
  }
  void str(const char *s) {            // สตริงพร้อมหลีกอักขระ
    fmt("\"");
    for (; *s && !over; s++) {
      if (*s == '"' || *s == '\\') fmt("\\%c", *s);
      else if ((unsigned char)*s < 0x20) fmt("\\u%04x", (unsigned)*s);
      else fmt("%c", *s);
    }
    fmt("\"");
  }
};

struct ApiCtx {
  const Bed *beds;
  int active;
  uint32_t now;          // millis()
  uint32_t epoch;        // 0 = ยังไม่ได้ตั้งนาฬิกา
  uint8_t  channel;
  uint32_t snoozeLeftS;
  bool     sdOk;
  uint32_t rxDropped, rxBad, rxWrongLen;
  const char *version;
};

inline float apiNum(float v) { return (v == v && fabsf(v) < 1e9f) ? v : 0.0f; }   // กัน NaN/ล้นออกเป็น JSON ที่ไม่ถูกต้อง

inline ApiErr apiStateJson(char *out, size_t cap, const ApiCtx &c) {
  JsonOut j(out, cap);
  j.fmt("{\"version\":"); j.str(c.version);
  j.fmt(",\"uptime\":%lu,\"epoch\":%lu,\"synced\":%s,\"channel\":%u,\"active\":%d,\"snooze\":%lu,\"sd\":%s,",
        (unsigned long)(c.now / 1000), (unsigned long)c.epoch, c.epoch ? "true" : "false", (unsigned)c.channel, c.active,
        (unsigned long)c.snoozeLeftS, c.sdOk ? "true" : "false");
  j.fmt("\"rx\":{\"dropped\":%lu,\"bad\":%lu,\"wrongLen\":%lu},\"beds\":[", (unsigned long)c.rxDropped, (unsigned long)c.rxBad, (unsigned long)c.rxWrongLen);
  for (int i = 0; i < c.active; i++) {
    const Bed &b = c.beds[i];
    j.fmt("%s{\"id\":%d,\"name\":", i ? "," : "", i + 1);
    j.str(b.name);
    j.fmt(",\"state\":");
    j.str(bedStateText(b, c.now));
    j.fmt(",\"alert\":%u,\"online\":%s,\"running\":%s,\"rate\":%.1f,\"target\":%.1f,\"plan\":%.1f,\"vol\":%.1f,\"drops\":%lu,"
          "\"df\":%u,\"near\":%u,\"left\":%d,\"link\":%u,\"rssi\":%d,\"batt\":%.2f,\"conflict\":%s,\"ack\":%s,\"resetSeq\":%u,\"noDropSync\":%s}",
          (unsigned)b.alert, bedOnline(b, c.now) ? "true" : "false", b.running ? "true" : "false", (double)apiNum(b.flowRateHr),
          (double)apiNum(b.cfg.targetRateHr), (double)apiNum(b.cfg.planVolumeMl), (double)apiNum(bedVolumeMl(b)),
          (unsigned long)b.totalDrops, (unsigned)safeDropFactor(b.cfg.dropFactor), (unsigned)safeNearPct(b.cfg.nearEndPct),
          bedRemainingMin(b), (unsigned)b.linkPct, (int)b.rssi, (double)apiNum(b.batteryV), b.idConflict ? "true" : "false",
          b.nearEndAck ? "true" : "false", (unsigned)b.cfg.resetSeq, b.supportsNoSignal ? "true" : "false");
  }
  j.fmt("]}");
  return j.over ? API_OVERFLOW : API_OK;
}

inline ApiErr apiEventsJson(char *out, size_t cap, const EventLog &log) {
  JsonOut j(out, cap);
  j.fmt("[");
  for (int i = 0; i < log.size(); i++) {
    const EventRec &e = log.newest(i);
    j.fmt("%s{\"up\":%lu,\"epoch\":%lu,\"bed\":%u,\"event\":", i ? "," : "", (unsigned long)(e.upMs / 1000), (unsigned long)e.epoch, (unsigned)e.bed);
    j.str(eventTypeName(e.type));
    j.fmt(",\"detail\":");
    j.str((e.type == EV_ALERT || e.type == EV_CLEAR) ? alertName(e.code) : "");
    j.fmt("}");
  }
  j.fmt("]");
  return j.over ? API_OVERFLOW : API_OK;
}

// ---- ตรวจค่าที่รับเข้า --------------------------------------------------------------------
// ตัวเลขต้องเป็นเลขฐานสิบล้วน: เครื่องหมาย +/- นำหน้าได้ จุดทศนิยมได้หนึ่งจุด ไม่รับเลขยกกำลัง (1e5) เลขฐานสิบหก (0x10) inf nan
// ไม่ว่าง ไม่มีขยะต่อท้าย (ยอมให้มีช่องว่างหน้า/ท้าย) strtod เปล่าๆ รับ 0x10 และ 1e5 ซึ่งไม่ควรผ่านจากช่องกรอกค่าของพยาบาล
inline bool apiParseNumber(const char *s, double &v) {
  if (!s) return false;
  while (*s == ' ') s++;
  const char *p = s;
  if (*p == '+' || *p == '-') p++;
  int digits = 0, dots = 0;
  const char *q = p;
  for (; *q && *q != ' '; q++) {
    if (*q >= '0' && *q <= '9') digits++;
    else if (*q == '.' && ++dots <= 1) continue;
    else return false;
  }
  const char *rest = q;
  while (*rest == ' ') rest++;
  if (*rest || digits == 0) return false;
  v = strtod(s, nullptr);
  return v == v && fabs(v) < 1e12;
}

inline bool apiHas(const char *s) { return s && *s; }

// แก้ค่าตั้งของเตียงจากข้อความที่รับมา: ฟิลด์ที่ว่างจะข้าม ใช้ทั้งชุดหรือไม่ใช้เลย คืน API_NO_FIELDS ถ้าไม่มีฟิลด์ใดเลย
inline ApiErr apiParseBedUpdate(const char *rate, const char *plan, const char *df, const char *nearPct, BedConfig &cfg) {
  BedConfig n = cfg;
  double v;
  bool any = false;
  if (apiHas(rate)) {
    if (!apiParseNumber(rate, v) || v < 0 || v > MAX_PLAUSIBLE_RATE_HR) return API_BAD_VALUE;
    n.targetRateHr = (float)v; any = true;
  }
  if (apiHas(plan)) {
    if (!apiParseNumber(plan, v) || v < 0 || v > 10000) return API_BAD_VALUE;
    n.planVolumeMl = (float)v; any = true;
  }
  if (apiHas(df)) {
    if (!apiParseNumber(df, v) || (v != 10 && v != 15 && v != 20 && v != 60)) return API_BAD_VALUE;
    n.dropFactor = (uint8_t)v; any = true;
  }
  if (apiHas(nearPct)) {
    if (!apiParseNumber(nearPct, v) || v < 50 || v > 95 || v != floor(v)) return API_BAD_VALUE;
    n.nearEndPct = (uint8_t)v; any = true;
  }
  if (!any) return API_NO_FIELDS;
  cfg = n;
  return API_OK;
}

// ชื่อผู้ป่วย: ว่างได้ (ล้างชื่อ) ต้อง UTF-8 ถูกต้อง ไม่มีอักขระควบคุม ไม่เกิน 31 ไบต์
inline ApiErr apiParseName(const char *s, char *out) { return nameClean(s, out) ? API_OK : API_BAD_VALUE; }

// แก้ค่า Wi-Fi/บัญชีเว็บ: ฟิลด์ที่ว่างจะข้าม ตรวจทั้งชุดก่อนใช้ ผิดฟิลด์เดียวไม่แก้อะไรเลย ไม่มีฟิลด์เลย = API_NO_FIELDS
inline ApiErr apiParseWifiUpdate(const char *ssid, const char *pass, const char *webUser, const char *webPass, NetConfig &cfg) {
  NetConfig n = cfg;
  bool any = false;
  if (apiHas(ssid))    { if (!netValidSsid(ssid))       return API_BAD_VALUE; strcpy(n.ssid, ssid); any = true; }
  if (apiHas(pass))    { if (!netValidWifiPass(pass))   return API_BAD_VALUE; strcpy(n.wifiPass, pass); any = true; }
  if (apiHas(webUser)) { if (!netValidWebUser(webUser)) return API_BAD_VALUE; strcpy(n.webUser, webUser); any = true; }
  if (apiHas(webPass)) { if (!netValidWebPass(webPass)) return API_BAD_VALUE; strcpy(n.webPass, webPass); any = true; }
  if (!any) return API_NO_FIELDS;
  cfg = n;
  return API_OK;
}

// ข้อมูล Wi-Fi สำหรับหน้าเว็บ: ไม่ส่งรหัสผ่านออกไปเด็ดขาด บอกแค่ว่ายังใช้รหัสปริยายอยู่หรือไม่
inline ApiErr apiWifiJson(char *out, size_t cap, const NetConfig &c, const char *ip, int clients) {
  const NetConfig d = netDefaults();
  const bool defWifi = strcmp(c.wifiPass, d.wifiPass) == 0, defWeb = strcmp(c.webPass, d.webPass) == 0;
  JsonOut j(out, cap);
  j.fmt("{\"ssid\":"); j.str(c.ssid);
  j.fmt(",\"webUser\":"); j.str(c.webUser);
  j.fmt(",\"ip\":"); j.str(ip);
  j.fmt(",\"clients\":%d,\"defaultWifiPass\":%s,\"defaultWebPass\":%s}", clients, defWifi ? "true" : "false", defWeb ? "true" : "false");
  return j.over ? API_OVERFLOW : API_OK;
}

inline ApiErr apiParseBed(const char *s, int active, int &idx) {
  double v;
  if (!apiParseNumber(s, v) || v != floor(v) || v < 1 || v > active) return API_BAD_BED;
  idx = (int)v - 1;
  return API_OK;
}

inline ApiErr apiParseEpoch(const char *s, uint32_t &epoch) {
  double v;
  if (!apiParseNumber(s, v) || v != floor(v) || v < (double)EPOCH_MIN || v > (double)EPOCH_MAX) return API_BAD_TIME;
  epoch = (uint32_t)v;
  return API_OK;
}
