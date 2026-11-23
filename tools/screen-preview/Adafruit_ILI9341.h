#pragma once
#include "Adafruit_GFX.h"
#include "SPI.h"
// สตับของไลบรารี Adafruit_ILI9341 สำหรับพรีวิวบนเครื่อง PC (ลำดับพารามิเตอร์ตรงของจริง: DC ก่อน CS)
class Adafruit_ILI9341 : public Adafruit_GFX {
public:
  Adafruit_ILI9341(SPIClass*, int8_t dc, int8_t cs = -1, int8_t rst = -1) { (void)dc; (void)cs; (void)rst; }
  void begin(uint32_t = 0) {}
  void invertDisplay(bool) {}
  uint8_t readcommand8(uint8_t, uint8_t = 0) { return 0; }
};
