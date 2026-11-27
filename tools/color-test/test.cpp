// ทดสอบ CYD-ColorTest บนเครื่อง PC: ทุกคำสั่งทำงาน ชุดค่าวนครบ ข้อความอยู่ในจอ และมีภาพให้ดูผัง
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include "Arduino.h"

FILE *g_ops = nullptr;
SerialClass Serial;
ESPClass ESP;
unsigned long g_ms = 1000;
unsigned long millis() { return g_ms; }
unsigned long micros() { return g_ms * 1000UL; }
void delay(unsigned long ms) { g_ms += ms; }
void delayMicroseconds(unsigned long) {}
void pinMode(int, int) {}
void digitalWrite(int, int) {}
static unsigned long g_pressUntil = 0;           // ปุ่ม BOOT ถูกกดอยู่จนถึงเวลานี้ (เวลาเดินด้วย delay() ในเฟิร์มแวร์)
int digitalRead(int p) { return p == 0 ? (g_ms < g_pressUntil ? 0 : 1) : 1; }
bool ledcAttach(int, int, int) { return true; }
void ledcWrite(int, int) {}
void ledcSetup(int, int, int) {}
void ledcAttachPin(int, int) {}
void analogSetAttenuation(adc_attenuation_t) {}
void analogReadResolution(int) {}
int analogRead(int) { return 0; }
void tone(int, unsigned int, unsigned long) {}
void noTone(int) {}
long map(long x, long a, long b, long c, long d) { return (x - a) * (d - c) / (b - a) + c; }

#include "sketch.cpp"

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { printf("ล้มเหลว: %s (บรรทัด %d)\n", msg, __LINE__); fails++; } } while (0)

static std::string g_path;
static long g_checked = 0;
// ข้อความทุกบรรทัดต้องอยู่ในจอ 320x240 และไม่ซ้อนกับบรรทัดอื่นที่ y เดียวกัน (ตรวจแค่ขอบเขต)
static void checkBounds(const char *what) {
  fflush(g_ops);
  FILE *f = fopen(g_path.c_str(), "r");
  fseek(f, g_checked, SEEK_SET);
  char line[512];
  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "TEXT ", 5) != 0) continue;
    int x, y, size, col, bg, off = 0;
    sscanf(line, "TEXT %d %d %d %d %d %n", &x, &y, &size, &col, &bg, &off);
    int len = (int)strlen(line + off);
    while (len > 0 && (line[off + len - 1] == '\n' || line[off + len - 1] == '\r')) len--;
    if (x < 0 || y < 0 || x + len * 6 * size > 320 || y + 8 * size > 240) {
      printf("ล้มเหลว: %s ข้อความล้นจอที่ (%d,%d) ขนาด %d: %.*s\n", what, x, y, size, len, line + off);
      fails++;
    }
  }
  g_checked = ftell(f);
  fclose(f);
}

static void press(unsigned ms) { g_pressUntil = g_ms + ms; loop(); }

int main(int argc, char **argv) {
  g_path = argc > 1 ? argv[1] : "/tmp/cyd_colortest_ops.txt";
  g_ops = fopen(g_path.c_str(), "w");
  setup();
  CHECK(panelType == START_PANEL && (START_PANEL ? st != nullptr : ili != nullptr), "ต้องเริ่มที่ชิปตาม START_PANEL");
  if (START_PANEL) { handleCommand('p'); CHECK(panelType == 0 && ili != nullptr && st == nullptr, "p ต้องสลับกลับเป็น ILI9341"); }
  fprintf(g_ops, "MARK ili9341_set0\n");
  checkBounds("ILI9341 ชุดแรก");
  if (panelType != 0) { handleCommand('p'); }
  // ILI9341: สองชุด (กลับสี) แล้ววนกลับ
  const bool inv0 = invertOn;
  handleCommand('n'); CHECK(invertOn != inv0, "n ต้องสลับกลับสีบน ILI9341");
  handleCommand('n'); CHECK(invertOn == inv0, "ILI9341 วนครบสองชุด");
  // สลับเป็น ST7789: ต้องวนสี่ชุดไม่ซ้ำกันแล้วกลับมาจุดเดิม
  handleCommand('p');
  CHECK(panelType == 1 && st != nullptr && ili == nullptr, "p ต้องสลับเป็น ST7789 และคืนตัวเก่า");
  fprintf(g_ops, "MARK st7789_set0\n");
  checkBounds("ST7789 ชุดแรก");
  bool seen[4] = {false, false, false, false};
  for (int i = 0; i < 4; i++) { seen[(invertOn ? 2 : 0) + (bgrOn ? 1 : 0)] = true; handleCommand('n'); checkBounds("ST7789 วนชุด"); }
  CHECK(seen[0] && seen[1] && seen[2] && seen[3], "ST7789 ต้องวนครบ 4 ชุด (กลับสี x BGR)");
  const bool i1 = invertOn, b1 = bgrOn;
  handleCommand('i'); handleCommand('i'); handleCommand('b'); handleCommand('b');
  CHECK(invertOn == i1 && bgrOn == b1, "i/b สลับสองครั้งต้องกลับมาเดิม");
  for (int r = 0; r < 4; r++) { handleCommand('r'); checkBounds("หมุนจอ"); }
  CHECK(rotation == 1, "หมุนครบ 4 ครั้งต้องกลับมาเดิม");
  handleCommand('h');
  // โหมดเดินเอง
  handleCommand('a'); CHECK(autoMode, "a เปิดโหมดเดินเอง");
  const bool beforeInv = invertOn, beforeBgr = bgrOn;
  g_ms += 3000; loop();
  CHECK(invertOn == beforeInv && bgrOn == beforeBgr, "ยังไม่ถึง 4 วินาทีต้องไม่เปลี่ยนชุด");
  g_ms += 1500; loop();
  CHECK(invertOn != beforeInv || bgrOn != beforeBgr, "ครบ 4 วินาทีต้องเปลี่ยนชุดเอง");
  handleCommand('a'); CHECK(!autoMode, "a ปิดโหมดเดินเอง");
  // ปุ่ม BOOT: กดสั้น = ชุดถัดไป · กดค้าง = สลับชิป
  {
    const bool sb = bgrOn, si = invertOn; const uint8_t sp = panelType;
    press(200);
    CHECK(panelType == sp && (bgrOn != sb || invertOn != si), "กด BOOT สั้นต้องเปลี่ยนชุดค่า ไม่สลับชิป");
    press(1500);
    CHECK(panelType != sp, "กด BOOT ค้างต้องสลับชิป");
    CHECK(g_ms >= g_pressUntil, "ต้องรอปล่อยปุ่มก่อนกลับไปทำงาน (กันสั่งซ้ำ)");
    checkBounds("หลังสลับชิปด้วยปุ่ม");
  }
  fclose(g_ops);
  if (fails) { printf("รวมล้มเหลว %d รายการ\n", fails); return 1; }
  printf("ผ่านทั้งหมด\n");
  return 0;
}
