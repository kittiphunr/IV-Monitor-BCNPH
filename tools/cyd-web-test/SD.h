#pragma once
// SD ตัวแทนสำหรับทดสอบ: begin() สำเร็จเมื่อ g_mem.cardPresent
#include "FS.h"
#include "SPI.h"
class SDFS : public fs::FS {
 public:
  bool begin(uint8_t = 0, SPIClass & = SPI, uint32_t = 4000000, const char * = "/sd", uint8_t = 5) { return fs::g_mem.cardPresent; }
  void end() {}
  uint64_t cardSize() { return 0; }
};
inline SDFS SD;
