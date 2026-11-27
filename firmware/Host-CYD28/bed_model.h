/**
 * @file      bed_model.h
 * @brief     แบบจำลองเตียง: รับแพ็กเก็ต สถานะออนไลน์ การตัดสินรหัสเตือน คุณภาพลิงก์ ตัวจัดช่วงส่ง Sync
 * @version   2.0.4-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * ตรรกะล้วน ไม่แตะฮาร์ดแวร์ เวลาส่งเข้ามาเป็นพารามิเตอร์ ทดสอบบน PC ได้ Host เป็นผู้ตัดสินรหัสเตือนจุดเดียว แล้วส่งให้ Station ผ่าน struct_host_sync
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.4-CYD | 2026-11-27 | Bed::supportsNoSignal อ่านจากบิต 0x02 ของแพ็กเก็ตล่าสุด และ bedBuildSync() ส่งรหัส NO DROP (6) ให้เฉพาะเตียงที่ประกาศรองรับ เตียงรุ่นเก่ายังได้ 0 เพื่อไม่ให้ Station ที่ไม่รู้จักรหัสนี้ทำงานผิด |
 * | 2.0.3-CYD | 2026-11-27 | SyncScheduler::isPending() สำหรับทดสอบว่ามีคำขอส่งด่วนค้างหรือไม่ |
 * | 2.0.2-CYD | 2026-11-27 | เพิ่ม bedRemainingMin() สำหรับหน้าจอด่านที่ 3 |
 * | 2.0.1-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 2 (แกนเครือข่าย) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <string.h>
#include "protocol.h"

constexpr int      MAX_BEDS             = 8;
constexpr uint32_t SYNC_INTERVAL_MS     = 1000;     // วนส่ง Sync ครบทุกเตียงภายในเวลานี้
constexpr uint32_t SYNC_SLOT_MIN_MS     = 60;
constexpr uint32_t ONLINE_TIMEOUT_MS    = 8000;     // เงียบเกินนี้ = OFFLINE
constexpr uint32_t LOST_LINK_GRACE_MS   = 30000;    // OFFLINE แล้วเงียบต่ออีกเท่านี้ = ขาดการติดต่อจริง
constexpr uint32_t NO_SIGNAL_MS         = 90000;    // ติดต่อได้แต่ไม่เคยมีหยดนานเท่านี้
constexpr uint32_t MIN_OCCLUSION_MS     = 8000;
constexpr uint32_t MAX_OCCLUSION_MS     = 300000;
constexpr uint32_t RATE_HOLD_MS         = 20000;    // เร็ว/ช้าเกินต่อเนื่องเท่านี้จึงเตือน
constexpr float    FAST_RATIO           = 1.35f;
constexpr float    SLOW_RATIO           = 0.65f;
constexpr uint32_t LINK_WINDOW_MS       = 10000;
constexpr uint8_t  LINK_WEAK_PCT        = 60;
constexpr uint32_t ID_CONFLICT_WINDOW_MS = 30000;
constexpr uint8_t  ID_CONFLICT_MIN_FLIPS = 4;
constexpr uint8_t  DEFAULT_NEAR_END_PCT  = 80;

inline uint8_t safeDropFactor(uint8_t df) { return (df == 10 || df == 15 || df == 20 || df == 60) ? df : 20; }
inline uint8_t safeNearPct(uint8_t p) { return (p >= 50 && p <= 95) ? p : DEFAULT_NEAR_END_PCT; }

struct BedConfig {
  float   targetRateHr = 0.0f;
  float   planVolumeMl = 0.0f;
  uint8_t dropFactor   = 20;
  uint8_t nearEndPct   = DEFAULT_NEAR_END_PCT;
  uint8_t resetSeq     = 0;
};

struct Bed {
  BedConfig cfg;
  bool     seen = false;           // เคยได้ยินเตียงนี้ตั้งแต่บูต
  bool     running = true;
  int8_t   rssi = -100;
  float    batteryV = 0;
  uint32_t totalDrops = 0;
  float    flowRateHr = 0;
  uint32_t msSinceLastDrop = 0;
  uint32_t lastRecvMs = 0;
  uint32_t firstRecvMs = 0;
  uint8_t  alert = ALERT_NONE;
  uint32_t deviationSince = 0;
  bool     nearEndAck = false;
  bool     wasMonitoring = false;
  bool     lostLinkAck = false;
  uint16_t rxWindow = 0;
  uint32_t rxTotal = 0;
  uint8_t  linkPct = 0;
  uint8_t  srcMac[6] = {0};
  bool     macKnown = false;
  uint8_t  flips = 0;
  uint32_t flipWindowMs = 0;
  bool     idConflict = false;
  bool     supportsNoSignal = false;   // [2.0.4-CYD] เพิ่ม: Station ประกาศว่ารู้จักรหัส 6 (อ่านใหม่ทุกแพ็กเก็ต)
};

inline bool bedOnline(const Bed &b, uint32_t now) {
  return b.seen && (uint32_t)(now - b.lastRecvMs) < ONLINE_TIMEOUT_MS;
}

inline bool bedLostLink(const Bed &b, uint32_t now) {
  if (!b.wasMonitoring || b.lostLinkAck || !b.seen || bedOnline(b, now)) return false;
  return (uint32_t)(now - b.lastRecvMs) > (ONLINE_TIMEOUT_MS + LOST_LINK_GRACE_MS);
}

inline bool bedNoSignal(const Bed &b, uint32_t now) {
  if (!bedOnline(b, now) || !b.running || b.totalDrops > 0 || !b.seen) return false;
  return (uint32_t)(now - b.firstRecvMs) > NO_SIGNAL_MS;
}

// เกณฑ์ "ไม่มีหยด" = 2.5 เท่าของช่วงหยดตามเป้าหมาย ขั้นต่ำ 8 วินาที เพดาน 5 นาที (สูตรเดียวกับ Station)
inline uint32_t occlusionThresholdMs(float rateHr, uint8_t df) {
  if (rateHr <= 0.0f) return MIN_OCCLUSION_MS;
  const float dropsPerHr = rateHr * (float)df;
  uint32_t th = ((uint32_t)(3600000.0f / dropsPerHr) * 5) / 2;
  if (th < MIN_OCCLUSION_MS) th = MIN_OCCLUSION_MS;
  if (th > MAX_OCCLUSION_MS) th = MAX_OCCLUSION_MS;
  return th;
}

inline bool bedOccluded(const Bed &b, uint32_t now) {
  if (!bedOnline(b, now) || !b.running || b.totalDrops == 0 || b.msSinceLastDrop == 0) return false;
  const uint32_t since = b.msSinceLastDrop + (uint32_t)(now - b.lastRecvMs);
  return since > occlusionThresholdMs(b.cfg.targetRateHr, safeDropFactor(b.cfg.dropFactor));
}

inline float bedVolumeMl(const Bed &b) { return (float)b.totalDrops / (float)safeDropFactor(b.cfg.dropFactor); }

// เวลาที่เหลือจนครบแผน (นาที) ใช้อัตราจริงถ้ามี ไม่งั้นใช้เป้าหมาย คืน -1 เมื่อคำนวณไม่ได้
inline int bedRemainingMin(const Bed &b) {
  const float plan = b.cfg.planVolumeMl, vol = bedVolumeMl(b);
  const float rate = b.flowRateHr > 0 ? b.flowRateHr : b.cfg.targetRateHr;
  if (!(plan > 0) || !(rate > 0) || vol >= plan) return -1;
  const float m = (plan - vol) / rate * 60.0f;
  return m > 99 * 60 ? 99 * 60 : (int)(m + 0.5f);
}

// รับแพ็กเก็ตหนึ่งใบ (ผ่าน packetPlausible มาแล้ว) คืน true เมื่อเลขเตียงมีบอร์ดแย่งกัน
inline void bedIngest(Bed &b, const struct_message &m, int8_t rssi, const uint8_t *src, uint32_t now) {
  if (src != nullptr) {
    if (!b.macKnown) {
      memcpy(b.srcMac, src, 6); b.macKnown = true; b.flipWindowMs = now;
    } else if (memcmp(b.srcMac, src, 6) != 0) {
      memcpy(b.srcMac, src, 6);
      if ((uint32_t)(now - b.flipWindowMs) > ID_CONFLICT_WINDOW_MS) { b.flipWindowMs = now; b.flips = 0; }
      if (b.flips < 255) b.flips++;
      if (b.flips >= ID_CONFLICT_MIN_FLIPS) b.idConflict = true;
    } else if ((uint32_t)(now - b.flipWindowMs) > ID_CONFLICT_WINDOW_MS) {
      b.flipWindowMs = now; b.flips = 0; b.idConflict = false;
    }
  }
  if (!b.seen) b.firstRecvMs = now;
  b.seen = true;
  b.running = (m.isRunning != 0);
  b.rssi = rssi;
  b.batteryV = m.batteryVolts;
  b.totalDrops = m.totalDrops;
  b.flowRateHr = b.running ? m.flowRateHr : 0.0f;
  b.msSinceLastDrop = m.msSinceLastDrop;
  b.lastRecvMs = now;
  if (b.totalDrops > 0) b.firstRecvMs = now;       // ยังมีหยดเข้ามา เลื่อนเวลาอ้างอิงตาม (หลังเริ่มถุงใหม่นับ 90 วินาทีใหม่)
  if (b.rxWindow < 0xFFFF) b.rxWindow++;
  b.rxTotal++;
  if ((m.flags & MSG_FLAG_NEAR_END_ACK) && b.cfg.planVolumeMl > 0) b.nearEndAck = true;
  b.supportsNoSignal = (m.flags & MSG_FLAG_NO_SIGNAL_OK) != 0;   // [2.0.4-CYD] เพิ่ม:
}

// ตัดสินรหัสเตือนของเตียงเดียว คืนรหัสใหม่ (ผู้เรียกเทียบกับ b.alert เพื่อขอส่ง Sync ด่วน)
inline uint8_t bedEvaluate(Bed &b, uint32_t now) {
  if (!bedOnline(b, now)) {
    b.deviationSince = 0;
    return bedLostLink(b, now) ? (uint8_t)ALERT_LOST_LINK : (uint8_t)ALERT_NONE;
  }
  b.lostLinkAck = false;
  b.wasMonitoring = b.running;
  if (!b.running) { b.deviationSince = 0; return ALERT_NONE; }

  const float tr = b.cfg.targetRateHr, act = b.flowRateHr, vol = bedVolumeMl(b), plan = b.cfg.planVolumeMl;
  if (bedNoSignal(b, now))  { b.deviationSince = 0; return ALERT_NO_SIGNAL; }
  if (bedOccluded(b, now))  { b.deviationSince = 0; return ALERT_OCCLUSION; }
  if (plan > 0 && vol >= plan) { b.deviationSince = 0; return ALERT_COMPLETE; }

  const bool rateValid = (act > 0.0f) && (b.totalDrops >= 3);
  uint8_t dev = ALERT_NONE;
  if (tr > 0 && rateValid && act > tr * FAST_RATIO) dev = ALERT_TOO_FAST;
  else if (tr > 0 && rateValid && act < tr * SLOW_RATIO) dev = ALERT_TOO_SLOW;
  uint8_t code = ALERT_NONE;
  if (dev != ALERT_NONE) {
    if (b.deviationSince == 0) b.deviationSince = now;
    if ((uint32_t)(now - b.deviationSince) >= RATE_HOLD_MS) code = dev;
  } else {
    b.deviationSince = 0;
  }
  if (code == ALERT_NONE && plan > 0 && vol >= plan * (float)safeNearPct(b.cfg.nearEndPct) / 100.0f) code = ALERT_NEAR_END;
  return code;
}

// เริ่มถุงใหม่: เปลี่ยน resetSeq ให้ Station ล้างตัวนับ แล้วล้างสถานะฝั่ง Host ที่ผูกกับถุงเดิม
inline void bedNewBag(Bed &b, uint32_t now) {
  b.cfg.resetSeq++;
  b.totalDrops = 0; b.flowRateHr = 0; b.msSinceLastDrop = 0;
  b.alert = ALERT_NONE; b.deviationSince = 0; b.nearEndAck = false;
  b.firstRecvMs = now;
}

// ทุก LINK_WINDOW_MS: ได้รับกี่ % ของจำนวนที่ควรได้ (Station ส่งราว 1 ใบ/วินาที)
inline void bedsLinkTick(Bed *beds, int n) {
  const uint16_t expected = (uint16_t)(LINK_WINDOW_MS / 1000);
  for (int i = 0; i < n; i++) {
    uint32_t pct = ((uint32_t)beds[i].rxWindow * 100u) / expected;
    beds[i].linkPct = (uint8_t)(pct > 100 ? 100 : pct);
    beds[i].rxWindow = 0;
  }
}

inline struct_host_sync bedBuildSync(const Bed &b, int idx, uint8_t channel, uint32_t epoch, uint32_t now) {
  struct_host_sync s = {};
  s.epochTime    = epoch;
  s.isSynced     = epoch > 1600000000UL ? 1 : 0;
  s.stationId    = (uint8_t)(idx + 1);
  s.targetRateHr = b.cfg.targetRateHr;
  s.totalPlanMl  = b.cfg.planVolumeMl;
  s.alertCode    = alertCodeForBed(b.alert, b.supportsNoSignal);   // [2.0.4-CYD] แก้: เดิมแปลง NO DROP เป็น 0 เสมอ
  s.dropFactor   = safeDropFactor(b.cfg.dropFactor);
  s.resetSeq     = b.cfg.resetSeq;
  s.hostChannel  = channel;
  s.nearEndPct   = safeNearPct(b.cfg.nearEndPct);
  s.flags        = (b.nearEndAck ? SYNC_FLAG_NEAR_END_ACK : 0) | SYNC_FLAG_REPORTS | (bedOnline(b, now) ? SYNC_FLAG_HEARD : 0);
  return s;
}

// จัดช่องเวลาส่ง Sync: ส่งได้มากสุดหนึ่งใบต่อช่อง ไม่มี delay() ในลูป งานด่วนมาก่อนรอบปกติ
class SyncScheduler {
 public:
  void request(int idx) { if (idx >= 0 && idx < MAX_BEDS) pending |= (uint16_t)(1u << idx); }
  void requestAll(int n) { for (int i = 0; i < n; i++) request(i); }
  bool isPending(int idx) const { return idx >= 0 && idx < MAX_BEDS && (pending & (1u << idx)) != 0; }
  uint32_t slotMs(int active) const {
    uint32_t n = active < 1 ? 1 : (uint32_t)active;
    uint32_t s = SYNC_INTERVAL_MS / n;
    return s < SYNC_SLOT_MIN_MS ? SYNC_SLOT_MIN_MS : s;
  }
  int next(uint32_t now, int active) {         // คืนเลขเตียงที่ควรส่งตอนนี้ หรือ -1
    if ((uint32_t)(now - lastSlot) < slotMs(active)) return -1;
    lastSlot = now;
    if (pending) {
      for (int i = 0; i < MAX_BEDS; i++)
        if (pending & (1u << i)) { pending &= (uint16_t)~(1u << i); return i; }
    }
    if (active < 1) return -1;
    if (round >= active) round = 0;
    int r = round;
    round = (uint8_t)((round + 1) % active);
    return r;
  }
 private:
  uint16_t pending = 0;
  uint8_t  round = 0;
  uint32_t lastSlot = 0;
};
