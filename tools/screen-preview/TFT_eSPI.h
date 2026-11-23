#pragma once
#include "Adafruit_GFX.h"
// สตับของไลบรารี TFT_eSPI สำหรับตรวจคอมไพล์/พรีวิวบนเครื่อง PC (ใช้ชุดคำสั่งวาดเดียวกับ Adafruit_GFX)
class TFT_eSPI : public Adafruit_GFX {
public:
  TFT_eSPI(int = 240, int = 320) {}
  void init(uint8_t = 0) {}
  void invertDisplay(bool) {}
};
