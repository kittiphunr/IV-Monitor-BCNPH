// ทดสอบฐานบอร์ด Host-CYD28 ด่านที่ 1 บนเครื่อง PC (ไม่ต้องใช้บอร์ดจริง)
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "Arduino.h"
#include "touch_cal.h"
#include "protocol.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

FILE *g_ops = nullptr;
SerialClass Serial;
ESPClass ESP;

// ---------------- แบบจำลองเวลาและชิปทัช ----------------
static const int P_CLK = 25, P_MOSI = 32, P_MISO = 39, P_CS = 33, P_IRQ = 36;
unsigned long g_ms = 100;
unsigned long millis() { return g_ms; }
unsigned long micros() { return g_ms * 1000UL; }
void delay(unsigned long ms) { g_ms += ms; }
void delayMicroseconds(unsigned long) {}
void pinMode(int, int) {}
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

static const float W = 320, H = 240;                        // แนวนอน
// ความจริงของบอร์ดสมมติ: แกนสลับและกลับด้าน (ผู้ใช้เคยเจอแบบนี้) raw = f(screen)
static void truthRaw(float sx, float sy, int &rx, int &ry) {
  rx = (int)((1.0f - sy / H) * 3800 + 150);
  ry = (int)((sx / W) * 3700 + 200);
}
static CalPoint targets[3];
static unsigned long simT0 = 0;
static bool simEnabled = false;
static bool penDown = false;
static bool holdPen = false;
static int penRx = 0, penRy = 0;
static int lvl[64], edge, cmdByte, outBits, outBit;

static void simUpdate() {
  if (holdPen) { penDown = true; return; }
  if (!simEnabled) { penDown = false; return; }
  const unsigned long t = g_ms - simT0, slot = 1100;        // 400 ms ปล่อย + 700 ms กด ต่อหนึ่งจุด
  const unsigned long i = t / slot, ph = t % slot;
  if (i < 3 && ph >= 400) { penDown = true; truthRaw(targets[i].sx, targets[i].sy, penRx, penRy); penRx += (int)(g_ms % 5) - 2; }
  else penDown = false;
}
static int chan(int c) {
  switch (c & 0x70) {
    case 0x50: return penRx;
    case 0x10: return penRy;
    case 0x30: return 600;
    case 0x40: return 3600;               // z = 600 + 4095 - 3600 = 1095
  }
  return 0;
}
void digitalWrite(int pin, int v) {
  int old = lvl[pin]; lvl[pin] = v;
  if (pin == P_CS && v == 0) { edge = 0; cmdByte = 0; outBit = 0; }
  if (pin == P_CLK && v == 1 && old == 0 && lvl[P_CS] == 0) {
    edge++;
    if (edge <= 8) cmdByte = (cmdByte << 1) | lvl[P_MOSI];
    if (edge == 8) outBits = chan(cmdByte) & 0xFFF;
  }
  if (pin == P_CLK && v == 0 && old == 1 && lvl[P_CS] == 0) {
    if (edge >= 9 && edge <= 20) outBit = (outBits >> (20 - edge)) & 1; else outBit = 0;
  }
}
int digitalRead(int pin) {
  if (pin == P_IRQ) { simUpdate(); return penDown ? 0 : 1; }
  if (pin == P_MISO) return outBit;
  return 1;                                // BOOT ไม่กด
}

// ---------------- แบบจำลอง ESP-NOW ----------------
static esp_now_recv_cb_t g_recvCb = nullptr;
static struct_host_sync g_sent[256];
static int g_sentCount = 0;
static int g_initOk = 1;
WiFiClassStub WiFi;
int esp_now_init() { return g_initOk ? 0 : -1; }
int esp_now_deinit() { return 0; }
int esp_now_register_recv_cb(esp_now_recv_cb_t cb) { g_recvCb = cb; return 0; }
int esp_now_add_peer(const esp_now_peer_info_t *) { return 0; }
int esp_now_del_peer(const uint8_t *) { return 0; }
int esp_now_send(const uint8_t *, const uint8_t *d, int len) {
  if (len == (int)sizeof(struct_host_sync) && g_sentCount < 256) memcpy(&g_sent[g_sentCount++], d, len);
  return 0;
}
int esp_wifi_get_channel(uint8_t *ch, wifi_second_chan_t *) { *ch = 1; return 0; }
int esp_wifi_set_channel(uint8_t, wifi_second_chan_t) { return 0; }
int esp_wifi_set_ps(int) { return 0; }

#include "sketch.cpp"

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { printf("ล้มเหลว: %s (บรรทัด %d)\n", msg, __LINE__); fails++; } } while (0)

static void testCalMath() {
  // ความจริงสามแบบ: แกนตรง / สลับแกน / กลับด้านทั้งสองแกน
  for (int model = 0; model < 3; model++) {
    CalPoint p[3];
    touchCalTargets((int)W, (int)H, p);
    for (int i = 0; i < 3; i++) {
      float u = p[i].sx / W, v = p[i].sy / H;
      float rx, ry;
      if (model == 0) { rx = u * 3800 + 150; ry = v * 3700 + 200; }
      else if (model == 1) { rx = (1 - v) * 3800 + 150; ry = u * 3700 + 200; }
      else { rx = (1 - u) * 3800 + 150; ry = (1 - v) * 3700 + 200; }
      p[i].rx = rx; p[i].ry = ry;
    }
    TouchCal k;
    CHECK(touchCalSolve(p, (int)W, (int)H, k), "แก้สมการไม่ได้");
    float worst = 0;
    for (int t = 0; t < 50; t++) {
      float u = (t % 10) / 9.0f, v = (t / 10) / 4.0f, rx, ry;
      if (model == 0) { rx = u * 3800 + 150; ry = v * 3700 + 200; }
      else if (model == 1) { rx = (1 - v) * 3800 + 150; ry = u * 3700 + 200; }
      else { rx = (1 - u) * 3800 + 150; ry = (1 - v) * 3700 + 200; }
      int16_t sx, sy;
      touchCalApply(k, rx, ry, (int)W, (int)H, sx, sy);
      float dx = fabsf(sx - u * (W - 1)), dy = fabsf(sy - v * (H - 1));
      if (dx > worst) worst = dx;
      if (dy > worst) worst = dy;
    }
    CHECK(worst < 24.0f, "ความคลาดเคลื่อนเกินกำหนด (จุดนอกกรอบเป้าหมายถูกประมาณค่าเพิ่ม)");
    printf("  แบบจำลอง %d: ผิดพลาดสูงสุด %.1f พิกเซลทั่วทั้งจอ\n", model, worst);
  }
  // จุดสามจุดในแนวเดียวกัน ต้องถูกปฏิเสธ
  CalPoint bad[3] = {{500, 500, 48, 36}, {1500, 1500, 272, 48}, {2500, 2500, 160, 204}};
  TouchCal k;
  CHECK(!touchCalSolve(bad, (int)W, (int)H, k), "ต้องปฏิเสธจุดที่อยู่แนวเดียวกัน");
  // ค่าดิบนิ่งเกินไป (แตะที่เดิมสามครั้ง) ต้องถูกปฏิเสธ
  CalPoint same[3] = {{2000, 2000, 48, 36}, {2001, 2002, 272, 48}, {2002, 2001, 160, 204}};
  CHECK(!touchCalSolve(same, (int)W, (int)H, k), "ต้องปฏิเสธเมื่อค่าดิบแทบไม่ต่างกัน");
}

static void testMedianAndTracker() {
  int16_t a[5] = {900, 10, 12, 11, 4000};
  CHECK(touchMedian5(a) == 12, "ค่ากลางต้องตัดค่าโดดสองข้าง");
  TouchTracker t;
  uint32_t ms = 1000;
  CHECK(t.update(true, ms) == TOUCH_NONE, "การกดครั้งแรกยังไม่นับ");
  CHECK(t.update(true, ms += 4) == TOUCH_DOWN, "กดครั้งที่สองต้องเป็น DOWN");
  CHECK(t.update(true, ms += 4) == TOUCH_MOVE, "ลากต้องเป็น MOVE");
  CHECK(t.update(false, ms += 10) == TOUCH_NONE, "สะดุดสั้นไม่ใช่ UP");
  CHECK(t.update(true, ms += 10) == TOUCH_MOVE, "กลับมากดต่อได้เป็น MOVE ไม่เกิด DOWN ซ้ำ");
  CHECK(t.update(false, ms += 10) == TOUCH_NONE, "ยังไม่ครบเวลา");
  CHECK(t.update(false, ms += 60) == TOUCH_UP, "หายเกินเวลาต้องเป็น UP");
  CHECK(t.update(true, ms += 4) == TOUCH_NONE, "กดใหม่ต้องรอสองครั้งอีก");
  TouchTracker t2;                                     // สัญญาณกระตุกครั้งเดียวต้องไม่เกิดเหตุการณ์
  CHECK(t2.update(true, 0) == TOUCH_NONE && t2.update(false, 5) == TOUCH_NONE && t2.update(false, 100) == TOUCH_NONE, "สัญญาณกระตุกครั้งเดียวต้องถูกกรอง");
}


// ---------------- ทดสอบแกนเครือข่ายของสเก็ตช์จริง ----------------
static uint8_t g_mac[6] = {9, 8, 7, 6, 5, 4};
static void inject(const struct_message &m, int len = sizeof(struct_message), int rssi = -52) {
  uint8_t src[6]; memcpy(src, g_mac, 6);
  wifi_pkt_rx_ctrl_t rc = {rssi, 1};
  esp_now_recv_info_t info = { src, nullptr, &rc };
  g_recvCb(&info, (const uint8_t *)&m, len);
}
static void runMs(int ms) { for (int e = 0; e < ms; e += 5) { g_ms += 5; loop(); } }
static struct_message mk(int id, int run, uint32_t drops, float rate, uint32_t since) {
  struct_message m = {}; m.stationId = (uint8_t)id; m.isRunning = (uint8_t)run; m.totalDrops = drops;
  m.flowRateHr = rate; m.msSinceLastDrop = since; m.batteryVolts = 4.0f; return m;
}
static int syncsFor(int bed, int from) { int n = 0; for (int i = from; i < g_sentCount; i++) if (g_sent[i].stationId == bed) n++; return n; }

static void testNetwork() {
  Serial.println("");                                     // ให้ Serial จำลองถูกเรียกอย่างน้อยหนึ่งครั้ง
  handleLine((char *)"beds 5");
  handleLine((char *)"rate 1 60");
  handleLine((char *)"plan 1 500");
  g_sentCount = 0;
  // 1) ได้ยินเตียง 1 แล้ว ทุกเตียงที่เปิดใช้ต้องได้ Sync ภายในหนึ่งรอบ และธง HEARD เฉพาะเตียงที่ได้ยิน
  inject(mk(1, 1, 5, 60, 400));
  runMs(1300);
  for (int b = 1; b <= 5; b++) CHECK(syncsFor(b, 0) >= 1, "ทุกเตียงที่เปิดใช้ต้องได้ Sync ภายในหนึ่งรอบ");
  CHECK(syncsFor(6, 0) == 0, "เตียงที่ไม่ได้เปิดใช้ต้องไม่ได้ Sync");
  int s1 = -1, s2 = -1;
  for (int i = 0; i < g_sentCount; i++) { if (g_sent[i].stationId == 1) s1 = i; if (g_sent[i].stationId == 2) s2 = i; }
  CHECK(s1 >= 0 && g_sent[s1].targetRateHr == 60 && g_sent[s1].totalPlanMl == 500, "Sync เตียง 1 ต้องมีค่าตั้งที่สั่งทาง Serial");
  CHECK((g_sent[s1].flags & SYNC_FLAG_HEARD) && (g_sent[s1].flags & SYNC_FLAG_REPORTS), "เตียงที่ได้ยินต้องมีธง HEARD และ REPORTS");
  CHECK(s2 >= 0 && !(g_sent[s2].flags & SYNC_FLAG_HEARD), "เตียงที่ไม่เคยได้ยินต้องไม่มีธง HEARD");
  CHECK(g_sent[s1].hostChannel == 1, "Sync ต้องบอกช่องสัญญาณของ Host");
  // 2) สายพับ: รหัสเปลี่ยนต้องออกไปเร็วโดยไม่รอครบรอบ
  // รอจนเพิ่งส่ง Sync ของเตียง 1 ไป (รอบปกติจะวนกลับมาเตียง 1 อีกทีในราว 1 วินาที) แล้วค่อยเกิดสายพับ
  for (int guard = 0; guard < 400; guard++) { int c = g_sentCount; runMs(5); if (g_sentCount > c && g_sent[g_sentCount - 1].stationId == 1) break; }
  inject(mk(1, 1, 10, 60, 9000));
  int mark = g_sentCount;
  runMs(600);
  bool urgent = false;
  for (int i = mark; i < g_sentCount; i++) if (g_sent[i].stationId == 1 && g_sent[i].alertCode == ALERT_OCCLUSION) urgent = true;
  CHECK(urgent, "สายพับต้องส่งรหัส 5 ให้เตียงนั้นภายใน 600 ms (ช้ากว่านี้แปลว่ารอรอบปกติ ไม่ได้ส่งด่วน)");
  // 3) แพ็กเก็ตเสียต้องถูกนับและไม่กระทบเตียง
  uint32_t bad0 = badPackets, wl0 = rxWrongLen, ig0 = ignoredPackets;
  inject(mk(2, 1, 5, 5000, 400));                        // อัตราเกินจริง
  inject(mk(0, 1, 5, 50, 400));                          // เลขเตียง 0
  inject(mk(2, 1, 5, 50, 400), 10);                      // ความยาวผิด
  inject(mk(7, 1, 5, 50, 400));                          // เตียงที่ไม่ได้เปิดใช้
  runMs(50);
  CHECK(badPackets - bad0 == 2, "แพ็กเก็ตไม่สมเหตุสมผลสองใบต้องถูกนับ");
  CHECK(rxWrongLen - wl0 == 1, "ความยาวผิดต้องถูกนับ");
  CHECK(ignoredPackets - ig0 == 1, "เตียงที่ไม่ได้เปิดใช้ต้องถูกนับว่าเมิน");
  CHECK(!beds[1].seen, "แพ็กเก็ตเสียต้องไม่ทำให้เตียง 2 ขึ้นออนไลน์");
  // 4) เริ่มถุงใหม่ resetSeq ต้องเปลี่ยนใน Sync ถัดไป
  uint8_t seq0 = beds[0].cfg.resetSeq;
  handleLine((char *)"newbag 1");
  mark = g_sentCount;
  runMs(300);
  bool seqSeen = false;
  for (int i = mark; i < g_sentCount; i++) if (g_sent[i].stationId == 1 && g_sent[i].resetSeq == (uint8_t)(seq0 + 1)) seqSeen = true;
  CHECK(seqSeen, "newbag ต้องส่ง resetSeq ใหม่ให้เตียง 1 ทันที");
  // 5) คิวรับเต็มต้องไม่ล้ม และต้องนับที่ทิ้ง
  uint32_t drop0 = rxDropped;
  for (int i = 0; i < 40; i++) inject(mk(1, 1, 20 + i, 60, 300));
  CHECK(rxDropped - drop0 >= 24, "ใบที่ล้นคิวต้องถูกนับ ไม่ทำให้พัง");
  runMs(100);
  CHECK(beds[0].totalDrops >= 20, "ใบที่อยู่ในคิวต้องถูกประมวลผลครบ");
  handleLine((char *)"table");
  handleLine((char *)"bogus");
  printf("  แกนเครือข่าย: ส่ง Sync รวม %d ใบ\n", g_sentCount);
}

static void testDriverAndSketch() {
  g_ops = fopen("/dev/null", "w");
  touchChip.begin();
  penDown = false;
  simEnabled = false;
  int16_t x, y, z;
  CHECK(!touchChip.read(x, y, z), "ไม่แตะต้องไม่ขึ้นแตะ");
  // ให้ชิปจำลองแตะเป้าหมายของหน้าคาลิเบรตเอง แล้วเรียก setup() ของสเก็ตช์จริงทั้งไฟล์
  touchCalTargets((int)W, (int)H, targets);
  simT0 = g_ms; simEnabled = true;
  setup();
  CHECK(g_recvCb != nullptr, "setup() ต้องลงทะเบียน callback ESP-NOW");
  CHECK(!cal.valid, "บูตแล้วระบบเครือข่ายต้องทำงานเลย ไม่บังคับคาลิเบรตก่อน");
  setMode(MODE_DIAG);                                     // เข้าหน้าทดสอบ: ยังไม่มีค่าคาลิเบรต จึงต้องทำคาลิเบรตก่อน
  CHECK(cal.valid, "เข้าหน้าทดสอบแล้วต้องคาลิเบรตสำเร็จเมื่อยังไม่มีค่าในหน่วยความจำ");
  float worst = 0;
  for (int i = 0; i < 3; i++) {
    int rx, ry; truthRaw(targets[i].sx, targets[i].sy, rx, ry);
    int16_t sx, sy; touchCalApply(cal, (float)rx, (float)ry, (int)W, (int)H, sx, sy);
    worst = fmaxf(worst, fmaxf(fabsf(sx - targets[i].sx), fabsf(sy - targets[i].sy)));
  }
  CHECK(worst < 4.0f, "จุดเป้าหมายต้องแม่นภายใน 4 พิกเซล แม้ค่าดิบมีสัญญาณรบกวน");
  printf("  คาลิเบรตผ่านสเก็ตช์จริง ผิดพลาดที่จุดเป้าหมาย %.1f พิกเซล\n", worst);
  simEnabled = false;
  for (int i = 0; i < 200; i++) { g_ms += 5; loop(); }   // วนหลักไม่ค้าง ไม่มีนิ้วแตะ
  // แตะปุ่ม + ที่ตำแหน่งจริง ต้องเพิ่มความสว่าง ปุ่ม - ต้องลดลง
  int rx, ry;
  brightness = 50;
  truthRaw(buttons[3].x + buttons[3].w / 2, buttons[3].y + buttons[3].h / 2, rx, ry);
  penRx = rx; penRy = ry; holdPen = true;
  for (int i = 0; i < 12; i++) { g_ms += 5; loop(); }
  holdPen = false;
  for (int i = 0; i < 30; i++) { g_ms += 5; loop(); }
  CHECK(brightness == 60, "แตะปุ่ม + ต้องเพิ่มความสว่างเป็น 60");
  truthRaw(buttons[2].x + buttons[2].w / 2, buttons[2].y + buttons[2].h / 2, rx, ry);
  penRx = rx; penRy = ry; holdPen = true;
  for (int i = 0; i < 12; i++) { g_ms += 5; loop(); }
  holdPen = false;
  for (int i = 0; i < 30; i++) { g_ms += 5; loop(); }
  CHECK(brightness == 50, "แตะปุ่ม - ต้องลดความสว่างเป็น 50");
  setMode(MODE_STATUS);
  simEnabled = false;
  testNetwork();
  fclose(g_ops);
}

int main() {
  printf("ทดสอบคณิตศาสตร์คาลิเบรต\n");  testCalMath();
  printf("ทดสอบตัวกรองและตัวติดตามนิ้ว\n"); testMedianAndTracker();
  printf("ทดสอบไดรเวอร์และสเก็ตช์ (CYD_PANEL=%d)\n", CYD_PANEL); testDriverAndSketch();
  if (fails) { printf("รวมล้มเหลว %d รายการ\n", fails); return 1; }
  printf("ผ่านทั้งหมด\n");
  return 0;
}
