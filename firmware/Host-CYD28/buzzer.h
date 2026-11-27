/**
 * @file      buzzer.h
 * @brief     เครื่องยนต์เสียงเตือนและเสียงกดปุ่มสำหรับบัซเซอร์ Active
 * @version   2.0.2-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * ตรรกะล้วนไม่แตะขา: update(now) คืนระดับที่ควรขับ ผู้เรียกเขียนลงขาเอง จังหวะเสียงเตือนตามกลุ่มจำนวนครั้งต่อรอบแบบเดียวกับ Station เพื่อให้พยาบาลได้ยินแล้วแยกชนิดเหตุได้เหมือนกัน (บัซเซอร์ Active ปรับความถี่ไม่ได้ จึงแยกด้วยจำนวนครั้งและความยาว) ไม่ได้ผ่านมาตรฐานเสียงเตือนทางการแพทย์ IEC 60601-1-8
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.2-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 3 (หน้าจอและเสียง) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <stdint.h>
#include "protocol.h"

struct BuzzPattern { uint8_t bursts; uint16_t onMs, gapMs, pauseMs; };

// รหัสเตือน -> จังหวะ (ลำดับความสำคัญสูงไปต่ำคือสายพับ ขาดการติดต่อ เร็วเกิน ครบแผน ไม่มีหยด ช้าเกิน)
inline BuzzPattern buzzPatternFor(uint8_t code) {
  switch (code) {
    case ALERT_OCCLUSION: return {3, 150, 100, 900};
    case ALERT_LOST_LINK: return {5,  80,  80, 2000};
    case ALERT_TOO_FAST:  return {4,  60,  60, 900};
    case ALERT_COMPLETE:  return {2, 250, 200, 1800};
    case ALERT_NO_SIGNAL: return {2, 100, 100, 1500};
    case ALERT_TOO_SLOW:  return {1, 350,   0, 1200};
    default:              return {0, 0, 0, 0};
  }
}

// ลำดับความสำคัญ: เลือกรหัสที่ควรร้องก่อนเมื่อหลายเตียงเตือนพร้อมกัน
inline uint8_t buzzPriority(uint8_t code) {
  switch (code) {
    case ALERT_OCCLUSION: return 6;
    case ALERT_LOST_LINK: return 5;
    case ALERT_TOO_FAST:  return 4;
    case ALERT_COMPLETE:  return 3;
    case ALERT_NO_SIGNAL: return 2;
    case ALERT_TOO_SLOW:  return 1;
    default:              return 0;
  }
}

class BuzzerEngine {
 public:
  bool enabled = true;

  // ตั้งรหัสเตือนที่กำลังร้อง (0 = เงียบ) เริ่มจังหวะใหม่เฉพาะเมื่อรหัสเปลี่ยน
  void setAlert(uint8_t code, uint32_t now) {
    if (code == _code) return;
    _code = code;
    _alertStart = now;
  }
  void click(uint32_t now)  { _clickEnd = now + CLICK_MS; }
  void chime(uint32_t now)  { _chimeStart = now; _chimeOn = true; }       // เตือนใกล้หมด 3 ครั้งสั้น ๆ
  void test(uint32_t now)   { _testEnd = now + 400; }

  // ระดับที่ควรขับขาบัซเซอร์ ณ เวลานี้
  bool update(uint32_t now) {
    if (!enabled) return false;
    if ((int32_t)(_clickEnd - now) > 0) return true;
    if ((int32_t)(_testEnd - now) > 0) return true;
    if (_code != ALERT_NONE) {
      const BuzzPattern p = buzzPatternFor(_code);
      if (p.bursts) return patternLevel(p, now - _alertStart);
    }
    if (_chimeOn) {
      const uint32_t t = now - _chimeStart;
      if (t >= CHIME_TOTAL_MS) _chimeOn = false;
      else return patternLevel({3, 100, 100, 0}, t);
    }
    return false;
  }

  static constexpr uint32_t CLICK_MS = 25;
  static constexpr uint32_t CHIME_TOTAL_MS = 500;
  uint8_t code() const { return _code; }

 private:
  uint8_t  _code = ALERT_NONE;
  uint32_t _alertStart = 0, _clickEnd = 0, _testEnd = 0, _chimeStart = 0;
  bool     _chimeOn = false;

  static bool patternLevel(const BuzzPattern &p, uint32_t t) {
    const uint32_t step = (uint32_t)p.onMs + p.gapMs;
    const uint32_t cycle = (uint32_t)p.bursts * step - p.gapMs + p.pauseMs;
    t %= cycle ? cycle : 1;
    const uint32_t k = t / step;
    return k < p.bursts && (t % step) < p.onMs;
  }
};
