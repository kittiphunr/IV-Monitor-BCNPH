/**
 * @file      trend.h
 * @brief     บันทึกรายนาทีต่อเตียง (วงแหวน 240 นาที) สถิติสรุปสำหรับรายงาน และบรรทัด CSV
 * @version   2.2.0-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * ตรรกะล้วน ทดสอบบน PC ได้ ทุก 1 วินาทีเก็บสถิติของนาทีปัจจุบันของแต่ละเตียง (ค่าเฉลี่ยอัตราไหล รหัสเตือนที่สำคัญที่สุด จำนวนหยดที่เพิ่ม) พอครบนาทีจึงบันทึกเป็นตัวอย่างหนึ่งรายการ เตียงที่เงียบทั้งนาทีบันทึกเป็น 'ไม่มีข้อมูล' (ไม่ใช่ศูนย์) เพื่อกราฟและรายงานไม่เอาช่วงที่ไม่รู้ไปคิดเป็นอัตราไหลศูนย์ ปริมาตรคิดจากผลรวมจำนวนหยดที่เพิ่มหารด้วย drop factor ณ ขณะนั้น จึงถูกต้องแม้เริ่มถุงใหม่ (ตัวนับ Station กลับเป็น 0) ระหว่างช่วง เวลาในวงแหวนนับเป็นนาทีนับจากเปิดเครื่อง ไม่ผูกกับนาฬิกาจริง (Host ไม่มีนาฬิกาถ้าไม่ได้ตั้ง) ส่วนไฟล์ CSV บน SD ใส่เวลาจริงให้ถ้าตั้งนาฬิกาแล้ว
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.2.0-CYD | 2026-11-27 | เพิ่มกราฟแนวโน้มอัตราไหลและรายงานย้อนหลังรายนาที |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bed_model.h"
#include "event_log.h"
#include "buzzer.h"            // buzzPriority(): เลือกรหัสเตือนที่สำคัญที่สุดของนาที

#ifndef CYD_TREND_MINUTES
#define CYD_TREND_MINUTES 240                      // ความจุวงแหวนต่อเตียง (4 ชั่วโมง) ใช้ RAM ราว 23 KB ทั้งแปดเตียง ลดค่านี้ถ้าคอมไพล์แล้วหน่วยความจำ DRAM ไม่พอ
#endif
constexpr int TREND_MINUTES = CYD_TREND_MINUTES;
static_assert(TREND_MINUTES >= 30 && TREND_MINUTES <= 1000, "CYD_TREND_MINUTES ต้องอยู่ระหว่าง 30 ถึง 1000");
constexpr uint32_t TREND_PERIOD_MS = 60000;
constexpr uint8_t TS_VALID = 0x01, TS_RUNNING = 0x02;

struct TrendSample {
  uint16_t rate;       // อัตราไหลเฉลี่ยของนาที (mL/h ปัดเป็นจำนวนเต็ม)
  uint16_t volMl;      // ปริมาตรสะสม ณ สิ้นนาที (mL) ตัดที่ 65535
  uint16_t drops;      // จำนวนหยดที่เพิ่มในนาทีนี้
  uint8_t  alert;      // รหัสเตือนที่สำคัญที่สุดในนาทีนั้น (0 = ไม่มี)
  uint8_t  flags;      // TS_VALID / TS_RUNNING
  uint8_t  link;       // คุณภาพลิงก์ %
  int8_t   rssi;
  uint16_t target;     // เป้าหมาย mL/h ณ ตอนนั้น (ตัดที่ 65535) เก็บไว้เพราะผู้ใช้เปลี่ยนเป้าระหว่างถุงได้
};

struct TrendRing {
  TrendSample s[TREND_MINUTES];
  uint16_t head = 0;                // ตำแหน่งที่จะเขียนถัดไป
  uint16_t count = 0;
  uint32_t totalMinutes = 0;        // จำนวนตัวอย่างที่เคยบันทึกทั้งหมด (ใช้ติดตามเลขนาทีของตัวอย่างล่าสุด)
  void add(const TrendSample &x) { s[head] = x; head = (uint16_t)((head + 1) % TREND_MINUTES); if (count < TREND_MINUTES) count++; totalMinutes++; }
  void clear() { head = 0; count = 0; }
  // i = 0 คือตัวอย่างใหม่สุด
  const TrendSample &newest(int i) const { return s[(head + TREND_MINUTES - 1 - i) % TREND_MINUTES]; }
};

// สถิติของนาทีปัจจุบัน
struct TrendAcc {
  uint32_t rateSum = 0;             // ผลรวมอัตราไหลที่เห็นทุกวินาที (mL/h x 1)
  uint16_t n = 0;                   // จำนวนวินาทีที่ออนไลน์
  uint16_t runningN = 0;
  uint8_t  alertMax = 0;
  bool     haveStart = false;
  uint32_t dropsStart = 0;
  uint32_t dropsLast = 0;
  uint32_t dropsAdded = 0;          // ผลรวมหยดที่เพิ่ม (รองรับตัวนับรีเซ็ต)
  float    volAdded = 0;            // ปริมาตรที่เพิ่มในนาทีนี้ (mL) คิดตาม drop factor ขณะนั้น
};

struct TrendState {
  TrendRing ring[MAX_BEDS];
  TrendAcc  acc[MAX_BEDS];
  uint32_t  minuteStartMs = 0;
  bool      started = false;
};

inline uint16_t trendClamp16(float v) { return v <= 0 ? 0 : (v >= 65535.0f ? 65535 : (uint16_t)(v + 0.5f)); }

// เรียกทุกประมาณ 1 วินาที (เรียกถี่กว่านี้ได้ แต่จะนับทุกครั้งที่เรียก) เก็บสถิติของนาทีปัจจุบัน
inline void trendObserve(TrendState &t, const Bed *beds, int active, uint32_t now) {
  if (!t.started) { t.started = true; t.minuteStartMs = now; }
  for (int i = 0; i < active; i++) {
    const Bed &b = beds[i];
    TrendAcc &a = t.acc[i];
    if (!bedOnline(b, now)) continue;
    a.rateSum += (uint32_t)(b.flowRateHr > 0 ? b.flowRateHr + 0.5f : 0);
    a.n++;
    if (b.running) a.runningN++;
    if (buzzPriority(b.alert) > buzzPriority(a.alertMax)) a.alertMax = b.alert;
    else if (a.alertMax == 0 && b.alert == ALERT_NEAR_END) a.alertMax = b.alert;     // ใกล้หมดไม่มีลำดับเสียง แต่ต้องเห็นในกราฟ
    if (!a.haveStart) { a.haveStart = true; a.dropsLast = b.totalDrops; }
    else {
      const uint32_t delta = (b.totalDrops >= a.dropsLast) ? (b.totalDrops - a.dropsLast) : b.totalDrops;   // ตัวนับรีเซ็ต (ถุงใหม่/Station รีบูต)
      a.dropsAdded += delta;
      a.volAdded += (float)delta / (float)safeDropFactor(b.cfg.dropFactor);
      a.dropsLast = b.totalDrops;
    }
  }
}

// ปิดนาทีเมื่อครบ TREND_PERIOD_MS บันทึกตัวอย่างของทุกเตียง คืน true ถ้าปิดนาทีไป (ผู้เรียกใช้บันทึก CSV ต่อ)
// out[i] = ตัวอย่างที่เพิ่งบันทึกของเตียง i (ใช้ตอนเขียน SD) outCount = จำนวนเตียง
inline bool trendClose(TrendState &t, const Bed *beds, int active, uint32_t now, TrendSample *out = nullptr) {
  if (!t.started || (uint32_t)(now - t.minuteStartMs) < TREND_PERIOD_MS) return false;
  t.minuteStartMs += TREND_PERIOD_MS;                              // ไม่เลื่อนตามเวลาปิดจริง กันเวลาเพี้ยนสะสม
  if ((uint32_t)(now - t.minuteStartMs) >= TREND_PERIOD_MS) t.minuteStartMs = now;   // ค้างนานหลายนาที (ลูปช้า): ตัดตอนใหม่แทนไล่ปิดย้อนหลัง
  for (int i = 0; i < active; i++) {
    const Bed &b = beds[i];
    TrendAcc &a = t.acc[i];
    TrendSample x = {};
    x.target = trendClamp16(b.cfg.targetRateHr);
    if (a.n > 0) {
      x.flags = TS_VALID | (a.runningN * 2 >= a.n ? TS_RUNNING : 0);
      x.rate = trendClamp16((float)a.rateSum / (float)a.n);
      x.drops = (uint16_t)(a.dropsAdded > 65535 ? 65535 : a.dropsAdded);
      x.volMl = trendClamp16(bedVolumeMl(b));
      x.alert = a.alertMax;
      x.link = b.linkPct;
      x.rssi = b.rssi;
    }
    t.ring[i].add(x);
    if (out) out[i] = x;
    const bool keepStart = a.haveStart;
    const uint32_t last = a.dropsLast;
    a = TrendAcc();
    if (keepStart) { a.haveStart = true; a.dropsLast = last; }       // นาทีถัดไปนับต่อจากตัวนับล่าสุด ไม่ทิ้งหยดที่ตกคาบเกี่ยวรอยต่อ
  }
  return true;
}

// ---- สรุปสำหรับรายงาน ------------------------------------------------------------------------------
struct TrendSummary {
  int minutes = 0;            // จำนวนนาทีที่ขอ (ไม่เกินที่มี)
  int validMinutes = 0;       // นาทีที่มีข้อมูลจริง
  int runningMinutes = 0;     // นาทีที่กำลังนับ (ไม่ได้หยุดชั่วคราว)
  int gapMinutes = 0;         // นาทีที่ไม่มีข้อมูล (Station เงียบ)
  float avgRate = 0;          // เฉลี่ยเฉพาะนาทีที่กำลังนับ
  uint16_t minRate = 0, maxRate = 0;
  float totalMl = 0;          // ปริมาตรที่ให้ในช่วงนี้ = ผลรวมหยด / drop factor ปัจจุบัน
  float avgTarget = 0;
  float devPct = 0;           // ความคลาดเฉลี่ยจากเป้า (ค่าสัมบูรณ์ %) เฉพาะนาทีที่เป้า > 0 และกำลังนับ
  int withinPct = 0;          // % ของนาทีที่อยู่ในช่วง ±10% ของเป้า (คิดจากนาทีที่เป้า > 0 และกำลังนับ)
  int alertMinutes[8] = {0};  // นาทีที่มีรหัสเตือนแต่ละชนิด (ดัชนี = รหัส 0..7)
  int alertEpisodes = 0;      // จำนวนครั้งที่เกิดเตือน (นับการเปลี่ยนจากไม่เตือนเป็นเตือนวิกฤต)
};

inline bool trendAlertCritical(uint8_t code) { return code != ALERT_NONE && code != ALERT_NEAR_END && code < 8; }

// สรุปย้อนหลัง mins นาทีล่าสุดของเตียงหนึ่ง (df = drop factor ใช้แปลงหยดเป็น mL)
inline TrendSummary trendSummarize(const TrendRing &r, int mins, uint8_t df) {
  TrendSummary s;
  s.minutes = mins > r.count ? r.count : mins;
  double rateSum = 0, targetSum = 0, devSum = 0;
  uint32_t drops = 0;
  int devN = 0, within = 0, first = 1;
  bool prevCritical = false;
  for (int i = s.minutes - 1; i >= 0; i--) {                       // เก่า -> ใหม่
    const TrendSample &x = r.newest(i);
    if (!(x.flags & TS_VALID)) { s.gapMinutes++; prevCritical = false; continue; }
    s.validMinutes++;
    drops += x.drops;
    if (x.alert < 8) s.alertMinutes[x.alert]++;
    const bool crit = trendAlertCritical(x.alert);
    if (crit && !prevCritical) s.alertEpisodes++;
    prevCritical = crit;
    if (!(x.flags & TS_RUNNING)) continue;
    s.runningMinutes++;
    rateSum += x.rate; targetSum += x.target;
    if (first || x.rate < s.minRate) s.minRate = x.rate;
    if (first || x.rate > s.maxRate) s.maxRate = x.rate;
    first = 0;
    if (x.target > 0) {
      const double dev = (x.rate > x.target ? x.rate - x.target : x.target - x.rate) * 100.0 / x.target;
      devSum += dev; devN++;
      if (dev <= 10.0) within++;
    }
  }
  if (s.runningMinutes > 0) { s.avgRate = (float)(rateSum / s.runningMinutes); s.avgTarget = (float)(targetSum / s.runningMinutes); }
  s.totalMl = (float)drops / (float)safeDropFactor(df);
  if (devN > 0) { s.devPct = (float)(devSum / devN); s.withinPct = (int)(within * 100.0 / devN + 0.5); }
  return s;
}

// บรรทัด CSV ของหนึ่งตัวอย่าง: epoch,upMin,bed,rate,target,volMl,drops,alert,link,rssi,running  คืนความยาว (0 ถ้าบัฟเฟอร์เล็กเกิน)
// upMin = นาทีนับจากเปิดเครื่องของตัวอย่างนี้ epoch = 0 ถ้ายังไม่ได้ตั้งนาฬิกา
inline int trendCsvLine(char *out, size_t cap, uint32_t epoch, uint32_t upMin, int bed, const TrendSample &x) {
  const int n = snprintf(out, cap, "%lu,%lu,%d,%u,%u,%u,%u,%s,%u,%d,%d\n", (unsigned long)epoch, (unsigned long)upMin, bed, (unsigned)x.rate,
                         (unsigned)x.target, (unsigned)x.volMl, (unsigned)x.drops, alertName(x.alert), (unsigned)x.link, (int)x.rssi,
                         (x.flags & TS_RUNNING) ? 1 : 0);
  return (n > 0 && (size_t)n < cap) ? n : 0;
}
constexpr const char *TREND_CSV_HEADER = "epoch,upMin,bed,rate_mlh,target_mlh,vol_ml,drops,alert,link_pct,rssi,running\n";

// ชื่อวัน YYYYMMDD จาก epoch (UTC) บวกเขตเวลา tzMin นาที ใช้ตั้งชื่อไฟล์รายวันบน SD (ไม่ใช้ gmtime เพื่อให้เหมือนกันทุกแพลตฟอร์ม)
// ขั้นตอนแปลงวันที่แบบ civil_from_days ของ Howard Hinnant ใช้ได้ทุกวันที่หลัง 1970
inline void trendDayName(uint32_t epoch, int tzMin, char out[9]) {
  const int64_t secs = (int64_t)epoch + (int64_t)tzMin * 60;
  int64_t z = (secs >= 0 ? secs : secs - 86399) / 86400 + 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const unsigned doe = (unsigned)(z - era * 146097);
  const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int64_t y = (int64_t)yoe + era * 400;
  const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const unsigned mp = (5 * doy + 2) / 153;
  const unsigned d = doy - (153 * mp + 2) / 5 + 1;
  const unsigned m = mp < 10 ? mp + 3 : mp - 9;
  if (m <= 2) y++;
  snprintf(out, 9, "%04u%02u%02u", (unsigned)(y % 10000), (unsigned)(m % 100), (unsigned)(d % 100));
}
