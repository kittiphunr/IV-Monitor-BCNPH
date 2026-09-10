// ทดสอบเฟิร์มแวร์ Host-Gateway บนเครื่อง PC (ไม่ต้องมีบอร์ด)
//   ./gw unit            ชุดทดสอบภายใน: แกะคำตอบพีซี ทำคำสั่ง ตรวจช่วงค่า คิวเต็ม ส่งซ้ำ ฯลฯ
//   ./gw emit  FILE      ป้อนแพ็กเก็ตเตียงเสมือนแล้วเขียน JSON ที่ Host จะส่งขึ้นพีซีลงไฟล์
//   ./gw apply FILE      อ่านคำตอบของพีซีจากไฟล์ แกะและทำคำสั่ง แล้วพิมพ์สถานะที่เปลี่ยนเป็น key=value
// สองโหมดหลังใช้คู่กับ e2e.py ซึ่งรันเซิร์ฟเวอร์ PC ตัวจริง จึงพิสูจน์ได้ว่าเฟิร์มแวร์กับเซิร์ฟเวอร์ตกลงกันตรงกัน
#include "Arduino.h"
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
unsigned long millis(){ return g_millis; }
unsigned long micros(){ return g_millis*1000UL; }
void delay(unsigned long ms){ g_millis += ms; }
void delayMicroseconds(unsigned long us){ g_millis += us/1000; }
void pinMode(int,int){} void digitalWrite(int,int){}
// ปุ่ม (GPIO 4) ตามสคริปต์เวลา: g_btnEv คือรายการ (เวลา, ระดับ) ที่เรียงตามเวลา ไม่มีสคริปต์ = ปล่อยปุ่ม (HIGH)
std::vector<std::pair<unsigned long,int>> g_btnEv;
int digitalRead(int pin) {
  if (pin != 4) return 1;
  int lvl = 1;
  for (auto& e : g_btnEv) { if (e.first <= g_millis) lvl = e.second; else break; }
  return lvl;
}
int analogRead(int){ return 2400; }
void analogReadResolution(int){} void analogSetAttenuation(adc_attenuation_t){}
void tone(int,unsigned int,unsigned long){} void noTone(int){}
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
int g_deepSleeps = 0; int g_wakeCause = 0;
void esp_deep_sleep_start(){ g_deepSleeps++; }
esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause(){ return (esp_sleep_wakeup_cause_t)g_wakeCause; }
int  esp_now_deinit(){ return 0; }
int  esp_wifi_set_ps(int){ return 0; }
bool ledcAttach(int,int,int) { return true; }
void ledcWrite(int,int) {}
#include "XPT2046_Touchscreen.h"
bool XPT2046_Touchscreen::g_touched = false;
TS_Point XPT2046_Touchscreen::g_point;

#include "gateway.cpp"   // สำเนาของ Host-Gateway.ino ที่ run.sh คัดลอกมาให้

static void sendFromStation(uint8_t id, uint32_t drops, float rate, const uint8_t mac[6], bool running = true) {
  struct_message m = {};
  m.stationId = id; m.isRunning = running ? 1 : 0; m.totalDrops = drops; m.periodDrops = 20;
  m.flowRateHr = rate; m.msSinceLastDrop = 900; m.batteryVolts = 3.95f; m.flags = 0;
  esp_now_recv_info_t info = {}; wifi_pkt_rx_ctrl_t ctrl = {}; ctrl.rssi = -62;
  info.rx_ctrl = &ctrl; info.src_addr = (uint8_t*)mac;
  OnDataRecv(&info, (const uint8_t*)&m, sizeof(m));
}

static int failures = 0;
static void check(bool ok, const char* what) {
  printf("  %-66s %s\n", what, ok ? "ผ่าน" : "ไม่ผ่าน");
  if (!ok) failures++;
}

static void resetWorld(int beds) {
  g_millis = 100000;
  activeStationCount = beds;
  cmdCount = 0; lastCmdEnqueued = 0; lastCmdApplied = 0; pendingEpoch = 0;
  pendingSyncMask = 0; syncRoundIdx = 0; lastSyncSlotTime = 0;
  for (int i = 0; i < MAX_SUPPORTED_STATIONS; i++) {
    stations[i] = StationData();
    stations[i].cfg.dropFactor = 20;
    stations[i].cfg.targetRateHr = 100;
    stations[i].cfg.planVolumeMl = 1000;
  }
}

static void feedBeds(int n, int rounds) {
  uint8_t mac[6] = {0x24, 0x6F, 0x28, 0x00, 0x00, 0};
  for (int r = 0; r < rounds; r++) {
    for (int i = 0; i < n; i++) {
      mac[5] = 0x10 + i; g_millis += 100;
      sendFromStation(i + 1, 100 + r * 20, 99.0f, mac);
    }
    g_millis += 50;
  }
}

static void setupIdentity() {
  hostId = "AABBCCDDEEFF";
  pcHost = "192.168.1.10"; pcPort = 8080; pcToken = "sekret"; pcConfigured = true;
}

// ---------------------------------------------------------------------------
static void unitTests() {
  g_ops = fopen("/dev/null", "w");   // ตัวจำลองจอเขียนคำสั่งวาดลงไฟล์นี้ การทดสอบนี้ไม่ใช้ภาพ
  printf("ขนาดโครงสร้าง: struct_message=%d, struct_host_sync=%d, StationData=%d ไบต์, stations[%d]=%.1f KB\n\n",
         (int)sizeof(struct_message), (int)sizeof(struct_host_sync), (int)sizeof(StationData),
         MAX_SUPPORTED_STATIONS, sizeof(stations) / 1024.0);
  check(sizeof(struct_message) == 23 && sizeof(struct_host_sync) == 20, "ขนาดแพ็กเก็ต Protocol v3 ไม่เปลี่ยน");

  // ---- 1) สถานะที่ส่งขึ้นพีซี ----
  printf("\n1) JSON ที่ส่งขึ้นพีซี\n");
  resetWorld(8); setupIdentity(); feedBeds(8, 3);
  strcpy(stations[2].cfg.patientName, "นาย \"ทดสอบ\"\\");
  String js = buildUplinkJson();
  printf("  ความยาว JSON 8 เตียง = %u ไบต์\n", js.length());
  check(js.length() < 3400, "ไม่เกินที่จองไว้ (ไม่ต้องขยายหน่วยความจำกลางทาง)");
  check(strstr(js.c_str(), "\"host\":\"AABBCCDDEEFF\"") != nullptr, "ระบุ Host ด้วย MAC");
  int count = 0; for (const char* p = js.c_str(); (p = strstr(p, "\"online\":")); p++) count++;
  check(count == 8, "มีสถานะครบทุกเตียง");
  check(strstr(js.c_str(), "\\\"ทดสอบ\\\"\\\\") != nullptr, "ชื่อผู้ป่วยที่มีเครื่องหมายคำพูดถูกหนีอักขระ");
  check(strstr(js.c_str(), "\"channel\":6") != nullptr, "รายงานช่องสัญญาณ ESP-NOW");

  // ---- 2) แกะคำตอบและทำคำสั่ง ----
  printf("\n2) คำสั่งจากพีซี\n");
  resetWorld(5); setupIdentity(); feedBeds(5, 3);
  parsePcReply(String("T 1800000000\nC 1 2 rate=80 vol=500 df=15 near=85 name=%E0%B8%AA%E0%B8%A1\n"));
  check(pendingEpoch == 1800000000UL, "แกะเวลาจากพีซี");
  check(cmdCount == 1, "คำสั่งเข้าคิว 1 รายการ");
  check(stations[1].cfg.targetRateHr == 100, "ยังไม่ทำจนกว่า loop() จะมาเรียก");
  serviceCommands();
  check(stations[1].cfg.targetRateHr == 80 && stations[1].cfg.planVolumeMl == 500, "ตั้งอัตรา/ปริมาตรแล้ว");
  check(stations[1].cfg.dropFactor == 15 && stations[1].nearEndPct == 85, "ตั้ง drop factor / near-end แล้ว");
  check(strcmp(stations[1].cfg.patientName, "สม") == 0, "ถอดรหัสชื่อ UTF-8 ถูกต้อง");
  check(lastCmdApplied == 1, "ack = 1");
  check(isTimeSynced, "นาฬิกาถูกตั้งจากพีซี");
  check((pendingEpoch == 0), "เวลาที่รอถูกใช้แล้ว");

  parsePcReply(String("C 1 2 rate=1\n"));
  check(cmdCount == 0, "คำสั่งที่เคยรับแล้วไม่เข้าคิวซ้ำ");
  parsePcReply(String("A 1 2\n"));
  check(cmdCount == 0, "เลขลำดับต่ำกว่าที่เคยรับไม่เข้าคิว");

  printf("\n3) การตรวจช่วงค่าที่ Host (ด่านสุดท้ายก่อนถึงเตียง)\n");
  parsePcReply(String("C 2 1 rate=5000 vol=99999 df=25 near=10\n"));
  serviceCommands();
  check(stations[0].cfg.targetRateHr == 100 && stations[0].cfg.planVolumeMl == 1000, "อัตรา/ปริมาตรเกินช่วงถูกปฏิเสธ");
  check(stations[0].cfg.dropFactor == 20 && stations[0].nearEndPct == DEFAULT_NEAR_END_PCT, "df/near เกินช่วงถูกปฏิเสธ");
  check(lastCmdApplied == 2, "คำสั่งที่ไม่ผ่านตรวจก็ต้อง ack ไม่งั้นพีซีส่งซ้ำไม่จบ");

  printf("\n4) เริ่มถุงใหม่ / รับทราบ / จำนวนเตียง\n");
  uint8_t seqBefore = stations[2].cfg.resetSeq;
  parsePcReply(String("R 3 3\nA 4 3\nN 5 0 count=6\n"));
  serviceCommands();
  check(stations[2].cfg.resetSeq == (uint8_t)(seqBefore + 1), "resetSeq เพิ่ม (Station จะรีเซ็ตตาม)");
  check(stations[2].totalDrops == 0 && stations[2].hasBaseline == false, "ตัวนับหยดที่ Host ถูกล้าง");
  check(stations[2].nearEndAck == true, "รับทราบเตือนแล้ว");
  check(activeStationCount == 6, "เปลี่ยนจำนวนเตียงเป็น 6");
  check(lastCmdApplied == 5, "ack = 5");
  parsePcReply(String("N 6 0 count=99\n"));
  serviceCommands();
  check(activeStationCount == 6, "จำนวนเตียงเกินช่วงถูกปฏิเสธ");

  printf("\n5) ข้อมูลขยะไม่ทำให้เครื่องล้ม\n");
  const char* junk[] = { "", "\n\n", "X 1 1", "C abc", "C 7", "C 8 99 rate=1", "C 9 -1 rate=1", "T x", "T 5",
                         "C 10 1 name=%", "C 11 1 name=%zz", "R 12 0", "C 0 1 rate=1", "\xff\xfe\n" };
  for (const char* j : junk) { parsePcReply(String(j)); serviceCommands(); }
  check(true, "แกะข้อความขยะทุกแบบได้ไม่ล้ม");
  check(stations[0].cfg.targetRateHr == 100, "ข้อมูลขยะไม่เปลี่ยนค่าเตียง");
  check(!(pendingEpoch != 0 && pendingEpoch < 1600000000UL), "เวลาที่ผิดปกติไม่ถูกรับ");

  printf("\n6) คิวเต็ม\n");
  resetWorld(5); setupIdentity();
  std::string big;
  for (int i = 1; i <= 20; i++) big += "A " + std::to_string(i) + " 1\n";
  parsePcReply(String(big.c_str()));
  check(cmdCount == CMD_QUEUE_LEN, "รับเข้าคิวได้เท่าความจุ");
  check(lastCmdEnqueued == (uint32_t)CMD_QUEUE_LEN, "ที่ล้นยังไม่นับว่ารับ (พีซีจะส่งซ้ำ)");
  serviceCommands();
  parsePcReply(String(big.c_str()));
  check(cmdCount == 20 - CMD_QUEUE_LEN, "รอบถัดไปรับส่วนที่เหลือ");
  serviceCommands();
  check(lastCmdApplied == 20, "ทำครบทุกคำสั่งตามลำดับ");

  // ---- 7) เส้นทางส่งจริงผ่านสตับ HTTP ----
  printf("\n7) pushToPc()\n");
  resetWorld(5); setupIdentity(); feedBeds(5, 2);
  g_httpReply = String("T 1800000000\nA 7 1\n"); g_httpCode = 200;
  check(pushToPc(), "พีซีตอบ 200 = สำเร็จ");
  check(g_httpLastUrl == String("http://192.168.1.10:8080/api/ingest"), "URL ถูกต้อง");
  check(g_httpLastToken == String("sekret"), "แนบรหัสเกตเวย์");
  check(cmdCount == 1, "คำตอบถูกแกะเป็นคำสั่ง");
  g_httpCode = 500; g_httpReply = String("A 8 1\n");
  check(!pushToPc(), "พีซีตอบ 500 = ล้มเหลว");
  check(cmdCount == 1, "คำตอบของคำขอที่ล้มเหลวไม่ถูกแกะ");
  serviceCommands();

  // ---- 8) งานเดิมของ Host ต้องไม่เสีย ----
  printf("\n8) ตรรกะเดิมของ Host ยังทำงาน\n");
  resetWorld(5); setupIdentity(); feedBeds(5, 3);
  int s0 = g_sendCount;
  for (int k = 0; k < 6; k++) { g_millis += 400; serviceSyncScheduler(); }
  check(g_sendCount > s0, "ยังส่ง Sync กลับไปให้เตียงตามช่องเวลา");
  struct_host_sync sy; memcpy(&sy, g_lastSent, sizeof(sy));
  check(sy.stationId >= 1 && sy.stationId <= 5 && sy.hostChannel == 6, "Sync บอกช่องสัญญาณของ Host");
  uint8_t mac[6] = {0x24,0x6F,0x28,0,0,0x11};
  g_millis += 20000;                                  // เงียบเกินเกณฑ์
  evaluateClinicalAlerts();
  check(!isStationOnline(0), "เตียงที่เงียบเกินเกณฑ์ถูกตัดเป็นออฟไลน์");
  sendFromStation(1, 500, 80.0f, mac);
  check(isStationOnline(0), "เตียงกลับมาออนไลน์เมื่อได้รับแพ็กเก็ต");
  strncpy(stations[0].cfg.patientName, "x", 2);

  // ---- 9) ข้อความสถานะบนจอ ----
  printf("\n9) ข้อความสถานะลิงก์บนจอ OLED\n");
  portalActive = false; staConfigured = true; pcConfigured = true; pcLinkUp = true;
  check(gatewayLine().length() <= 25, "ข้อความยาวไม่เกินความกว้างจอ (25 ตัวอักษร)");
  pcConfigured = false;
  check(gatewayLine() == String("NO PC SET") || gatewayLine() == String("NO ROUTER"), "ยังไม่ตั้งพีซีแสดงสถานะที่ถูกต้อง");
  printf("\nปุ่มเดียว (Host-Gateway จอ OLED)\n");
  {
    auto press = [&](unsigned long at, unsigned long dur) { g_btnEv.push_back({at, 0}); g_btnEv.push_back({at + dur, 1}); };
    auto run = [&](unsigned long untilMs) { while (g_millis < untilMs) { g_millis += 10; checkHostButton(); } };
    auto fresh = [&]() {
      resetWorld(5); g_btnEv.clear(); g_deepSleeps = 0; g_millis += 5000;
      currentOledPage = 0; oledDisplaySleeping = false; oledScreensaverActive = false;
      globalAlarmTriggered = false; globalSnoozeUntil = 0; isPowerOffProgressActive = false;
    };
    unsigned long t;

    fresh(); t = g_millis; press(t + 100, 100); run(t + 800);
    check(currentOledPage == 1, "กดสั้นครั้งเดียว -> หน้าถัดไป");

    fresh(); t = g_millis; press(t + 100, 100); press(t + 350, 100); run(t + 1200);
    check(oledScreensaverActive && currentOledPage == 0, "กดสองครั้ง -> โหมดนาฬิกา (หน้าไม่เลื่อน)");

    fresh(); t = g_millis; press(t + 100, 100); press(t + 350, 100); press(t + 600, 100); run(t + 1500);
    check(oledDisplaySleeping && !oledScreensaverActive, "กดสามครั้ง -> ดับจอ");
    t = g_millis; g_btnEv.clear(); press(t + 100, 100); run(t + 800);
    check(!oledDisplaySleeping && currentOledPage == 0, "จอดับอยู่ กดครั้งเดียว -> ปลุกจออย่างเดียว ไม่เลื่อนหน้า");

    fresh(); oledScreensaverActive = true; t = g_millis; press(t + 100, 100); run(t + 800);
    check(!oledScreensaverActive && currentOledPage == 0, "อยู่โหมดนาฬิกา กดครั้งเดียว -> กลับหน้าปกติอย่างเดียว ไม่เลื่อนหน้า");

    fresh(); globalAlarmTriggered = true; t = g_millis; press(t + 100, 100); run(t + 215);
    check(globalSnoozeUntil > g_millis, "มีเสียงเตือน กดสั้น -> พักเสียงทันที (ไม่รอช่องนับคลิก)");
    run(t + 1500);
    check(currentOledPage == 0, "กดพักเสียงแล้วไม่ถูกนับเป็นการเลื่อนหน้า");
    globalAlarmTriggered = true;
    check(globalSnoozeUntil > g_millis, "ระหว่างพักเสียง เสียงเตือนถูกพักอยู่");
    t = g_millis; g_btnEv.clear(); press(t + 100, 100); run(t + 800);
    check(currentOledPage == 1, "หลังพักเสียงแล้ว กดสั้นกลับไปเลื่อนหน้าได้ตามปกติ");

    fresh(); stations[0].active = true; stations[0].isRunning = true; stations[0].lastRecvTime = g_millis; stations[0].alertCode = ALERT_NEAR_END; stations[0].nearEndAck = false; stations[0].cfg.planVolumeMl = 1000;
    t = g_millis; press(t + 100, 100);
    check(anyNearEndPending(), "ก่อนกด: มีเตือนใกล้หมดค้างอยู่");
    run(t + 215);
    check(stations[0].nearEndAck && !anyNearEndPending(), "มีเตือนใกล้หมด กดสั้น -> รับทราบทันที");
    run(t + 1500);
    check(currentOledPage == 0, "กดรับทราบแล้วไม่ถูกนับเป็นการเลื่อนหน้า");

    fresh(); oledScreensaverActive = true; globalAlarmTriggered = true; t = g_millis; press(t + 100, 100); run(t + 215);
    check(globalSnoozeUntil > g_millis && !oledScreensaverActive, "มีเสียงเตือนขณะโหมดนาฬิกา กดสั้น -> พักเสียงและออกจากโหมดนาฬิกา");

    fresh(); t = g_millis; press(t + 100, 1000); run(t + 2500);
    check(g_deepSleeps == 0 && !isPowerOffProgressActive && currentOledPage == 0, "กดค้าง 1 วินาทีแล้วปล่อย -> ยกเลิกการปิดเครื่อง ไม่เลื่อนหน้า");

    fresh(); t = g_millis; press(t + 100, 420); run(t + 1500);
    check(g_deepSleeps == 0 && currentOledPage == 0 && !oledScreensaverActive, "กด 0.42 วินาที (ยาวกว่ากดสั้นแต่ไม่ถึงค้าง) -> ไม่ทำอะไร");

    fresh(); t = g_millis; press(t + 100, 2500); run(t + 4000);
    check(g_deepSleeps == 1, "กดค้าง 2 วินาทีขึ้นไป -> ปิดเครื่อง (เข้า deep sleep)");

    fresh(); t = g_millis; press(t + 100, 390); run(t + 1200);
    check(currentOledPage == 1, "กด 0.39 วินาที ยังนับเป็นกดสั้น");
  }

  printf("\nปุ่มเดียว: ล้างค่าเครือข่ายตอนเปิดเครื่อง (Host-Gateway จอ OLED)\n");
  {
    auto bootWith = [&](int wakeCause, unsigned long holdMs) {
      g_btnEv.clear(); g_millis = 0; g_prefsClears = 0; g_deepSleeps = 0; g_wakeCause = wakeCause;
      if (holdMs > 0) { g_btnEv.push_back({0, 0}); g_btnEv.push_back({holdMs, 1}); }
      setup();
    };
    bootWith(0, 0);
    check(g_prefsClears == 0, "เสียบไฟปกติ ไม่กดปุ่ม -> ไม่ล้างค่า");
    bootWith(0, 2000);
    check(g_prefsClears == 0, "เสียบไฟขณะกดปุ่มค้าง 2 วินาที -> ไม่ล้างค่า (กันเผลอ)");
    bootWith(0, 9000);
    check(g_prefsClears >= 1, "เสียบไฟขณะกดปุ่มค้าง 8 วินาที -> ล้างค่าเครือข่าย");
    bootWith(ESP_SLEEP_WAKEUP_EXT0, 2000);
    check(g_prefsClears == 0 && g_deepSleeps == 0, "ปลุกเครื่องด้วยการกดค้าง 2 วินาที -> เปิดเครื่องปกติ ไม่ล้างค่า");
    bootWith(ESP_SLEEP_WAKEUP_EXT0, 9000);
    check(g_prefsClears >= 1, "ปลุกเครื่องแล้วกดค้างต่อถึง 8 วินาที -> ล้างค่าเครือข่าย");
    bootWith(ESP_SLEEP_WAKEUP_EXT0, 600);
    check(g_deepSleeps >= 1 && g_prefsClears == 0, "ปลุกเครื่องด้วยการกดสั้น (ไม่ถึง 1.5 วินาที) -> กลับไปหลับ ไม่ล้างค่า");
    g_wakeCause = 0;
  }

}

// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
  const char* mode = argc > 1 ? argv[1] : "unit";
  if (!strcmp(mode, "unit")) {
    unitTests();
    printf("\n%s\n", failures ? "มีข้อที่ไม่ผ่าน" : "ผ่านทุกข้อ");
    return failures ? 1 : 0;
  }
  if (!strcmp(mode, "emit") && argc > 2) {
    resetWorld(3); setupIdentity(); feedBeds(3, 4);
    strcpy(stations[0].cfg.patientName, "ผู้ป่วย A");
    stations[0].cfg.targetRateHr = 60; stations[0].cfg.planVolumeMl = 500;
    g_millis += 10;
    FILE* f = fopen(argv[2], "wb"); String j = buildUplinkJson(); fwrite(j.c_str(), 1, j.length(), f); fclose(f);
    return 0;
  }
  if (!strcmp(mode, "apply") && argc > 2) {
    resetWorld(3); setupIdentity(); feedBeds(3, 4);
    FILE* f = fopen(argv[2], "rb"); std::string body; char b[256]; size_t n;
    while ((n = fread(b, 1, sizeof(b), f)) > 0) body.append(b, n);
    fclose(f);
    parsePcReply(String(body.c_str())); serviceCommands();
    printf("ack=%u\nrate1=%.1f\nvol1=%.0f\ndf1=%d\nnear1=%d\nname1=%s\nreset2=%d\nnearAck3=%d\ncount=%d\nsynced=%d\n",
           (unsigned)lastCmdApplied, stations[0].cfg.targetRateHr, stations[0].cfg.planVolumeMl,
           stations[0].cfg.dropFactor, stations[0].nearEndPct, stations[0].cfg.patientName,
           stations[1].cfg.resetSeq, stations[2].nearEndAck ? 1 : 0, activeStationCount, isTimeSynced ? 1 : 0);
    return 0;
  }
  fprintf(stderr, "usage: gw unit | emit FILE | apply FILE\n");
  return 2;
}
