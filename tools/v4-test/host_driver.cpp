// ทดสอบเฟิร์มแวร์ Host-Gateway บนเครื่อง PC (ไม่ต้องมีบอร์ด)
//   ./gw unit            ชุดทดสอบภายใน: แกะคำตอบพีซี ทำคำสั่ง ตรวจช่วงค่า คิวเต็ม ส่งซ้ำ ฯลฯ
//   ./gw emit  FILE      ป้อนแพ็กเก็ตเตียงเสมือนแล้วเขียน JSON ที่ Host จะส่งขึ้นพีซีลงไฟล์
//   ./gw apply FILE      อ่านคำตอบของพีซีจากไฟล์ แกะและทำคำสั่ง แล้วพิมพ์สถานะที่เปลี่ยนเป็น key=value
// สองโหมดหลังใช้คู่กับ e2e.py ซึ่งรันเซิร์ฟเวอร์ PC ตัวจริง จึงพิสูจน์ได้ว่าเฟิร์มแวร์กับเซิร์ฟเวอร์ตกลงกันตรงกัน
#include "Arduino.h"
#include "SPI.h"
#include "WiFi.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

FILE* g_ops = nullptr;
SerialClass Serial;
WiFiClassStub WiFi;
ESPClass ESP;
const uint8_t u8g2_font_4x6_tf[1]={0}, u8g2_font_5x8_tf[1]={0}, u8g2_font_6x10_tf[1]={0};
const uint8_t u8g2_font_7x13_tf[1]={0}, u8g2_font_7x14B_tf[1]={0}, u8g2_font_helvB10_tf[1]={0};
const uint8_t u8g2_font_helvB12_tf[1]={0}, u8g2_font_logisoso16_tf[1]={0}, u8g2_font_logisoso32_tf[1]={0};
unsigned long g_millis = 0;
unsigned long g_lowUntil = 0;   // ขา g_lowPin อ่านได้ LOW จนถึงเวลานี้ (จำลองการกดปุ่มค้าง)
unsigned long millis(){ return g_millis; }
unsigned long micros(){ return g_millis*1000UL; }
void delay(unsigned long ms){ g_millis += ms; }
void delayMicroseconds(unsigned long us){ g_millis += us/1000; }
void pinMode(int,int){} void digitalWrite(int,int){} int g_lowPin = -1; int digitalRead(int p){ return (p == g_lowPin && g_millis < g_lowUntil) ? 0 : 1; }
int analogRead(int){ return 2400; }
void analogReadResolution(int){} void analogSetAttenuation(adc_attenuation_t){}
int g_tones = 0; void tone(int,unsigned int,unsigned long){ g_tones++; } void noTone(int){}
void randomSeed(unsigned long){} long random(long m){return 0;} long random(long a,long){return a;}
void rgbLedWrite(int,int,int,int){} void neopixelWrite(int,int,int,int){}
long map(long x,long a,long b,long c,long d){return (x-a)*(d-c)/(b-a)+c;}

int  g_sendCount = 0;
uint8_t g_lastSent[64]; int g_lastSentLen = 0;
int esp_now_init(){return 0;}
int esp_now_register_recv_cb(esp_now_recv_cb_t){return 0;}
int esp_now_add_peer(const esp_now_peer_info_t*){return 0;}
int esp_now_del_peer(const uint8_t*){return 0;}
int esp_now_send(const uint8_t*,const uint8_t* d,int n){
  g_sendCount++;
  if (n > 0 && n <= (int)sizeof(g_lastSent)) { memcpy(g_lastSent, d, n); g_lastSentLen = n; }
  return 0;
}
int esp_wifi_set_channel(uint8_t,wifi_second_chan_t){return 0;}
int esp_wifi_get_channel(uint8_t* p,wifi_second_chan_t*){ *p=6; return 0; }
void emitMark(const char*){}
bool getLocalTime(struct tm* info, unsigned long){
  time_t t = previewNow(); struct tm* g = localtime(&t); if (!g) return false; *info = *g; return true;
}
void rtc_gpio_pullup_en(int){} void rtc_gpio_pulldown_dis(int){} void rtc_gpio_deinit(int){}
int  esp_sleep_enable_ext0_wakeup(int,int){ return 0; }
int g_wakeCause = 0;   // 0 = เปิดเครื่องปกติ (cold boot), ESP_SLEEP_WAKEUP_EXT0 = ปลุกจาก deep sleep
int g_deepSleepCalls = 0;
void esp_deep_sleep_start(){ g_deepSleepCalls++; }
esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause(){ return (esp_sleep_wakeup_cause_t)g_wakeCause; }
int  esp_now_deinit(){ return 0; }
int  esp_wifi_set_ps(int){ return 0; }
bool ledcAttach(int,int,int) { return true; }
void ledcWrite(int,int) {}
#include "XPT2046_Touchscreen.h"
bool XPT2046_Touchscreen::g_touched = false;
TS_Point XPT2046_Touchscreen::g_point;

#include "host_tg.cpp"   // สำเนาของ Host-TFT-Gateway.ino ที่ run_host.sh คัดลอกมาให้

static int failures = 0;
static void check(bool ok, const char* what) {
  printf("  %-72s %s\n", what, ok ? "ผ่าน" : "ไม่ผ่าน");
  if (!ok) failures++;
}

static uint8_t g_seq[9];
static void sendUp(uint8_t id, uint32_t drops, uint32_t since, uint8_t flags = V4_UP_RUNNING | V4_UP_SENSOR_SEEN,
                   uint16_t mv = 3950, const uint8_t* mac = nullptr) {
  struct_up_v4 u = {};
  u.magic = V4_MAGIC_UP; u.stationId = id; u.flags = flags; u.seq = g_seq[id]++;
  u.totalDrops = drops; u.msSinceLastDrop = since; u.batteryMv = mv;
  static const uint8_t m0[6] = {0x24,0x6F,0x28,0,0,0x10};
  esp_now_recv_info_t info = {}; wifi_pkt_rx_ctrl_t ctrl = {}; ctrl.rssi = -60;
  info.rx_ctrl = &ctrl; info.src_addr = (uint8_t*)(mac ? mac : m0);
  OnDataRecv(&info, (const uint8_t*)&u, sizeof(u));
}

// จำลองสถานีหนึ่งตัวที่หยดตามอัตรา mlHr แล้วส่งแพ็กเก็ตวินาทีละใบเข้า Host
struct SimBed {
  uint8_t id; double mlHr; int df = 20;
  uint32_t drops = 0, lastDrop = 0; bool seen = false; double next = 0; bool running = true;
  void secs(int n) {
    double iv = 3600000.0 / (mlHr * df);
    if (next == 0) next = g_millis + iv;
    for (int s = 0; s < n; s++) {
      unsigned long end = g_millis + 1000;
      while (running && mlHr > 0 && next <= end) { drops++; lastDrop = (uint32_t)next; seen = true; next += iv; }
      g_millis = end;
      sendUp(id, drops, seen ? (uint32_t)(g_millis - lastDrop) + (seen && g_millis == lastDrop ? 1 : 0) : 0,
             running ? (V4_UP_RUNNING | V4_UP_SENSOR_SEEN) : 0);
      evaluateClinicalAlerts();
    }
  }
};

static void resetWorld(int beds) {
  g_millis = 100000;
  activeStationCount = beds;
  cmdCount = 0; lastCmdEnqueued = 0; lastCmdApplied = 0; pendingEpoch = 0;
  pendingSyncMask = 0; syncRoundIdx = 0; lastSyncSlotTime = 0; globalSnoozeUntil = 0;
  for (int i = 0; i < MAX_SUPPORTED_STATIONS; i++) {
    stations[i] = StationData();
    stations[i].cfg.dropFactor = 20;
    stations[i].cfg.targetRateHr = 100;
    stations[i].cfg.planVolumeMl = 1000;
    g_seq[i + 1] = 0;
  }
  hostId = "AABBCCDDEEFF"; pcHost = "192.168.1.10"; pcPort = 8080; pcToken = "k"; pcConfigured = true;
}

// เก็บแพ็กเก็ตลงที่ Host ส่งออกไป (esp_now_send ใน driver เก็บล่าสุดไว้ ที่นี่เรียกตัวจัดคิวทีละช่องเวลา)
static std::vector<struct_down_v4> pumpDown(int ms) {
  std::vector<struct_down_v4> out;
  for (int t = 0; t < ms; t += 10) {
    g_millis += 10;
    int before = g_sendCount;
    serviceSyncScheduler();
    if (g_sendCount != before && g_lastSentLen == (int)sizeof(struct_down_v4)) {
      struct_down_v4 d; memcpy(&d, g_lastSent, sizeof(d)); out.push_back(d);
    }
  }
  return out;
}


// ---- โหมดจำลองต่อเนื่อง (ใช้กับ closed_loop.py) ----
static void hexOut(const char* tag, const uint8_t* p, int n) {
  printf("%s ", tag); for (int i = 0; i < n; i++) printf("%02x", p[i]); printf("\n");
}
static int simMain() {
  resetWorld(2);
  g_millis = 50000;
  stations[0].cfg.targetRateHr = 100; stations[0].cfg.planVolumeMl = 1000;
  char line[1024];
  int lastSend = g_sendCount;
  while (fgets(line, sizeof(line), stdin)) {
    if (line[0] == 'T') {
      g_millis = strtoul(line + 2, 0, 10);
      evaluateClinicalAlerts();
      // loop() ตัวจริงมีงานอื่นปนด้วย (ปุ่ม จอ ฯลฯ) ที่นี่เรียกเฉพาะตัวจัดคิวส่งกับงานรายวินาทีที่เกี่ยวกับการคำนวณ
      serviceSyncScheduler(); serviceLinkQuality(); serviceCommands();
      if (g_sendCount != lastSend) { if (g_lastSentLen == (int)sizeof(struct_down_v4)) hexOut("S", g_lastSent, g_lastSentLen); lastSend = g_sendCount; }
      printf("OK\n");
    }
    else if (line[0] == 'U') {   // U <hex> <rxms> : แพ็กเก็ตขึ้นจาก Station
      uint8_t buf[64]; int n = 0; const char* p = line + 2;
      while (p[0] && p[1] && p[0] != ' ' && p[0] != '\n') { unsigned v; sscanf(p, "%2x", &v); buf[n++] = (uint8_t)v; p += 2; }
      esp_now_recv_info_t info = {}; wifi_pkt_rx_ctrl_t ctrl = {}; ctrl.rssi = -60; info.rx_ctrl = &ctrl;
      static const uint8_t mac[6] = {0x24,0x6F,0x28,0,0,0x10}; info.src_addr = (uint8_t*)mac;
      OnDataRecv(&info, buf, n);
    }
    else if (line[0] == 'J') { String j = buildUplinkJson(); printf("J %s\n", j.c_str()); }
    else if (line[0] == 'R') { parsePcReply(String(line + 2)); }
    else if (line[0] == 'Q') {
      printf("STATE rate=%.2f vol=%.2f alert=%d online=%d ack=%u rateValid=%d drops=%u reset=%d\n",
             stations[0].flowRate_ml_hr, stations[0].totalVolumeMl, (int)stations[0].alertCode,
             isStationOnline(0) ? 1 : 0, (unsigned)lastCmdApplied, stations[0].rateValid ? 1 : 0,
             (unsigned)stations[0].totalDrops, (int)stations[0].cfg.resetSeq);
    }
    else if (line[0] == 'X') break;
    fflush(stdout);
  }
  return 0;
}

// ---------------------------------------------------------------------------
// [สุนัขเฝ้าบ้าน] setup() ต้องสมัครสุนัขเฝ้าบ้านทุกครั้งที่เปิดเครื่อง ทั้ง cold boot และปลุกจาก deep sleep
// เดิมสามบรรทัด (bootResetReason / Serial.printf / setupLoopWatchdog) ตกอยู่ในบล็อก if ของการปลุก
// จึงทำงานเฉพาะตอนปลุก ตอนเสียบไฟปกติไม่มีสุนัขเฝ้าบ้านเลย
static void watchdogTests() {
  printf("\nสุนัขเฝ้าบ้านถูกสมัครทุกครั้งที่เปิดเครื่อง (setup)\n");
  g_lowPin = POWER_BTN_PIN;

  // cold boot: ไม่มีสาเหตุการปลุก
  wdtStatsReset(); g_wakeCause = 0; g_millis = 100000; g_lowUntil = 0; g_deepSleepCalls = 0;
  bootResetReason = ESP_RST_UNKNOWN;
  setup();
  check(wdtStats().adds == 1, "cold boot: esp_task_wdt_add ถูกเรียกครั้งเดียว");
  check(wdtStats().inits == 1, "cold boot: ตั้งค่าสุนัขเฝ้าบ้าน (esp_task_wdt_init) ครั้งเดียว");
  check(bootResetReason == ESP_RST_POWERON, "cold boot: บันทึกสาเหตุการรีบูตแล้ว");
  check(wdtMaxGapNow() < (unsigned long)LOOP_WDT_TIMEOUT_S * 1000UL, "cold boot: setup() ไม่ปล่อยเกิน LOOP_WDT_TIMEOUT_S โดยไม่ป้อน");
  check(g_deepSleepCalls == 0, "cold boot: ไม่เข้า deep sleep");

  // เส้นทางปิดเครื่อง ต้องเลิกเฝ้าและเข้า deep sleep ได้เหมือนเดิม
  enterDeepSleepWaitPowerButton();
  check(wdtStats().deletes == 1 && !wdtStats().registered, "ปิดเครื่อง: stopLoopWatchdog() ถอนสุนัขเฝ้าบ้านแล้ว");
  check(g_deepSleepCalls == 1, "ปิดเครื่อง: เข้า deep sleep");

  // ปลุกจาก deep sleep ด้วยการกดปุ่มค้างเกิน 1.5 วินาที
  wdtStatsReset(); g_wakeCause = ESP_SLEEP_WAKEUP_EXT0; g_millis = 200000; g_lowUntil = g_millis + 2000; g_deepSleepCalls = 0;
  bootResetReason = ESP_RST_UNKNOWN;
  setup();
  check(wdtStats().adds == 1, "ปลุก (EXT0): esp_task_wdt_add ถูกเรียกครั้งเดียว");
  check(wdtStats().inits == 1, "ปลุก (EXT0): ตั้งค่าสุนัขเฝ้าบ้านครั้งเดียว");
  check(bootResetReason == ESP_RST_POWERON, "ปลุก (EXT0): บันทึกสาเหตุการรีบูตแล้ว");
  check(wdtMaxGapNow() < (unsigned long)LOOP_WDT_TIMEOUT_S * 1000UL, "ปลุก (EXT0): ลูปรอปุ่มและ setup() ป้อนสุนัขทันเวลา");
  check(g_deepSleepCalls == 0, "ปลุก (EXT0): กดค้างพอ จึงไม่กลับไปหลับ");

  g_wakeCause = 0; g_lowPin = -1;
}

int main(int argc, char** argv) {
  if (argc > 1 && !strcmp(argv[1], "sim")) return simMain();
  g_ops = fopen("/dev/null", "w");   // ตัวจำลองจอ (Adafruit_GFX) เขียนคำสั่งวาดลงไฟล์นี้ ทดสอบนี้ไม่ใช้ภาพ
  printf("ขนาด: ขึ้น %d ลง %d ไบต์, StationData=%d ไบต์, stations[%d]=%.1f KB\n",
         (int)sizeof(struct_up_v4), (int)sizeof(struct_down_v4), (int)sizeof(StationData),
         MAX_SUPPORTED_STATIONS, sizeof(stations) / 1024.0);

  printf("\n1) รับ Protocol v4 และคำนวณอัตราไหล\n");
  resetWorld(5);
  SimBed b1{1, 100}; b1.secs(60);
  check(isStationOnline(0), "เตียงออนไลน์หลังรับแพ็กเก็ต");
  check(stations[0].rateValid && fabsf(stations[0].flowRate_ml_hr - 100.0f) < 3.0f, "อัตราไหล 100 mL/h คำนวณได้ถูก (ผิดไม่เกิน 3)");
  check(fabsf(stations[0].totalVolumeMl - b1.drops / 20.0f) < 0.01f, "ปริมาตร = หยดสะสม / drop factor");
  check(stations[0].alertCode == ALERT_NONE, "อัตราตรงเป้า = ไม่เตือน");
  check(fabsf(stations[0].batteryVolts - 3.95f) < 0.01f, "แบตเตอรี่แปลงจากมิลลิโวลต์ถูก");
  check(stations[0].macKnown && stations[0].rxTotal == 60, "นับแพ็กเก็ตที่รับ");

  printf("\n2) ตัดสินเตือนจากอัตราที่ Host คำนวณเอง (ตรรกะเตือนเดิมไม่เปลี่ยน)\n");
  resetWorld(5);
  { SimBed b{1, 160}; b.secs(60); check(stations[0].alertCode == ALERT_TOO_FAST, "160 mL/h เป้า 100 -> TOO FAST"); }
  resetWorld(5);
  { SimBed b{1, 50}; b.secs(120); check(stations[0].alertCode == ALERT_TOO_SLOW, "50 mL/h เป้า 100 -> TOO SLOW"); }
  resetWorld(5);
  { SimBed b{1, 100}; b.secs(40); b.mlHr = 0; b.next = 0; b.secs(30);
    check(stations[0].alertCode == ALERT_OCCLUSION, "หยดหยุดเกินเกณฑ์ -> OCCLUSION");
  }
  resetWorld(5);
  { SimBed b{1, 100}; b.secs(40); b.running = false; b.secs(30);
    check(stations[0].alertCode == ALERT_NONE && stations[0].flowRate_ml_hr == 0, "หยุดชั่วคราว -> ไม่เตือน อัตรา 0"); }
  resetWorld(5);
  { stations[0].cfg.planVolumeMl = 6; stations[0].cfg.targetRateHr = 300; SimBed b{1, 300}; b.secs(90);   // 300 mL/h = 5 mL/นาที ครบ 6 mL ภายใน 90 วินาที
    check(stations[0].alertCode == ALERT_COMPLETE, "ปริมาตรถึงแผน -> BAG EMPTY (COMPLETE)"); }

  printf("\n3) ส่งผลกลับให้ Station (Protocol v4 ลง)\n");
  resetWorld(5);
  SimBed s1{1, 100}, s2{2, 160};
  for (int i = 0; i < 60; i++) { s1.secs(1); g_millis -= 1000; s2.secs(1); }
  s1.next = s2.next = 0;
  stations[0].cfg.targetRateHr = 100; stations[1].cfg.targetRateHr = 100;
  stations[2].lastRecvTime = 0;
  std::vector<struct_down_v4> sent = pumpDown(2000);
  int per[9] = {0};
  for (auto& d : sent) per[d.stationId]++;
  check(per[1] >= 1 && per[2] >= 1 && per[3] >= 1 && per[4] >= 1 && per[5] >= 1, "ทุกเตียงได้รับแพ็กเก็ตลง");
  check(per[1] <= 3, "ไม่ส่งถี่เกินราวหนึ่งใบต่อวินาทีต่อเตียง");
  struct_down_v4 d1 = {}, d2 = {};
  for (auto& d : sent) { if (d.stationId == 1) d1 = d; if (d.stationId == 2) d2 = d; }
  check(d1.magic == V4_MAGIC_DOWN, "รหัสนำหน้าถูก");
  check(fabsf(d1.rateHr - stations[0].flowRate_ml_hr) < 0.01f && d1.rateHr > 90, "อัตราไหลที่ส่งลง = ที่ Host คำนวณ");
  check(fabsf(d1.infusedMl - stations[0].totalVolumeMl) < 0.01f, "ปริมาตรที่ส่งลง = ที่ Host คำนวณ");
  check(d1.planMl == 1000 && d1.targetRateHr == 100 && d1.dropFactor == 20 && d1.nearEndPct == 80, "ค่าตั้งเตียงครบ");
  check((d1.flags & V4_DOWN_HEARS_YOU) && (d1.flags & V4_DOWN_PLAN_SET) && (d1.flags & V4_DOWN_RATE_OK), "ธง ได้ยินเตียง/มีแผน/อัตราใช้ได้");
  check(d1.hostChannel == 6, "บอกช่องสัญญาณ ESP-NOW");
  check(d1.minutesLeft != V4_MINUTES_UNKNOWN && d1.infusedPct <= 100, "นาทีที่เหลือและ % มีค่า");
  { int expect = bedMinutesLeft(0); check(d1.minutesLeft == (uint16_t)expect, "นาทีที่เหลือตรงกับที่ Host คำนวณ"); }
  check(d2.alertCode == ALERT_TOO_FAST, "รหัสเตือนส่งลงไปให้เตียงที่ผิดปกติ");
  struct_down_v4 d3 = {}; for (auto& d : sent) if (d.stationId == 3) d3 = d;
  check(!(d3.flags & V4_DOWN_HEARS_YOU) && d3.rateHr == 0, "เตียงที่ไม่เคยได้ยิน: ไม่มีธงได้ยิน อัตรา 0");
  stations[3].alertCode = ALERT_LOST_LINK;
  { struct_down_v4 dd = {}; for (int k = 0; k < 6; k++) for (auto& d : pumpDown(300)) if (d.stationId == 4) dd = d;
    check(dd.alertCode == ALERT_NONE, "รหัสภายใน Host (เตียงหาย) ไม่หลุดไปถึง Station"); }

  printf("\n4) กรองแพ็กเก็ตที่ไม่ใช่ v4\n");
  resetWorld(5);
  { uint32_t rx0 = stations[0].rxTotal;
    uint8_t v3[23]; memset(v3, 0, sizeof(v3)); v3[0] = 1; v3[1] = 1;
    esp_now_recv_info_t info = {}; wifi_pkt_rx_ctrl_t ctrl = {}; ctrl.rssi = -60; info.rx_ctrl = &ctrl;
    OnDataRecv(&info, v3, sizeof(v3));
    check(stations[0].rxTotal == rx0, "แพ็กเก็ต v3 (23 ไบต์) ถูกทิ้ง");
    struct_up_v4 u = {}; u.magic = 0x11; u.stationId = 1; u.totalDrops = 5; u.flags = V4_UP_RUNNING;
    OnDataRecv(&info, (uint8_t*)&u, sizeof(u));
    check(stations[0].rxTotal == rx0, "รหัสนำหน้าผิดถูกทิ้ง");
    u.magic = V4_MAGIC_UP; u.stationId = 9; OnDataRecv(&info, (uint8_t*)&u, sizeof(u));
    u.stationId = 0; OnDataRecv(&info, (uint8_t*)&u, sizeof(u));
    check(stations[0].rxTotal == rx0, "เลขเตียงนอกช่วงถูกทิ้ง");
    u.stationId = 1; u.flags = 0xF1; OnDataRecv(&info, (uint8_t*)&u, sizeof(u));
    check(stations[0].rxTotal == rx0, "ธงที่ใช้บิตสำรองถูกทิ้ง");
    u.flags = V4_UP_RUNNING; u.batteryMv = 9000; OnDataRecv(&info, (uint8_t*)&u, sizeof(u));
    check(stations[0].rxTotal == rx0, "แบตเตอรี่เกินจริงถูกทิ้ง");
    u.batteryMv = 3900; u.totalDrops = 0; u.msSinceLastDrop = 500; OnDataRecv(&info, (uint8_t*)&u, sizeof(u));
    check(stations[0].rxTotal == rx0, "มีเวลาจากหยดล่าสุดทั้งที่ไม่เคยมีหยดถูกทิ้ง");
    u.msSinceLastDrop = 0; OnDataRecv(&info, (uint8_t*)&u, sizeof(u));
    check(stations[0].rxTotal == rx0 + 1, "แพ็กเก็ตที่ถูกต้องผ่าน"); }

  printf("\n5) คำสั่งจากพีซีส่งผลถึง Station\n");
  resetWorld(5);
  { SimBed b{1, 100}; b.secs(30);
    uint8_t seq0 = stations[0].cfg.resetSeq;
    parsePcReply(String("C 1 1 rate=80 vol=600 df=15 near=90\nR 2 1\n"));
    serviceCommands();
    check(stations[0].cfg.targetRateHr == 80 && stations[0].cfg.planVolumeMl == 600 && stations[0].cfg.dropFactor == 15, "ตั้งค่าเตียงตามคำสั่ง");
    check(stations[0].cfg.resetSeq == (uint8_t)(seq0 + 1) && stations[0].totalDrops == 0, "เริ่มถุงใหม่: resetSeq เพิ่ม ล้างหยดและตัวคำนวณ");
    check(stations[0].est.n == 0 && !stations[0].rateValid, "ตัวคำนวณอัตราไหลถูกล้าง");
    struct_down_v4 d = {}; for (int k = 0; k < 8; k++) for (auto& x : pumpDown(200)) if (x.stationId == 1) d = x;
    check(d.resetSeq == stations[0].cfg.resetSeq && d.targetRateHr == 80, "แพ็กเก็ตลงถัดไปมี resetSeq ใหม่และค่าตั้งใหม่ (Station จะล้างยอดตาม)");
    check(lastCmdApplied == 2, "ack = 2"); }

  printf("\n6) JSON ขึ้นพีซีและการส่ง\n");
  resetWorld(3);
  { SimBed b{1, 100}; b.secs(30); strcpy(stations[0].cfg.patientName, "ทดสอบ");
    String js = buildUplinkJson();
    check(js.length() > 400 && js.length() < 3400, "ขนาด JSON อยู่ในเกณฑ์");
    check(strstr(js.c_str(), "\"host\":\"AABBCCDDEEFF\"") && strstr(js.c_str(), "\"flowRateHr\":10"), "มีรหัส Host และอัตราไหลที่คำนวณ");
    g_httpReply = String("T 1800000000\n"); g_httpCode = 200;
    check(pushToPc(), "pushToPc สำเร็จเมื่อพีซีตอบ 200"); }

  printf("\n7) นาฬิกา\n");
  resetWorld(3);
  { parsePcReply(String("T 1800000000\n")); serviceCommands();
    check(isTimeSynced, "ตั้งเวลาจากพีซี");
    check(!isTimeApprox, "เวลาที่ตั้งจากพีซีไม่ถูกทำเครื่องหมายว่าโดยประมาณ"); }

  printf("\n8) ข้อความสถานะ\n");
  portalActive = false; staConfigured = true; pcConfigured = true; pcLinkUp = true;
  check(gatewayLine().length() <= 26, "ข้อความสถานะลิงก์ยาวไม่เกินความกว้างจอ");


  printf("\n9) ตรวจระดับแบตเตอรี่ (Host เอง และทุกเตียง)\n");
  {
    auto chime = [&](unsigned long ms) {
      int before = g_tones;
      for (unsigned long t = 0; t < ms; t += 10) { g_millis += 10; handleBatteryChime(); }
      return g_tones - before;
    };
    auto reset = [&]() {
      resetWorld(3); hostBat = BatteryMonitor(); hostBatteryVolts = 4.1f; hostBatteryPct = 90;
      batChimeRemaining = 0; batChimeNextRound = 0; nearChimeRemaining = 0; globalAlarmTriggered = false;
      globalSnoozeUntil = 0; cacheBottom = "";
    };   // hostBat เริ่มที่ BAT_NONE การอ่านครั้งแรกจึงตัดสินทันที (ตรงกับการเปิดเครื่องจริง)

    // เตียง: ตัดสินจากแรงดันในแพ็กเก็ตขึ้น
    reset();
    sendUp(1, 10, 100, V4_UP_RUNNING, 3400); sendUp(2, 10, 100, V4_UP_RUNNING, 3900); sendUp(3, 10, 100, V4_UP_RUNNING, 0);
    check(stations[0].bat.level == BAT_LOW, "เตียงที่ส่ง 3.40 V -> ต่ำ ทันที");
    check(stations[1].bat.level == BAT_OK, "เตียงที่ส่ง 3.90 V -> ปกติ");
    check(stations[2].bat.level == BAT_NONE, "เตียงที่ส่ง 0 mV (ไม่มีวงจรวัด) -> ไม่ตรวจ");
    for (int i = 0; i < 31; i++) { g_millis += 1000; sendUp(2, 10 + i, 100, V4_UP_RUNNING, 3300); }
    check(stations[1].bat.level == BAT_CRIT, "เตียงที่ต่ำกว่า 3.35 V ต่อเนื่อง 30 วินาที -> วิกฤต");
    check(worstBatteryLevel() == BAT_CRIT, "ระดับแย่สุดคือวิกฤต");

    // เตียงออฟไลน์ไม่นับ
    reset();
    sendUp(1, 10, 100, V4_UP_RUNNING, 3300);
    check(worstBatteryLevel() == BAT_CRIT, "เตียงออนไลน์ที่แบตวิกฤตนับ");
    g_millis += ONLINE_TIMEOUT_MS + 2000;
    check(worstBatteryLevel() == BAT_NONE, "เตียงออฟไลน์แล้วไม่นับ (ระดับเก่า)");

    // Host เอง
    reset();
    hostBatteryVolts = 3.4f; hostBatteryPct = calculateBatteryPct(hostBatteryVolts); serviceHostBattery(g_millis);
    check(hostBat.level == BAT_LOW, "แบตเตอรี่ Host 3.40 V -> ต่ำ");
    check(chime(3000) == 2, "ต่ำ: เสียงสองครั้ง");
    check(chime(290000) == 0, "ต่ำ: เงียบระหว่าง 5 นาที");
    check(chime(20000) == 2, "ต่ำ: รอบถัดไปหลังครบ 5 นาที");
    hostBatteryVolts = 0.0f; serviceHostBattery(g_millis);
    check(hostBat.level == BAT_NONE && chime(60000) == 0, "Host ใช้ไฟ USB (0 V) -> ล้างการเตือน เงียบ");

    // ไม่แย่งเสียงสำคัญกว่า / พักเสียงได้
    reset(); hostBatteryVolts = 3.3f; serviceHostBattery(g_millis);
    check(hostBat.level == BAT_CRIT, "Host 3.30 V -> วิกฤต");
    globalAlarmTriggered = true;
    check(chime(5000) == 0, "มีเหตุวิกฤตทางคลินิกที่ยังไม่พักเสียง -> เงียบ");
    globalAlarmTriggered = false; batChimeNextRound = 0;
    nearChimeRemaining = 2;
    check(chime(2000) == 0, "กำลังเตือนใกล้หมด -> เงียบ");
    nearChimeRemaining = 0; batChimeNextRound = 0;
    globalSnoozeUntil = g_millis + 120000;
    check(chime(5000) == 0, "พักเสียงอยู่ -> เงียบ (พักด้วยปุ่มเดียวกับเตือนปกติ)");
    globalSnoozeUntil = 0; batChimeNextRound = 0;
    check(chime(3000) == 3, "วิกฤต: เสียงสามครั้ง");
    check(chime(55000) == 0, "วิกฤต: เงียบระหว่าง 1 นาที");
    check(chime(8000) == 3, "วิกฤต: รอบถัดไปหลังครบ 1 นาที");
    // แย่ลงจากต่ำเป็นวิกฤต: เตือนวิกฤตทันที ไม่รอรอบ 5 นาทีของระดับต่ำ
    reset(); hostBatteryVolts = 3.4f; serviceHostBattery(g_millis);
    chime(3000);
    hostBatteryVolts = 3.3f; for (int i = 0; i < 31; i++) { g_millis += 1000; serviceHostBattery(g_millis); }
    check(hostBat.level == BAT_CRIT, "Host แย่ลงจากต่ำเป็นวิกฤต");
    check(chime(3000) == 3, "ต่ำ -> วิกฤต: เตือนวิกฤตทันที ไม่รอรอบ 5 นาทีของระดับต่ำ");

    // แถบล่าง
    reset();
    sendUp(1, 10, 100, V4_UP_RUNNING, 3400);
    updateBottomBar();
    check(cacheBottom == String("BED 01  LOW BATT  20%"), "แถบล่างบอกเตียงที่แบตต่ำ + เปอร์เซ็นต์");
    reset(); hostBatteryVolts = 3.3f; hostBatteryPct = calculateBatteryPct(hostBatteryVolts); serviceHostBattery(g_millis);
    updateBottomBar();
    check(strncmp(cacheBottom.c_str(), "HOST  BATT CRIT", 15) == 0, "แถบล่างบอกว่า Host แบตวิกฤต");
    reset(); sendUp(1, 10, 100, V4_UP_RUNNING, 3400);
    stations[1].alertCode = ALERT_OCCLUSION; stations[1].active = true; stations[1].lastRecvTime = g_millis; stations[1].isRunning = true;
    cacheBottom = ""; updateBottomBar();
    check(cacheBottom[0] == '!' , "เหตุวิกฤตทางคลินิกมาก่อนแบตต่ำในแถบล่าง");
    reset(); sendUp(1, 10, 100, V4_UP_RUNNING, 3400); stations[2].alertCode = ALERT_NEAR_END; stations[2].lastRecvTime = g_millis;
    cacheBottom = ""; updateBottomBar();
    check(strstr(cacheBottom.c_str(), "PREPARE NEXT BAG") != nullptr, "เตือนใกล้หมดมาก่อนแบตต่ำในแถบล่าง");

    // ขึ้นพีซี
    reset(); hostBatteryVolts = 3.4f; serviceHostBattery(g_millis);
    sendUp(1, 10, 100, V4_UP_RUNNING, 3300);
    String js = buildUplinkJson();
    check(strstr(js.c_str(), "\"batLevel\":2,\"linkWeakPct\"") != nullptr || strstr(js.c_str(), "\"batLevel\":2,") != nullptr, "JSON มีระดับแบตเตอรี่ของ Host");
    check(strstr(js.c_str(), "\"battery\":3.30,\"batLevel\":3") != nullptr, "JSON มีระดับแบตเตอรี่ของเตียง");
  }

  watchdogTests();
  printf("\n%s\n", failures ? "มีข้อที่ไม่ผ่าน" : "ผ่านทุกข้อ");
  return failures ? 1 : 0;
}
