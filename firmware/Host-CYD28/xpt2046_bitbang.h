/**
 * @file      xpt2046_bitbang.h
 * @brief     ไดรเวอร์ทัช XPT2046 แบบ bit-bang สำหรับบอร์ด ESP32-2432S028 (CYD)
 * @version   1.0.0-CYD
 * @date      2026-11-22
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * บอร์ด CYD ต่อทัช XPT2046 ไว้ที่ขา CLK=25 MOSI=32 MISO=39 CS=33 IRQ=36 ซึ่งแยกจากบัสของจอ
 * ส่วนการ์ด SD ก็ใช้อีกบัสหนึ่ง (18/19/23) ESP32 รุ่นปกติมีบัส SPI ให้ใช้แค่สองบัส (HSPI/VSPI)
 * จึงไม่มีบัสเหลือให้ไลบรารีทัชมาตรฐาน อ่านทัชด้วยการขยับขาเองแทน ไม่แย่งบัสกับใคร
 *
 * ให้ค่าเหมือนไลบรารี XPT2046_Touchscreen ของ Paul Stoffregen ในสาระสำคัญ คือชื่อเมธอด
 * วิธีคิดแรงกด (z = z1 + 4095 - z2) เฉลี่ยสองในสามตัวอย่าง และการแปลงตามการหมุนจอ
 * โค้ดฝั่ง Host จึงเรียกใช้ได้เหมือนเดิม ส่วนค่าปรับความแม่นให้ทำใหม่จากหน้า CALIBRATE ในเครื่อง
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 1.0.0-CYD | 2026-11-22 | สร้างใหม่สำหรับบอร์ด ESP32-2432S028 |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 * @note     ขา IRQ (GPIO36) เป็นขารับอย่างเดียว ไม่มีตัวต้านทานดึงในชิป แต่บนบอร์ดมีให้แล้ว
 */
#pragma once
#include <Arduino.h>

struct TS_Point {
  int16_t x = 0, y = 0, z = 0;
};

class XPT2046_Bitbang {
 public:
  XPT2046_Bitbang(uint8_t clk, uint8_t mosi, uint8_t miso, uint8_t cs, uint8_t irq = 255)
      : _clk(clk), _mosi(mosi), _miso(miso), _cs(cs), _irq(irq) {}

  bool begin() {
    pinMode(_clk, OUTPUT);
    pinMode(_mosi, OUTPUT);
    pinMode(_miso, INPUT);
    pinMode(_cs, OUTPUT);
    if (_irq != 255) pinMode(_irq, INPUT);
    digitalWrite(_cs, HIGH);
    digitalWrite(_clk, LOW);
    return true;
  }

  void setRotation(uint8_t r) { _rotation = r & 3; }

  // แตะอยู่หรือไม่ (อ่านซ้ำได้ถี่ แต่จะอ่านจากชิปจริงอย่างมากทุก 2 มิลลิวินาที)
  bool touched() {
    update();
    return _p.z >= Z_THRESHOLD;
  }

  TS_Point getPoint() {
    update();
    return _p;
  }

 private:
  static const int16_t Z_THRESHOLD = 400;
  static const uint8_t CMD_X = 0xD0, CMD_Y = 0x90, CMD_Z1 = 0xB0, CMD_Z2 = 0xC0;   // 12 บิต แบบ differential
  uint8_t _clk, _mosi, _miso, _cs, _irq;
  uint8_t _rotation = 0;
  TS_Point _p;
  unsigned long _lastMs = 0;

  // อ่านหนึ่งช่องสัญญาณตามรอบ 24 พัลส์ในดาต้าชีต (SPI โหมด 0 ถือ CS ต่ำตลอดรอบ)
  //   พัลส์ 1-8   : ส่งคำสั่ง
  //   พัลส์ 9     : บิต BUSY (เป็น 0 เสมอ)
  //   พัลส์ 10-21 : ผลแปลง 12 บิต MSB ก่อน
  //   พัลส์ 22-24 : ศูนย์ท้าย
  // อ่านทั้ง 16 พัลส์หลังคำสั่งแล้วเลื่อนทิ้ง 3 บิต ได้ผลแปลง 12 บิต
  // ใช้คำสั่งที่ PD1:PD0 = 00 หมด ชิปจึงกลับโหมดพักและเปิดขา IRQ ให้เองหลังทุกรอบ
  int16_t readChannel(uint8_t cmd) {
    digitalWrite(_cs, LOW);
    for (int8_t i = 7; i >= 0; i--) {
      digitalWrite(_mosi, (cmd >> i) & 1);
      digitalWrite(_clk, HIGH);
      delayMicroseconds(1);
      digitalWrite(_clk, LOW);
      delayMicroseconds(1);
    }
    digitalWrite(_mosi, LOW);
    uint16_t in = 0;
    for (int8_t i = 0; i < 16; i++) {
      digitalWrite(_clk, HIGH);
      delayMicroseconds(1);
      in = (uint16_t)((in << 1) | (digitalRead(_miso) ? 1 : 0));
      digitalWrite(_clk, LOW);
      delayMicroseconds(1);
    }
    digitalWrite(_cs, HIGH);
    return (int16_t)(in >> 3);
  }

  static int16_t best2avg(int16_t a, int16_t b, int16_t c) {
    int16_t dab = a > b ? a - b : b - a;
    int16_t dac = a > c ? a - c : c - a;
    int16_t dbc = b > c ? b - c : c - b;
    if (dab <= dac && dab <= dbc) return (a + b) / 2;
    if (dac <= dab && dac <= dbc) return (a + c) / 2;
    return (b + c) / 2;
  }

  void update() {
    unsigned long now = millis();
    if (now - _lastMs < 2) return;
    _lastMs = now;

    // ขา IRQ ต่ำเฉพาะตอนมีการกด (หลังชิปอยู่โหมดพักจากการอ่านรอบก่อน)
    if (_irq != 255 && digitalRead(_irq) != LOW) {
      _p.z = 0;
      return;
    }

    int16_t z1 = readChannel(CMD_Z1);
    int16_t z2 = readChannel(CMD_Z2);
    int16_t z = z1 + 4095 - z2;
    if (z < Z_THRESHOLD) {
      _p.z = 0;
      return;
    }
    readChannel(CMD_Y);                          // ทิ้งผลแปลงรอบแรก ให้ค่านิ่งก่อน
    int16_t x0 = readChannel(CMD_X), y0 = readChannel(CMD_Y);
    int16_t x1 = readChannel(CMD_X), y1 = readChannel(CMD_Y);
    int16_t x2 = readChannel(CMD_X), y2 = readChannel(CMD_Y);

    int16_t x = best2avg(x0, x1, x2);
    int16_t y = best2avg(y0, y1, y2);
    switch (_rotation) {                         // เหมือน XPT2046_Touchscreen::update()
      case 0:  _p.x = 4095 - y; _p.y = x;        break;
      case 1:  _p.x = x;        _p.y = y;        break;
      case 2:  _p.x = y;        _p.y = 4095 - x; break;
      default: _p.x = 4095 - x; _p.y = 4095 - y; break;
    }
    _p.z = z;
  }
};
