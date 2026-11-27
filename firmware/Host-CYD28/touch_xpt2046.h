/**
 * @file      touch_xpt2046.h
 * @brief     ไดรเวอร์ทัช XPT2046 แบบ bit-bang และตัวติดตามสถานะนิ้ว
 * @version   2.0.0-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * ทัชบน CYD ใช้ขาแยกจากบัสจอและ SD จึงขยับขาเองแทนการใช้บัส SPI ที่ไม่เหลือ อ่านค่าดิบ 0-4095 ไม่แปลงทิศ (การแปลงเป็นพิกเซลอยู่ที่ touch_cal.h) ตัวติดตามกรองสัญญาณสะดุดให้เป็นเหตุการณ์ DOWN/MOVE/UP
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.0-CYD | 2026-11-27 | เขียนใหม่ทั้งหมดด่านที่ 1 (ฐานบอร์ด: จอ ไฟหลังจอ ทัช) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <Arduino.h>

// ค่ากลางของ 5 ตัวอย่าง (เรียงแบบแทรก) ใช้กรองสัญญาณสะดุดของ XPT2046
inline int16_t touchMedian5(int16_t v[5]) {
  for (int i = 1; i < 5; i++) {
    int16_t k = v[i];
    int j = i - 1;
    while (j >= 0 && v[j] > k) { v[j + 1] = v[j]; j--; }
    v[j + 1] = k;
  }
  return v[2];
}

class XptBitbang {
 public:
  XptBitbang(int clk, int mosi, int miso, int cs, int irq)
      : _clk(clk), _mosi(mosi), _miso(miso), _cs(cs), _irq(irq) {}

  void begin() {
    pinMode(_clk, OUTPUT);
    pinMode(_mosi, OUTPUT);
    pinMode(_miso, INPUT);
    pinMode(_cs, OUTPUT);
    pinMode(_irq, INPUT);
    digitalWrite(_cs, HIGH);
    digitalWrite(_clk, LOW);
  }

  // true เมื่อกดอยู่ พร้อมค่าดิบ x y และแรงกด z (z ยิ่งมากยิ่งกดแน่น)
  bool read(int16_t &x, int16_t &y, int16_t &z) {
    if (digitalRead(_irq) != LOW) return false;           // IRQ ต่ำเฉพาะตอนมีแรงกด
    const int16_t z1 = channel(CMD_Z1), z2 = channel(CMD_Z2);
    z = (int16_t)(z1 + 4095 - z2);
    if (z < Z_MIN) return false;
    channel(CMD_X);                                       // ทิ้งผลแรก ให้ตัวแปลงนิ่ง
    int16_t xs[5], ys[5];
    for (int i = 0; i < 5; i++) { xs[i] = channel(CMD_X); ys[i] = channel(CMD_Y); }
    x = touchMedian5(xs);
    y = touchMedian5(ys);
    return true;
  }

 private:
  static const int16_t Z_MIN = 350;
  static const uint8_t CMD_X = 0xD0, CMD_Y = 0x90, CMD_Z1 = 0xB0, CMD_Z2 = 0xC0;  // 12 บิต differential PD=00
  int _clk, _mosi, _miso, _cs, _irq;

  int16_t channel(uint8_t cmd) {
    digitalWrite(_cs, LOW);
    for (int i = 7; i >= 0; i--) {
      digitalWrite(_mosi, (cmd >> i) & 1);
      digitalWrite(_clk, HIGH);
      delayMicroseconds(1);
      digitalWrite(_clk, LOW);
      delayMicroseconds(1);
    }
    digitalWrite(_mosi, LOW);
    uint16_t in = 0;
    for (int i = 0; i < 16; i++) {                        // 1 บิต BUSY + 12 บิตผล + 3 บิตศูนย์
      digitalWrite(_clk, HIGH);
      delayMicroseconds(1);
      in = (uint16_t)((in << 1) | (digitalRead(_miso) ? 1 : 0));
      digitalWrite(_clk, LOW);
      delayMicroseconds(1);
    }
    digitalWrite(_cs, HIGH);
    return (int16_t)(in >> 3);
  }
};

// ตัวติดตามนิ้ว: ต้องเห็นการกดสองครั้งติดกันจึงนับเป็น DOWN และต้องหายเกิน UP_HOLD_MS จึงนับเป็น UP
// (ทัชราคาถูกมักสะดุดเป็นช่วงสั้น ๆ ระหว่างลากนิ้ว)
enum TouchEvt : uint8_t { TOUCH_NONE = 0, TOUCH_DOWN, TOUCH_MOVE, TOUCH_UP };

class TouchTracker {
 public:
  static const uint32_t UP_HOLD_MS = 45;
  bool down() const { return _down; }
  TouchEvt update(bool hit, uint32_t nowMs) {
    if (hit) {
      _lastHit = nowMs;
      if (_down) return TOUCH_MOVE;
      if (++_hits >= 2) { _down = true; return TOUCH_DOWN; }
      return TOUCH_NONE;
    }
    _hits = 0;
    if (_down && (uint32_t)(nowMs - _lastHit) > UP_HOLD_MS) { _down = false; return TOUCH_UP; }
    return TOUCH_NONE;
  }
 private:
  bool _down = false;
  uint8_t _hits = 0;
  uint32_t _lastHit = 0;
};
