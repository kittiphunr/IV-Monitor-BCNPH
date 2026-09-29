// ฉากทดสอบความเข้ากันได้ระหว่าง Station กับ Host (ใช้ร่วมกันทั้งฝั่ง Station และฝั่ง Host)
// ทุกฉากใช้เตียง 3 เวลาเป็นมิลลิวินาทีนับจากเริ่มฉาก
#pragma once
#include <cstring>

struct Scenario {
  const char *name;
  unsigned char df;          // drop factor ที่ Host ตั้ง
  float target;              // อัตราเป้าหมาย mL/h
  float plan;                // ปริมาตรตามแผน mL
  float startMl;             // ปริมาตรที่ให้ไปแล้วตอนเริ่มฉาก (กู้คืนหลังบูต)
  unsigned long firstDrop;   // หยดแรก
  unsigned long gap;         // ช่วงห่างระหว่างหยด (0 = ไม่มีหยด)
  unsigned long dropsUntil;  // หยุดหยดหลังเวลานี้
  unsigned long dur;         // ความยาวฉาก
  unsigned char resetSeq;    // resetSeq ที่ Host ถืออยู่ (Host เก็บค่านี้ลง NVS)
  unsigned long newBagAt;    // เวลาที่พยาบาลกด "เริ่มถุงใหม่" ที่ Host (resetSeq + 1) 0 = ไม่กด
};

//              ชื่อ        df  เป้า  แผน  เริ่ม  หยดแรก ห่าง  หยดถึง  ยาว   seq ถุงใหม่
static const Scenario SCENARIOS[] = {
  {"normal",   20, 100, 1000, 620,  500, 1800, 40000, 40000, 0,    0},  // ไหลปกติ 100 mL/h
  {"fast",     10, 100, 1000, 620,  500,  100, 60000, 60000, 0,    0},  // ไหลอิสระ 3600 mL/h (DF10)
  {"maxrate",  10, 100, 1000, 620,  500,   70, 60000, 60000, 0,    0},  // เกือบถึงขีดที่ Station นับได้ 5143 mL/h
  {"nearend",  20, 100, 1000, 830,  500, 1800, 30000, 30000, 0,    0},  // เกิน 80% ของแผน
  {"reboot",   20, 100, 1000, 620,  500, 1800, 10000, 10000, 3,    0},  // บูตใหม่ ขณะ Host มี resetSeq = 3
  {"newbag",   20, 100, 1000, 620,  500, 1800, 15000, 15000, 3, 6000},  // กดเริ่มถุงใหม่กลางฉาก
};

static const Scenario &findScenario(const char *n) {
  for (const Scenario &s : SCENARIOS) if (!strcmp(s.name, n)) return s;
  return SCENARIOS[0];
}

static const unsigned char BED = 3;
