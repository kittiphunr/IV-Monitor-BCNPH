// ทดสอบไดรเวอร์ทัช bit-bang กับแบบจำลองชิป XPT2046 (ตามรอบ 24 พัลส์ในดาต้าชีต)
// ตรวจว่า: ไม่แตะ -> ไม่ขึ้นแตะ, แตะ -> ได้ x y z ถูก, แปลงตามการหมุนจอ, ไม่แตะแล้วหยุดอ่านชิปที่ขา IRQ
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "Arduino.h"

// ---------------- แบบจำลองชิป ----------------
static const int P_CLK = 25, P_MOSI = 32, P_MISO = 39, P_CS = 33, P_IRQ = 36;
static bool pen = false;
static int penX = 0, penY = 0, penZ1 = 0, penZ2 = 0;   // ค่าดิบ 12 บิตที่ชิปจะตอบ
static int lvl[64];
static int edge, cmd, outBits, outLen, irqReads, chanReads;
static int outBit = 0;

static int channelValue(int c) {
  switch (c & 0x70) {
    case 0x50: return penX;   // A=101 X
    case 0x10: return penY;   // A=001 Y
    case 0x30: return penZ1;  // A=011 Z1
    case 0x40: return penZ2;  // A=100 Z2
  }
  return 0;
}
unsigned long g_ms = 100;
unsigned long millis() { return g_ms; }
void delay(unsigned long ms) { g_ms += ms; }
void delayMicroseconds(unsigned long) {}
void pinMode(int, int) {}
void digitalWrite(int pin, int v) {
  int old = lvl[pin]; lvl[pin] = v;
  if (pin == P_CS && v == 0) { edge = 0; cmd = 0; outLen = 0; outBit = 0; }
  if (pin == P_CLK && v == 1 && old == 0 && lvl[P_CS] == 0) {      // ขอบขึ้น
    edge++;
    if (edge <= 8) cmd = (cmd << 1) | lvl[P_MOSI];
    if (edge == 8) { outBits = (channelValue(cmd) & 0xFFF); chanReads++; }
  }
  if (pin == P_CLK && v == 0 && old == 1 && lvl[P_CS] == 0) {      // ขอบลง: ชิปเปลี่ยนบิตขาออก
    if (edge == 8) outBit = 0;                                     // BUSY
    else if (edge >= 9 && edge <= 20) outBit = (outBits >> (20 - edge)) & 1;   // D11..D0 ปรากฏหลังขอบลงที่ 9..20
    else outBit = 0;
  }
}
int digitalRead(int pin) {
  if (pin == P_IRQ) { irqReads++; return pen ? 0 : 1; }
  if (pin == P_MISO) return outBit;
  return 1;
}
#include "xpt2046_bitbang.h"

static int bad = 0;
#define CHECK(c, msg) do { if (!(c)) { printf("ผิด: %s\n", msg); bad++; } else printf("ผ่าน: %s\n", msg); } while (0)

int main() {
  XPT2046_Bitbang ts(P_CLK, P_MOSI, P_MISO, P_CS, P_IRQ);
  ts.begin();
  ts.setRotation(1);   // หมุน 1 = ค่าดิบตรง ๆ ใช้ตรวจการถอดบิตก่อน

  pen = false; g_ms += 10;
  chanReads = 0;
  CHECK(!ts.touched(), "ไม่แตะ -> touched() เป็นเท็จ");
  CHECK(chanReads == 0, "ไม่แตะ -> ไม่คุยกับชิปเลย (ดูจากขา IRQ อย่างเดียว)");

  pen = true; penX = 1234; penY = 2890; penZ1 = 900; penZ2 = 2500;   // z = 900+4095-2500 = 2495
  g_ms += 10;
  CHECK(ts.touched(), "แตะ -> touched() เป็นจริง");
  TS_Point p = ts.getPoint();
  CHECK(p.x == 1234 && p.y == 2890, "ถอดค่า x y 12 บิตได้ตรงตามที่ชิปตอบ");
  CHECK(p.z == 2495, "แรงกด z = z1 + 4095 - z2");

  penZ1 = 100; penZ2 = 4000; g_ms += 10;   // z = 195 ต่ำกว่าเกณฑ์
  CHECK(!ts.touched(), "แรงกดต่ำกว่าเกณฑ์ -> ไม่ถือว่าแตะ");

  penX = 1000; penY = 3000; penZ1 = 900; penZ2 = 2500; g_ms += 10;
  ts.setRotation(0); p = ts.getPoint();
  CHECK(p.x == 4095 - 3000 && p.y == 1000, "หมุน 0: x=4095-y  y=x");
  g_ms += 10; ts.setRotation(2); p = ts.getPoint();
  CHECK(p.x == 3000 && p.y == 4095 - 1000, "หมุน 2: x=y  y=4095-x");
  g_ms += 10; ts.setRotation(3); p = ts.getPoint();
  CHECK(p.x == 4095 - 1000 && p.y == 4095 - 3000, "หมุน 3: กลับทั้งสองแกน");

  printf(bad ? "\nไม่ผ่าน %d ข้อ\n" : "\nผ่านทุกข้อ\n", bad);
  return bad ? 1 : 0;
}
