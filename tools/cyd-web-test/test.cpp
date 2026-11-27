// ทดสอบด่านที่ 4 ของ Host-CYD28 บน PC: API เว็บ (ล็อกอิน ตรวจค่า ผลข้างเคียง) ประวัติเหตุการณ์ นาฬิกา และ CSV
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>
#include <vector>
#include <map>
#include "Arduino.h"
#include "WebServer.h"
#include "protocol.h"
#include "scenario.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

FILE *g_ops = nullptr;
SerialClass Serial;
ESPClass ESP;
unsigned long g_ms = 200000;
unsigned long millis() { return g_ms; }
unsigned long micros() { return g_ms * 1000UL; }
void delay(unsigned long ms) { g_ms += ms; }
void delayMicroseconds(unsigned long) {}
void pinMode(int, int) {}
void digitalWrite(int, int) {}
static unsigned long g_bootUntil = 0;                       // ปุ่ม BOOT (GPIO0) ถูกกดอยู่จนถึงเวลานี้
int digitalRead(int p) { return (p == 0 && g_ms < g_bootUntil) ? 0 : 1; }
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
WiFiClassStub WiFi;
static esp_now_recv_cb_t g_recvCb = nullptr;
static struct_host_sync g_sent[512];
static int g_sentCount = 0;
int esp_now_init() { return 0; }
int esp_now_deinit() { return 0; }
int esp_now_register_recv_cb(esp_now_recv_cb_t cb) { g_recvCb = cb; return 0; }
int esp_now_add_peer(const esp_now_peer_info_t *) { return 0; }
int esp_now_del_peer(const uint8_t *) { return 0; }
int esp_now_send(const uint8_t *, const uint8_t *d, int len) {
  if (len == (int)sizeof(struct_host_sync) && g_sentCount < 512) memcpy(&g_sent[g_sentCount++], d, len);
  return 0;
}
int esp_wifi_get_channel(uint8_t *ch, wifi_second_chan_t *) { *ch = 1; return 0; }
int esp_wifi_set_channel(uint8_t, wifi_second_chan_t) { return 0; }
int esp_wifi_set_ps(int) { return 0; }
static int g_setTimeCalls = 0;
static uint32_t g_setTimeValue = 0;
int settimeofday(const struct timeval *tv, const struct timezone *) { g_setTimeCalls++; g_setTimeValue = (uint32_t)tv->tv_sec; return 0; }

#include "sketch.cpp"

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { printf("ล้มเหลว: %s (บรรทัด %d)\n", msg, __LINE__); fails++; } } while (0)

static const uint8_t MAC[6] = {1, 2, 3, 4, 5, 6};
static const char *U = CYD_WEB_USER, *P = CYD_WEB_PASS;
typedef std::map<std::string, std::string> Args;

static struct_message mk(int id, int run, uint32_t drops, float rate, uint32_t since, uint8_t flags = 0) {
  struct_message m = {}; m.stationId = (uint8_t)id; m.isRunning = (uint8_t)run; m.totalDrops = drops;
  m.flowRateHr = rate; m.msSinceLastDrop = since; m.batteryVolts = 3.9f; m.flags = flags; return m;
}
static void inject(const struct_message &m) {
  uint8_t src[6]; memcpy(src, MAC, 6);
  wifi_pkt_rx_ctrl_t rc = {-52, 1};
  esp_now_recv_info_t info = { src, nullptr, &rc };
  g_recvCb(&info, (const uint8_t *)&m, sizeof(m));
}
static void runMs(int ms) { for (int e = 0; e < ms; e += 50) { g_ms += 50; serviceRx(g_ms); serviceAlerts(g_ms);
  const int s = syncSched.next(g_ms, activeBeds); if (s >= 0) linkSendSync(bedBuildSync(beds[s], s, linkChannel(), clockEpoch(), g_ms)); } }
static void reset() {
  for (int i = 0; i < MAX_BEDS; i++) { beds[i] = Bed(); wasOnline[i] = false; }
  activeBeds = 5; snoozeUntil = 0; syncSched = SyncScheduler(); events = EventLog(); g_sentCount = 0;
  netCfg = netDefaults(); netReset(); restartPending = false; g_prefBlobs().clear();
  trend = TrendState(); clockTzMin = 0;
  fs::g_mem = fs::MemFS(); fs::g_shortNames = false; sdOk = false; sdWasOk = false; sdFails = 0; sdEvN = 0; sdEvLost = 0;
  beds[0].cfg.targetRateHr = 60; beds[0].cfg.planVolumeMl = 500; beds[0].cfg.resetSeq = 9;
}
static bool nextSyncIs(int bed) { return syncSched.isPending(bed); }     // มีคำขอส่งด่วนของเตียงนี้ค้างอยู่ (รอบปกติไม่นับ)
static void dump(const char *path, const std::string &s) { FILE *f = fopen(path, "w"); fwrite(s.data(), 1, s.size(), f); fclose(f); }

static void testStaticAndReadOnly() {
  reset();
  CHECK(webCall(HTTP_GET, "/") == 200 && g_web.type.find("text/html") == 0 && g_web.body.find("<script>") != std::string::npos && g_web.body.size() > 5000, "หน้าแรกต้องส่งหน้าเว็บ");
  CHECK(g_web.body.find("innerHTML") == std::string::npos && g_web.body.find("document.write") == std::string::npos && g_web.body.find("eval(") == std::string::npos,
        "หน้าเว็บต้องไม่ใช้ innerHTML/document.write/eval (กัน HTML แทรก)");
  CHECK(webCall(HTTP_GET, "/api/state") == 200 && g_web.type == "application/json", "state ดูได้โดยไม่ต้องล็อกอิน");
  CHECK(g_web.body.find("\"beds\":[") != std::string::npos && g_web.body.find("\"version\":\"" APP_VERSION "\"") != std::string::npos, "state ต้องมีเตียงและเวอร์ชัน");
  dump("/tmp/cyd_web_state_empty.json", g_web.body);
  CHECK(webCall(HTTP_GET, "/api/events") == 200 && g_web.body == "[]", "ไม่มีเหตุการณ์ต้องได้ []");
  CHECK(webCall(HTTP_GET, "/nope") == 404, "เส้นทางที่ไม่มีต้อง 404");
  CHECK(webCall(HTTP_GET, "/api/bed") == 404, "GET บนเส้นทางที่รับเฉพาะ POST ต้อง 404");
  CHECK(g_web.begun, "ต้องเริ่มเซิร์ฟเวอร์");
}

static void testAuthAndBedUpdate() {
  reset();
  const BedConfig before = beds[0].cfg;
  Args a = {{"bed", "1"}, {"rate", "75"}};
  CHECK(webCall(HTTP_POST, "/api/bed", a) == 401, "ไม่ล็อกอินต้อง 401");
  CHECK(webCall(HTTP_POST, "/api/bed", a, U, "wrong") == 401, "รหัสผิดต้อง 401");
  CHECK(webCall(HTTP_POST, "/api/bed", a, "root", P) == 401, "ชื่อผิดต้อง 401");
  CHECK(beds[0].cfg.targetRateHr == before.targetRateHr && events.size() == 0 && !nextSyncIs(0), "ล็อกอินไม่ผ่านต้องไม่เปลี่ยนอะไรเลย");
  CHECK(!syncSched.isPending(0) && !syncSched.isPending(1), "ไม่มีคำขอส่งด่วนค้าง");
  CHECK(webCall(HTTP_POST, "/api/bed", a, U, P) == 200, "ล็อกอินถูกต้องต้องสำเร็จ");
  CHECK(beds[0].cfg.targetRateHr == 75 && beds[0].cfg.planVolumeMl == 500, "ต้องแก้เฉพาะฟิลด์ที่ส่งมา");
  CHECK(beds[0].cfg.resetSeq == 9, "แก้ค่าตั้งต้องไม่แตะ resetSeq");
  CHECK(nextSyncIs(0), "แก้ค่าแล้วต้องขอส่ง Sync ด่วน");
  CHECK(events.size() == 1 && events.newest(0).type == EV_CONFIG && events.newest(0).bed == 1, "ต้องลงประวัติ CONFIG");
  Args all = {{"bed", "2"}, {"rate", "100"}, {"plan", "1000"}, {"df", "60"}, {"near", "90"}};
  CHECK(webCall(HTTP_POST, "/api/bed", all, U, P) == 200, "แก้ทั้งสี่ฟิลด์");
  CHECK(beds[1].cfg.targetRateHr == 100 && beds[1].cfg.planVolumeMl == 1000 && beds[1].cfg.dropFactor == 60 && beds[1].cfg.nearEndPct == 90, "ค่าครบทั้งสี่");
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "2"}, {"rate", " 120 "}}, U, P) == 200 && beds[1].cfg.targetRateHr == 120, "ช่องว่างหน้า/ท้ายยอมรับได้");
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "3"}, {"rate", "50"}, {"plan", ""}}, U, P) == 200 && beds[2].cfg.planVolumeMl == 0 && beds[2].cfg.targetRateHr == 50, "ฟิลด์ว่างต้องข้าม ไม่ใช่ตั้งเป็นศูนย์");
}

static void testValidationIsAtomic() {
  reset();
  const char *badRates[] = {"abc", "60abc", "-5", "1e999", "1e2", "nan", "inf", "-inf", "3001", "0x10", "0b1", "6 0", ",", ".", "-", "+", "..5", "5..", "1.2.3"};
  for (const char *r : badRates) {
    CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "1"}, {"rate", r}}, U, P) == 400, "อัตราผิดรูปต้อง 400");
    CHECK(beds[0].cfg.targetRateHr == 60, "อัตราผิดต้องไม่เปลี่ยนค่า");
  }
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "1"}, {"plan", "10001"}}, U, P) == 400, "ปริมาตรเกินช่วง");
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "1"}, {"plan", "-1"}}, U, P) == 400, "ปริมาตรติดลบ");
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "1"}, {"df", "17"}}, U, P) == 400, "drop factor ที่ไม่มีจริง");
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "1"}, {"df", "20.5"}}, U, P) == 400, "drop factor ทศนิยม");
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "1"}, {"near", "49"}}, U, P) == 400, "ใกล้หมดต่ำเกิน");
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "1"}, {"near", "96"}}, U, P) == 400, "ใกล้หมดสูงเกิน");
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "1"}, {"near", "80.5"}}, U, P) == 400, "ใกล้หมดทศนิยม");
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "1"}}, U, P) == 400 && g_web.body == "no fields", "ไม่มีฟิลด์ต้อง 400");
  // ผิดแค่ฟิลด์เดียว ต้องไม่แก้ฟิลด์ที่ถูกด้วย (ใช้ทั้งชุดหรือไม่ใช้เลย)
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "1"}, {"rate", "99"}, {"plan", "abc"}}, U, P) == 400, "ฟิลด์ผสมถูก/ผิด");
  CHECK(beds[0].cfg.targetRateHr == 60 && beds[0].cfg.planVolumeMl == 500, "ต้องไม่แก้ครึ่งเดียว");
  {                                                              // ฟังก์ชันตรวจค่าต้องไม่แตะค่าที่ผู้เรียกส่งมาเมื่อมีฟิลด์ผิด (ไม่ต้องพึ่งตัวเรียกให้ทิ้งสำเนา)
    BedConfig c; c.targetRateHr = 60; c.planVolumeMl = 500;
    CHECK(apiParseBedUpdate("99", "abc", "", "", c) == API_BAD_VALUE && c.targetRateHr == 60 && c.planVolumeMl == 500, "apiParseBedUpdate ต้องไม่แก้ค่าครึ่งเดียวเมื่อมีฟิลด์ผิด");
    CHECK(apiParseBedUpdate("99", "", "", "", c) == API_OK && c.targetRateHr == 99, "ฟิลด์ถูกต้องทั้งหมดต้องแก้ได้");
  }
  const char *badBeds[] = {"", "0", "6", "9", "x", "1.5", "-1", "1e0x"};
  for (const char *b : badBeds) CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", b}, {"rate", "50"}}, U, P) == 400, "เลขเตียงผิดต้อง 400");
  CHECK(webCall(HTTP_POST, "/api/bed", {{"rate", "50"}}, U, P) == 400, "ไม่ส่งเลขเตียงต้อง 400");
  CHECK(events.size() == 0 && !nextSyncIs(0), "คำขอที่ผิดทั้งหมดต้องไม่ลงประวัติและไม่ส่ง Sync");
  activeBeds = 2;
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "3"}, {"rate", "50"}}, U, P) == 400, "เตียงที่ไม่ได้เปิดใช้ต้องแก้ไม่ได้");
}

static void testNewBagAndAck() {
  reset();
  Args a = {{"bed", "1"}};
  CHECK(webCall(HTTP_POST, "/api/newbag", {{"bed", "1"}, {"confirm", "1"}}) == 401, "เริ่มถุงใหม่ต้องล็อกอิน");
  CHECK(webCall(HTTP_POST, "/api/newbag", a, U, P) == 400 && g_web.body == "confirm required", "ไม่มี confirm ต้องปฏิเสธ");
  CHECK(webCall(HTTP_POST, "/api/newbag", {{"bed", "1"}, {"confirm", "0"}}, U, P) == 400, "confirm=0 ต้องปฏิเสธ");
  CHECK(beds[0].cfg.resetSeq == 9 && events.size() == 0, "ปฏิเสธแล้วต้องไม่เปลี่ยนอะไร");
  CHECK(webCall(HTTP_POST, "/api/newbag", {{"bed", "1"}, {"confirm", "1"}}, U, P) == 200, "confirm=1 ต้องสำเร็จ");
  CHECK(beds[0].cfg.resetSeq == 10 && nextSyncIs(0) && events.newest(0).type == EV_NEWBAG, "resetSeq เพิ่ม ส่ง Sync ด่วน ลงประวัติ");
  // รับทราบ
  CHECK(webCall(HTTP_POST, "/api/ack", a) == 401, "รับทราบต้องล็อกอิน");
  CHECK(webCall(HTTP_POST, "/api/ack", a, U, P) == 409, "ไม่มีเตือนใกล้หมดต้องไม่ให้รับทราบ");
  beds[0].alert = ALERT_NEAR_END;
  CHECK(webCall(HTTP_POST, "/api/ack", a, U, P) == 200 && beds[0].nearEndAck, "ใกล้หมดต้องรับทราบได้");
  CHECK(webCall(HTTP_POST, "/api/ack", a, U, P) == 409, "รับทราบซ้ำต้อง 409");
  CHECK(events.newest(0).type == EV_ACK, "ลงประวัติรับทราบ");
}

static void testClock() {
  reset();
  clockSynced = false;
  inject(mk(1, 1, 5, 60, 400)); runMs(1300);
  bool allZero = g_sentCount > 0;
  for (int i = 0; i < g_sentCount; i++) if (g_sent[i].epochTime != 0 || g_sent[i].isSynced != 0) allZero = false;
  CHECK(allZero, "ยังไม่ได้ตั้งเวลา Sync ต้องส่ง epoch 0 และ isSynced 0 (ห้ามบอก Station ว่าเวลาตรง)");
  CHECK(webCall(HTTP_POST, "/api/time", {{"epoch", "1789000000"}}) == 200 && clockSynced, "ยังไม่ตั้งเวลา: ตั้งได้โดยไม่ล็อกอิน");
  CHECK(g_setTimeCalls == 1 && g_setTimeValue == 1789000000u, "ต้องเรียกตั้งนาฬิการะบบด้วยค่าที่ส่งมา");
  g_sentCount = 0; syncSched.requestAll(5); runMs(1300);
  bool allSet = g_sentCount > 0;
  for (int i = 0; i < g_sentCount; i++) if (g_sent[i].epochTime < 1600000000u || g_sent[i].isSynced != 1) allSet = false;
  CHECK(allSet, "ตั้งเวลาแล้ว Sync ต้องส่งเวลาจริงและ isSynced=1");
  CHECK(webCall(HTTP_POST, "/api/time", {{"epoch", "1789000100"}}) == 401, "ตั้งเวลาแล้ว การตั้งซ้ำต้องล็อกอิน");
  CHECK(webCall(HTTP_POST, "/api/time", {{"epoch", "1789000100"}}, U, P) == 200 && g_setTimeValue == 1789000100u, "ล็อกอินแล้วตั้งซ้ำได้");
  const char *badEpochs[] = {"", "abc", "100", "1599999999", "4100000001", "1789000000.5", "-1", "1e10"};
  for (const char *e : badEpochs) CHECK(webCall(HTTP_POST, "/api/time", {{"epoch", e}}, U, P) == 400, "เวลาที่เป็นไปไม่ได้ต้อง 400");
  CHECK(webCall(HTTP_POST, "/api/time", {{"epoch", "1600000000"}}, U, P) == 200 && webCall(HTTP_POST, "/api/time", {{"epoch", "4100000000"}}, U, P) == 200, "ขอบช่วงเวลาที่ยอมรับ");
  clockSynced = false;
}

static void testEventsFlow() {
  reset();
  clockSynced = false;
  beds[0].cfg.targetRateHr = 60;
  inject(mk(1, 1, 10, 60, 400)); runMs(100);
  CHECK(events.size() == 1 && events.newest(0).type == EV_ONLINE && events.newest(0).bed == 1, "ได้ยินเตียงครั้งแรกต้องลง ONLINE");
  inject(mk(1, 1, 10, 60, 20000)); runMs(100);                   // ไม่มีหยดนานเกินเกณฑ์ = สายพับ
  CHECK(events.newest(0).type == EV_ALERT && events.newest(0).code == ALERT_OCCLUSION, "สายพับต้องลง ALERT OCCLUSION");
  const int n = events.size();
  inject(mk(1, 1, 10, 60, 20000)); runMs(100);
  CHECK(events.size() == n, "รหัสเดิมซ้ำต้องไม่ลงประวัติซ้ำ");
  inject(mk(1, 1, 11, 60, 300)); runMs(100);
  CHECK(events.newest(0).type == EV_CLEAR && events.newest(0).code == ALERT_OCCLUSION, "หายแล้วต้องลง CLEAR พร้อมรหัสที่หาย");
  runMs(9000);
  bool sawOffline = false;                                       // เงียบใกล้ 8 วินาทีจะนับเป็นสายพับก่อน แล้วออฟไลน์ แล้วเหตุเตือนถูกล้าง จึงดูย้อนหลังหลายรายการ
  for (int i = 0; i < 4 && i < events.size(); i++) if (events.newest(i).type == EV_OFFLINE) sawOffline = true;
  CHECK(sawOffline, "เงียบเกิน 8 วินาทีต้องลง OFFLINE");
  inject(mk(1, 1, 12, 60, 300)); runMs(100);
  CHECK(events.newest(0).type == EV_ONLINE, "กลับมาต้องลง ONLINE");
  { double d; CHECK(apiParseNumber("+5", d) && apiParseNumber("-0.5", d) && apiParseNumber(".5", d) && apiParseNumber("5.", d), "รูปแบบตัวเลขที่ยอมรับ"); }
  CHECK(webCall(HTTP_GET, "/api/events") == 200, "events ต้องดึงได้");
  dump("/tmp/cyd_web_events.json", g_web.body);
  // ลำดับใหม่สุดก่อน
  CHECK(g_web.body.find("\"event\":\"ONLINE\"") < g_web.body.find("\"event\":\"OFFLINE\""), "ต้องเรียงใหม่สุดก่อน");
  // เตียงที่ไม่เคยได้ยินต้องไม่ลง OFFLINE
  Bed fresh; (void)fresh;
  CHECK(!beds[2].seen, "เตียง 3 ไม่เคยได้ยิน");
  // เมื่อมีเวลาจริง ประวัติต้องมี epoch
  clockSet(1789000000);
  inject(mk(2, 1, 5, 60, 400)); runMs(100);
  CHECK(events.newest(0).epoch >= 1789000000u || events.newest(0).epoch >= 1600000000u, "ประวัติหลังตั้งเวลาต้องมี epoch จริง");
  clockSynced = false;
}


// เส้นทางจริงทั้งสาย: Station ประกาศรองรับ -> ไม่เคยมีหยด 90 วินาที -> Host ส่งรหัส 6 ให้เตียงนั้นเท่านั้น และเว็บแสดงสถานะ
static void testNoDropEndToEnd() {
  reset();
  clockSynced = false;
  beds[0].cfg.targetRateHr = 60; beds[1].cfg.targetRateHr = 60;
  for (int sec = 0; sec < 95; sec++) {
    inject(mk(1, 1, 0, 0, 0, MSG_FLAG_NO_SIGNAL_OK));      // Station รุ่นใหม่
    inject(mk(2, 1, 0, 0, 0, 0));                          // Station รุ่นเก่า (ไม่ประกาศ)
    runMs(1000);
  }
  CHECK(beds[0].alert == ALERT_NO_SIGNAL && beds[1].alert == ALERT_NO_SIGNAL, "ทั้งสองเตียงต้องเป็น NO_SIGNAL ที่ Host");
  int code1 = -1, code2 = -1;
  for (int i = 0; i < g_sentCount; i++) { if (g_sent[i].stationId == 1) code1 = g_sent[i].alertCode; if (g_sent[i].stationId == 2) code2 = g_sent[i].alertCode; }
  CHECK(code1 == ALERT_NO_SIGNAL, "Sync ของ Station รุ่นใหม่ต้องมีรหัส 6");
  CHECK(code2 == ALERT_NONE, "Sync ของ Station รุ่นเก่าต้องเป็น 0 ไม่ถูกรบกวน");
  CHECK(webCall(HTTP_GET, "/api/state") == 200, "state");
  CHECK(g_web.body.find("\"noDropSync\":true") != std::string::npos && g_web.body.find("\"noDropSync\":false") != std::string::npos, "เว็บต้องบอกว่าเตียงไหนรองรับ NO DROP");
  bool sawEvent = false;
  for (int i = 0; i < events.size(); i++) if (events.newest(i).type == EV_ALERT && events.newest(i).code == ALERT_NO_SIGNAL) sawEvent = true;
  CHECK(sawEvent, "ต้องลงประวัติ ALERT NO_SIGNAL");
}


// ---- ชื่อผู้ป่วย ----
static void testNames() {
  reset();
  CHECK(webCall(HTTP_POST, "/api/name", {{"bed", "1"}, {"name", "A"}}) == 401, "ตั้งชื่อต้องล็อกอิน");
  CHECK(beds[0].name[0] == 0 && events.size() == 0, "ล็อกอินไม่ผ่านต้องไม่เปลี่ยนอะไร");
  CHECK(webCall(HTTP_POST, "/api/name", {{"bed", "1"}, {"name", "Somchai K."}}, U, P) == 200 && strcmp(beds[0].name, "Somchai K.") == 0, "ตั้งชื่อ ASCII");
  CHECK(events.size() == 1 && events.newest(0).type == EV_CONFIG && events.newest(0).bed == 1, "ลงประวัติ CONFIG");
  CHECK(webCall(HTTP_POST, "/api/name", {{"bed", "2"}, {"name", "  นายสมชาย ใจดี  "}}, U, P) == 200 && strcmp(beds[1].name, "นายสมชาย ใจดี") == 0, "ชื่อไทย (ตัดช่องว่างหัวท้าย)");
  CHECK(nameIsAscii(beds[0].name) && !nameIsAscii(beds[1].name), "ตรวจ ASCII สำหรับจอ");
  CHECK(webCall(HTTP_POST, "/api/name", {{"bed", "3"}, {"name", "a\"b\\c<script>x</script>&'"}}, U, P) == 200, "อักขระพิเศษต้องเก็บได้ (ปลอดภัยเพราะเว็บใช้ textContent และ JSON หลีกให้)");
  CHECK(webCall(HTTP_POST, "/api/name", {{"bed", "1"}, {"name", ""}}, U, P) == 200 && beds[0].name[0] == 0, "ชื่อว่าง = ล้างชื่อ");
  CHECK(webCall(HTTP_POST, "/api/name", {{"bed", "1"}}, U, P) == 400, "ไม่ส่งฟิลด์ name ต้อง 400 (ไม่ใช่ล้างชื่อ)");
  CHECK(webCall(HTTP_POST, "/api/name", {{"name", "x"}}, U, P) == 400 && webCall(HTTP_POST, "/api/name", {{"bed", "9"}, {"name", "x"}}, U, P) == 400, "เลขเตียงผิด");
  const std::string thai10 = std::string("กขคงจฉชซฌญกขคงจฉชซฌญ") + "ก", thai11 = thai10 + "ฎ";    // ชื่อเก็บเป็น thai10 = 21 ตัว (63 ไบต์) thai11 = 22 ตัว (66 ไบต์)
  CHECK(thai10.size() == 63 && webCall(HTTP_POST, "/api/name", {{"bed", "4"}, {"name", thai10}}, U, P) == 200, "ไทย 21 ตัว = 63 ไบต์ ใช้ได้ (ขอบ)");
  CHECK(thai11.size() == 66 && webCall(HTTP_POST, "/api/name", {{"bed", "4"}, {"name", thai11}}, U, P) == 400 && strcmp(beds[3].name, thai10.c_str()) == 0, "ไทย 22 ตัว = 66 ไบต์ ต้องปฏิเสธและไม่แตะชื่อเดิม");
  CHECK(webCall(HTTP_POST, "/api/name", {{"bed", "5"}, {"name", std::string(63, 'x')}}, U, P) == 200, "ยาว 63 ไบต์พอดี");
  CHECK(webCall(HTTP_POST, "/api/name", {{"bed", "5"}, {"name", std::string(64, 'x')}}, U, P) == 400, "ยาว 64 ไบต์ต้องปฏิเสธ");
  // UTF-8 ผิดรูป และอักขระควบคุม ต้องปฏิเสธทั้งหมด (ไม่งั้น JSON ของเว็บพัง)
  const char *bad[] = {"\xC0\x80", "\xC1\xBF", "\xE0\x80\x80", "\xED\xA0\x80", "\xF4\x90\x80\x80", "\xF5\x80\x80\x80", "\xFF", "\x80", "\xE0\xB8", "ab\xC3", "a\nb", "a\tb", "a\x01" "b", "a\x7F" "b", "a\rb", "\xF0\x80\x80\x80"};
  for (const char *b : bad) {
    CHECK(webCall(HTTP_POST, "/api/name", {{"bed", "5"}, {"name", b}}, U, P) == 400, "UTF-8 ผิดรูป/อักขระควบคุมต้องปฏิเสธ");
  }
  CHECK(strcmp(beds[4].name, std::string(63, 'x').c_str()) == 0, "ค่าที่ถูกปฏิเสธต้องไม่ทำให้ชื่อเดิมเปลี่ยน");
  CHECK(utf8Validate("สวัสดี") == 18 && utf8Validate("😀") == 4 && utf8Validate("\xF4\x8F\xBF\xBF") == 4 && utf8Validate("\xEF\xBF\xBF") == 3, "UTF-8 ที่ถูกต้อง (รวมขอบ U+10FFFF และ U+FFFF)");
  // JSON ของ state ต้องถูกต้องแม้ชื่อมีอักขระพิเศษและภาษาไทย (python ตรวจ json.loads ใน run.sh)
  webCall(HTTP_GET, "/api/state"); dump("/tmp/cyd_web_state_names.json", g_web.body);
  CHECK(g_web.body.find("\"name\":\"นายสมชาย ใจดี\"") != std::string::npos, "state ต้องมีชื่อไทยตรงตัว");
  CHECK(g_web.body.find("<script>") == std::string::npos || g_web.body.find("\\\"") != std::string::npos, "ข้อความพิเศษถูกหลีกใน JSON");
  // จดจำข้ามการรีบูต
  saveNames(beds);
  Bed copy[MAX_BEDS];
  loadNames(copy);
  CHECK(strcmp(copy[1].name, "นายสมชาย ใจดี") == 0 && strcmp(copy[3].name, thai10.c_str()) == 0 && copy[0].name[0] == 0, "ชื่อต้องโหลดกลับได้หลังรีบูต");
  // ข้อมูลใน NVS เสียเป็นรายเตียง: ล้างเฉพาะเตียงที่เสีย ไม่ทิ้งทั้งชุด
  StoredNames sn; memset(&sn, 0, sizeof(sn)); sn.magic = NAMES_MAGIC;
  strcpy(sn.name[0], "good"); memcpy(sn.name[1], "\xC0\x80" "zz", 5); strcpy(sn.name[2], "also ok");
  memset(sn.name[3], 'x', BED_NAME_MAX + 1);                    // ไม่มีตัวจบสตริง
  Preferences pr; pr.begin("names", false); pr.putBytes("all", &sn, sizeof(sn)); pr.end();
  Bed loaded[MAX_BEDS]; loadNames(loaded);
  CHECK(strcmp(loaded[0].name, "good") == 0 && loaded[1].name[0] == 0 && strcmp(loaded[2].name, "also ok") == 0, "ชื่อเสียต้องถูกล้างเฉพาะเตียงนั้น");
  CHECK(strlen(loaded[3].name) == BED_NAME_MAX, "ชื่อไม่มีตัวจบสตริงต้องถูกตัดอย่างปลอดภัย");
  sn.magic = 0xDEADBEEF; pr.begin("names", false); pr.putBytes("all", &sn, sizeof(sn)); pr.end();
  Bed none[MAX_BEDS]; loadNames(none);
  CHECK(none[0].name[0] == 0 && none[2].name[0] == 0, "เลขยืนยันผิดต้องไม่ใช้ข้อมูลนั้นเลย");
  CHECK(static_cast<size_t>(sizeof(Bed::name)) == BED_NAME_MAX + 1, "ขนาดช่องชื่อใน Bed ต้องตรงกับ BED_NAME_MAX");
}

// ---- ตั้งค่า Wi-Fi และบัญชีเว็บ ----
static void testWifiSettings() {
  reset();
  CHECK(webCall(HTTP_GET, "/api/wifi") == 200 && g_web.type == "application/json", "อ่านข้อมูล Wi-Fi ได้โดยไม่ล็อกอิน");
  CHECK(g_web.body.find(CYD_AP_SSID) != std::string::npos, "ต้องบอกชื่อ Wi-Fi");
  CHECK(g_web.body.find(CYD_AP_PASS) == std::string::npos && g_web.body.find(CYD_WEB_PASS) == std::string::npos, "ห้ามส่งรหัสผ่านออกไปทางเว็บเด็ดขาด");
  CHECK(g_web.body.find("\"defaultWifiPass\":true") != std::string::npos && g_web.body.find("\"defaultWebPass\":true") != std::string::npos, "ต้องเตือนว่ายังใช้รหัสปริยาย");
  dump("/tmp/cyd_web_wifi.json", g_web.body);
  Args good = {{"ssid", "Ward 5 IV"}, {"pass", "newpass123"}, {"webuser", "nurse01"}, {"webpass", "s3cret!x"}};
  CHECK(webCall(HTTP_POST, "/api/wifi", good) == 401 && !restartPending, "ตั้งค่า Wi-Fi ต้องล็อกอิน");
  CHECK(webCall(HTTP_POST, "/api/wifi", good, U, "wrong") == 401, "รหัสผิดต้อง 401");
  struct Bad { const char *k, *v; } bads[] = {
    {"ssid", "has\"quote"}, {"ssid", " lead"}, {"ssid", "trail "}, {"ssid", "ไทย"}, {"ssid", "semi;colon"}, {"ssid", "012345678901234567890123456789012"},
    {"pass", "short7!"}, {"pass", "ไม่ใช่ascii1"}, {"pass", "tab\tpass123"}, {"pass", "0123456789012345678901234567890123456789012345678901234567890123456789"},
    {"webuser", "ab"}, {"webuser", "has space"}, {"webuser", "name:colon"}, {"webuser", "12345678901234567"},
    {"webpass", "12345"}, {"webpass", "has space1"}, {"webpass", "ไทยไทยไทยไทย"}, {"webpass", "123456789012345678901234567890123"}};
  for (auto &b : bads) {
    Args a = {{b.k, b.v}};
    CHECK(webCall(HTTP_POST, "/api/wifi", a, U, P) == 400, "ค่า Wi-Fi/บัญชีที่ผิดกฎต้อง 400");
  }
  CHECK(!restartPending, "ค่าผิดทั้งหมดต้องไม่ทำให้รีสตาร์ท");
  CHECK(netLoad().ssid[0] == CYD_AP_SSID[0] && strcmp(netLoad().ssid, CYD_AP_SSID) == 0, "ค่าผิดต้องไม่ถูกบันทึก");
  CHECK(webCall(HTTP_POST, "/api/wifi", {{"ssid", "OK name"}, {"pass", "short"}}, U, P) == 400 && strcmp(netLoad().ssid, CYD_AP_SSID) == 0, "ใช้ทั้งชุดหรือไม่ใช้เลย: ฟิลด์แรกถูก ฟิลด์ที่สองผิด ต้องไม่บันทึกฟิลด์แรก");
  CHECK(webCall(HTTP_POST, "/api/wifi", {}, U, P) == 400 && g_web.body == "no fields", "ไม่มีฟิลด์เลยต้อง 400");
  // สำเร็จ
  g_ms += 1000;
  const uint32_t t0 = g_ms;
  CHECK(webCall(HTTP_POST, "/api/wifi", good, U, P) == 200 && g_web.body.find("restart") != std::string::npos, "ค่าถูกต้องต้องบันทึกสำเร็จ");
  CHECK(restartPending && restartAtMs - t0 >= 2000 && restartAtMs - t0 <= 3000, "ต้องรีสตาร์ทหลังตอบเบราว์เซอร์ ไม่ใช่ทันที (ราว 2.5 วินาที)");
  const NetConfig saved = netLoad();
  CHECK(strcmp(saved.ssid, "Ward 5 IV") == 0 && strcmp(saved.wifiPass, "newpass123") == 0 && strcmp(saved.webUser, "nurse01") == 0 && strcmp(saved.webPass, "s3cret!x") == 0, "ต้องเก็บใน NVS ครบ");
  // ก่อนรีสตาร์ท ค่าใช้งานยังเป็นของเดิม (ค่าใหม่มีผลหลังรีสตาร์ท) หลังรีสตาร์ทต้องใช้ค่าใหม่
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "1"}, {"rate", "10"}}, U, P) == 200, "ก่อนรีสตาร์ทบัญชีเดิมยังใช้ได้");
  netCfg = netLoad();                                                   // จำลองการรีบูต
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "1"}, {"rate", "11"}}, U, P) == 401, "หลังรีสตาร์ทบัญชีเดิมต้องใช้ไม่ได้");
  CHECK(webCall(HTTP_POST, "/api/bed", {{"bed", "1"}, {"rate", "11"}}, "nurse01", "s3cret!x") == 200 && beds[0].cfg.targetRateHr == 11, "บัญชีใหม่ใช้ได้");
  webCall(HTTP_GET, "/api/wifi");
  CHECK(g_web.body.find("Ward 5 IV") != std::string::npos && g_web.body.find("\"defaultWifiPass\":false") != std::string::npos && g_web.body.find("\"defaultWebPass\":false") != std::string::npos, "เปลี่ยนแล้วต้องเลิกเตือนรหัสปริยาย");
  CHECK(g_web.body.find("newpass123") == std::string::npos && g_web.body.find("s3cret!x") == std::string::npos, "ห้ามส่งรหัสใหม่ออกไปทางเว็บ");
  // เปลี่ยนเฉพาะบางฟิลด์ ที่เหลือคงเดิม
  restartPending = false;
  CHECK(webCall(HTTP_POST, "/api/wifi", {{"webpass", "another9"}}, "nurse01", "s3cret!x") == 200, "เปลี่ยนเฉพาะรหัสเว็บ");
  const NetConfig part = netLoad();
  CHECK(strcmp(part.ssid, "Ward 5 IV") == 0 && strcmp(part.wifiPass, "newpass123") == 0 && strcmp(part.webPass, "another9") == 0, "ฟิลด์ที่ไม่ส่งต้องคงเดิม");
  // คืนค่าปริยายทาง Serial
  restartPending = false;
  handleLine((char *)"wifireset");
  CHECK(restartPending && strcmp(netLoad().ssid, CYD_AP_SSID) == 0 && strcmp(netLoad().webPass, CYD_WEB_PASS) == 0, "wifireset ต้องคืนค่าปริยายและรีสตาร์ท");
  // ตรวจค่าที่อ่านจาก NVS: ข้อมูลเสียต้องตกกลับเป็นค่าปริยาย
  StoredNet bad; memset(&bad, 0, sizeof(bad)); bad.magic = NET_MAGIC; bad.cfg = netDefaults(); strcpy(bad.cfg.wifiPass, "x");   // รหัสสั้นเกินกฎ
  Preferences pr; pr.begin("net", false); pr.putBytes("cfg", &bad, sizeof(bad)); pr.end();
  CHECK(strcmp(netLoad().wifiPass, CYD_AP_PASS) == 0, "ค่าใน NVS ที่ผิดกฎต้องไม่ถูกใช้ (กลับเป็นค่าปริยาย)");
  bad.magic = 0; bad.cfg = netDefaults(); strcpy(bad.cfg.ssid, "Evil"); pr.begin("net", false); pr.putBytes("cfg", &bad, sizeof(bad)); pr.end();
  CHECK(strcmp(netLoad().ssid, CYD_AP_SSID) == 0, "เลขยืนยันผิดต้องไม่ใช้ข้อมูล");
  CHECK(netValidSsid("a") && netValidSsid("My Ward-5_x.y") && !netValidSsid("") && !netValidSsid(nullptr), "ขอบของชื่อ Wi-Fi");
  CHECK(netValidWifiPass("12345678") && !netValidWifiPass("1234567") && netValidWifiPass("pass word!") , "ขอบของรหัส Wi-Fi (มีช่องว่างได้)");
  CHECK(netValidWebUser("abc") && !netValidWebUser("ab") && netValidWebPass("123456") && !netValidWebPass("12345 6"), "ขอบของบัญชีเว็บ");
}

// ---- กด BOOT ค้างตอนเปิดเครื่อง = คืนค่าปริยาย ----
static void testBootReset() {
  reset();
  NetConfig custom = netDefaults(); strcpy(custom.ssid, "Custom"); strcpy(custom.webPass, "custompw1"); netSave(custom); netCfg = netLoad();
  g_ms = 400000; g_bootUntil = 0;
  bootWifiResetCheck();
  CHECK(strcmp(netCfg.ssid, "Custom") == 0, "ไม่กด BOOT ต้องไม่รีเซ็ต");
  g_ms = 410000; g_bootUntil = g_ms + 800;                                // กดสั้น 0.8 วินาที แล้วปล่อย
  bootWifiResetCheck();
  CHECK(strcmp(netCfg.ssid, "Custom") == 0 && strcmp(netLoad().webPass, "custompw1") == 0, "กดไม่ถึง 2 วินาทีต้องไม่รีเซ็ต");
  g_ms = 420000; g_bootUntil = g_ms + 6000;                               // กดค้าง
  bootWifiResetCheck();
  CHECK(strcmp(netCfg.ssid, CYD_AP_SSID) == 0 && strcmp(netCfg.webPass, CYD_WEB_PASS) == 0, "กดค้างตอนเปิดเครื่องต้องคืนค่าปริยาย");
  CHECK(strcmp(netLoad().ssid, CYD_AP_SSID) == 0, "ต้องล้างค่าที่เก็บไว้ด้วย");
  CHECK(g_ms >= 420000 + 6000, "ต้องรอปล่อยปุ่มก่อนเริ่มระบบต่อ (กันกดค้างแล้วเข้าโหมดอื่นโดยไม่ตั้งใจ)");
  CHECK(events.size() >= 1 && events.newest(0).type == EV_CONFIG, "ต้องลงประวัติ");
}


// ---- กราฟแนวโน้ม รายงานย้อนหลัง และไฟล์บน SD ----
static double jnum(const std::string &body, const char *key) {
  const std::string k = std::string("\"") + key + "\":";
  const size_t p = body.find(k);
  return p == std::string::npos ? -999999 : atof(body.c_str() + p + k.size());
}
static std::vector<std::string> splitLines(const std::string &s) {
  std::vector<std::string> out; size_t p = 0;
  while (p < s.size()) { size_t q = s.find('\n', p); if (q == std::string::npos) q = s.size(); out.push_back(s.substr(p, q - p)); p = q + 1; }
  return out;
}
static void playStandard() {
  beds[0].cfg.targetRateHr = 60;
  uint32_t drops = 0; double acc = 0; uint32_t now = g_ms;
  int n; const Seg *segs = standardScenario(n);
  runScenario(trend, beds, 0, segs, n, now, acc, drops);
  g_ms = now;
}

static void testTrendEndpoints() {
  reset();
  CHECK(webCall(HTTP_GET, "/api/trend", {{"bed", "1"}}) == 200, "ยังไม่มีข้อมูลต้องตอบได้ (ไม่ใช่ error)");
  CHECK(jnum(g_web.body, "count") == 0 && g_web.body.find("\"r\":[]") != std::string::npos, "ไม่มีข้อมูล = อาร์เรย์ว่าง");
  CHECK(webCall(HTTP_GET, "/api/report", {{"bed", "1"}}) == 200 && jnum(g_web.body, "minutes") == 0 && jnum(g_web.body, "totalMl") == 0, "รายงานว่างต้องเป็นศูนย์ ไม่หารศูนย์");
  playStandard();
  CHECK(trend.ring[0].count == 80, "ได้ 80 ตัวอย่าง");
  CHECK(webCall(HTTP_GET, "/api/trend", {{"bed", "1"}, {"mins", "80"}}) == 200 && g_web.type == "application/json", "กราฟดึงได้โดยไม่ต้องล็อกอิน");
  CHECK(jnum(g_web.body, "count") == 80 && jnum(g_web.body, "bed") == 1 && jnum(g_web.body, "mins") == 80, "จำนวนและเลขเตียง");
  dump("/tmp/cyd_web_trend.json", g_web.body);
  CHECK(webCall(HTTP_GET, "/api/trend", {{"bed", "1"}}) == 200 && jnum(g_web.body, "mins") == 60 && jnum(g_web.body, "count") == 60, "ไม่ส่ง mins = 60 นาที");
  CHECK(webCall(HTTP_GET, "/api/trend", {{"bed", "1"}, {"mins", "240"}}) == 200 && jnum(g_web.body, "count") == 80, "ขอมากกว่าที่มีได้เท่าที่มี");
  const char *badMins[] = {"0", "241", "-5", "abc", "1e2", "10.5", "0x10", " ", "99999"};
  for (const char *m : badMins) {
    CHECK(webCall(HTTP_GET, "/api/trend", {{"bed", "1"}, {"mins", m}}) == 400, "จำนวนนาทีที่ผิดต้อง 400");
    CHECK(webCall(HTTP_GET, "/api/report", {{"bed", "1"}, {"mins", m}}) == 400, "รายงาน: จำนวนนาทีผิดต้อง 400");
  }
  for (const char *b : {"", "0", "6", "x", "1.5", "-1"}) CHECK(webCall(HTTP_GET, "/api/trend", {{"bed", b}}) == 400 && webCall(HTTP_GET, "/api/report", {{"bed", b}}) == 400, "เลขเตียงผิดต้อง 400");
  CHECK(webCall(HTTP_GET, "/api/trend") == 400, "ไม่ส่งเตียงต้อง 400");
  // รายงาน: ค่าตรงกับสถานการณ์ที่รู้คำตอบ
  CHECK(webCall(HTTP_GET, "/api/report", {{"bed", "1"}, {"mins", "80"}}) == 200, "รายงาน 80 นาที");
  dump("/tmp/cyd_web_report.json", g_web.body);
  CHECK(jnum(g_web.body, "minutes") == 80 && jnum(g_web.body, "valid") == 71 && jnum(g_web.body, "gap") == 9 && jnum(g_web.body, "running") == 61, "จำนวนนาที");
  CHECK(fabs(jnum(g_web.body, "avgRate") - 4260.0 / 61) < 0.2 && jnum(g_web.body, "minRate") == 60 && jnum(g_web.body, "maxRate") == 90, "อัตราเฉลี่ย/ต่ำสุด/สูงสุด");
  CHECK(fabs(jnum(g_web.body, "totalMl") - 70.0) < 0.3 && jnum(g_web.body, "withinPct") == 67 && jnum(g_web.body, "episodes") == 1, "ปริมาตร % ในช่วง จำนวนครั้งเตือน");
  CHECK(g_web.body.find("\"TOO_FAST\":10") != std::string::npos && g_web.body.find("\"NEAR_END\":10") != std::string::npos && g_web.body.find("\"OCCLUSION\"") == std::string::npos, "นาทีของแต่ละเตือน (ไม่แสดงรหัสที่ไม่เคยเกิด)");
  // เตียงอื่นไม่ปน
  CHECK(webCall(HTTP_GET, "/api/report", {{"bed", "2"}, {"mins", "80"}}) == 200 && jnum(g_web.body, "valid") == 0 && jnum(g_web.body, "minutes") == 0, "เตียง 2 ไม่มีตัวอย่างเลย (สถานการณ์ทดสอบเก็บเฉพาะเตียง 1) ต้องไม่ปนกับเตียง 1");
  // ดาวน์โหลด CSV รายนาทีจากหน่วยความจำ
  CHECK(webCall(HTTP_GET, "/api/trend.csv", {{"bed", "1"}}) == 401, "ดาวน์โหลด CSV ต้องล็อกอิน");
  clockSynced = false;
  CHECK(webCall(HTTP_GET, "/api/trend.csv", {{"bed", "1"}}, U, P) == 200 && g_web.type == "text/csv", "ล็อกอินแล้วดาวน์โหลดได้");
  CHECK(g_web.headers["Content-Disposition"].find("bed1-trend.csv") != std::string::npos && g_web.contentLength == CONTENT_LENGTH_UNKNOWN, "ต้องมีชื่อไฟล์ และส่งแบบไม่ทราบความยาว (chunked)");
  auto lines = splitLines(g_web.body);
  CHECK(lines.size() == 1 + 71 && lines[0] + "\n" == TREND_CSV_HEADER, "หัวคอลัมน์ + เฉพาะนาทีที่มีข้อมูล (71 บรรทัด)");
  bool fieldsOk = true; uint32_t prevUp = 0; bool ordered = true;
  for (size_t i = 1; i < lines.size(); i++) {
    int commas = 0; for (char ch : lines[i]) if (ch == ',') commas++;
    if (commas != 10) fieldsOk = false;
    unsigned long ep, up; int bed;
    if (sscanf(lines[i].c_str(), "%lu,%lu,%d", &ep, &up, &bed) != 3 || bed != 1 || ep != 0) fieldsOk = false;     // ยังไม่ตั้งนาฬิกา epoch ต้องเป็น 0
    if (up < prevUp) ordered = false;
    prevUp = up;
  }
  CHECK(fieldsOk && ordered, "ทุกบรรทัด 11 ฟิลด์ เตียง 1 epoch=0 (ยังไม่ตั้งนาฬิกา) และเรียงเก่า -> ใหม่");
  CHECK(g_web.chunks >= 2, "ข้อมูลเกิน 1 KB ต้องส่งเป็นหลายก้อน");
  // ตั้งนาฬิกาแล้ว epoch ต้องเดินห่างกันนาทีละ 60 วินาที
  clockSet(1789000000);
  CHECK(webCall(HTTP_GET, "/api/trend.csv", {{"bed", "1"}, {"mins", "10"}}, U, P) == 200, "ตั้งนาฬิกาแล้วดาวน์โหลด");
  lines = splitLines(g_web.body);
  unsigned long e1 = 0, e2 = 0;
  CHECK(lines.size() == 11, "10 นาทีล่าสุด = 10 บรรทัด + หัว");
  sscanf(lines[1].c_str(), "%lu", &e1); sscanf(lines[2].c_str(), "%lu", &e2);
  CHECK(e1 >= 1700000000 && e2 - e1 == 60, "epoch ต้องจริงและห่างกัน 60 วินาที");
  CHECK(webCall(HTTP_GET, "/api/trend.csv", {{"bed", "1"}, {"mins", "0"}}, U, P) == 400, "CSV: จำนวนนาทีผิดต้อง 400");
  clockSynced = false;
  // JSON ของกราฟกรณีเลวร้ายสุด: 240 นาที ต้องไม่ล้นบัฟเฟอร์
  reset();
  TrendRing &r = trend.ring[0];
  for (int i = 0; i < TREND_MINUTES; i++) { TrendSample x = {}; x.flags = TS_VALID | TS_RUNNING; x.rate = 3000; x.target = 3000; x.alert = ALERT_LOST_LINK; r.add(x); }
  CHECK(webCall(HTTP_GET, "/api/trend", {{"bed", "1"}, {"mins", "240"}}) == 200 && jnum(g_web.body, "count") == 240, "240 นาทีเลวร้ายสุดต้องไม่ล้น");
  printf("  กราฟ 240 นาทีเลวร้ายสุด %zu ไบต์ จากบัฟเฟอร์ %zu · หน่วยความจำวงแหวนทั้งแปดเตียง %zu ไบต์\n", g_web.body.size(), sizeof(webBuf), sizeof(TrendState));
}

static void testSdCard() {
  reset();
  fs::g_mem.cardPresent = true;
  CHECK(sdLogBegin() && sdOk, "มีการ์ดต้องเริ่มได้");
  CHECK(fs::g_mem.files["/events.csv"] == "epoch,upSec,bed,event,detail\n" && fs::g_mem.dirs.count("/trend"), "สร้างไฟล์เหตุการณ์พร้อมหัวคอลัมน์ และโฟลเดอร์ /trend");
  g_ms = 5000;
  const int opensBefore = fs::g_mem.opens;
  sdLogEvent({5000, 1789000000, 2, EV_ALERT, ALERT_OCCLUSION});
  CHECK(fs::g_mem.opens == opensBefore && fs::g_mem.files["/events.csv"].find("OCCLUSION") == std::string::npos, "เหตุการณ์เข้าคิว ยังไม่แตะการ์ด (ไม่หน่วงลูป)");
  sdLogService(g_ms + SD_EV_FLUSH_MS - 1);
  CHECK(fs::g_mem.opens == opensBefore, "ยังไม่ถึงเวลาเขียน ต้องไม่เปิดไฟล์");
  sdLogService(g_ms + SD_EV_FLUSH_MS);
  CHECK(fs::g_mem.files["/events.csv"].find("1789000000,5,2,ALERT,OCCLUSION\n") != std::string::npos, "บันทึกเหตุการณ์ลงการ์ด");
  CHECK(fs::g_mem.opens == opensBefore + 1 && sdEvN == 0, "เขียนครบด้วยการเปิดไฟล์ครั้งเดียว และคิวว่าง");
  // เหตุการณ์ถี่ (เตียงขาดสัญญาณพร้อมกันหลายเตียง) ต้องรวมเป็นก้อนเดียว
  for (int i = 0; i < 8; i++) sdLogEvent({6000u + i, 1789000010, (uint8_t)(i + 1), EV_ALERT, ALERT_LOST_LINK});
  const int opensBurst = fs::g_mem.opens;
  sdLogService(g_ms + 5000);
  CHECK(fs::g_mem.opens == opensBurst + 1, "แปดเหตุการณ์ติดกันต้องเปิดไฟล์ครั้งเดียว");
  { auto ls = splitLines(fs::g_mem.files["/events.csv"]); CHECK(ls.size() == 1 + 1 + 8 && ls.back().find(",8,ALERT,LOST_LINK") != std::string::npos, "ทุกเหตุการณ์ครบและเรียงตามเวลา"); }
  // คิวเต็มต้องเขียนทันที ไม่ทิ้งเหตุการณ์
  const size_t beforeLines = splitLines(fs::g_mem.files["/events.csv"]).size();
  for (int i = 0; i < SD_EVQ_CAP + 4; i++) sdLogEvent({7000u + i, 1789000020, 1, EV_ONLINE, 0});
  sdLogService(g_ms + 20000);
  CHECK(splitLines(fs::g_mem.files["/events.csv"]).size() == beforeLines + SD_EVQ_CAP + 4 && sdEvLost == 0, "คิวเต็มแล้วเขียนทันที ไม่มีเหตุการณ์หาย");
  // events.csv โตเกินกำหนด: หมุนเป็น events.old.csv แล้วเริ่มไฟล์ใหม่พร้อมหัวคอลัมน์
  fs::g_mem.files["/events.csv"] = std::string(SD_EVENTS_MAX_BYTES + 10, 'x');
  sdLogEvent({9000, 1789000030, 3, EV_NEWBAG, 0}); sdLogService(g_ms + 40000);
  CHECK(fs::g_mem.files["/events.old.csv"].size() == SD_EVENTS_MAX_BYTES + 10 && fs::g_mem.files["/events.csv"].find("epoch,upSec") == 0 && fs::g_mem.files["/events.csv"].find("NEWBAG") != std::string::npos, "ไฟล์เหตุการณ์ใหญ่เกินต้องหมุนเป็น .old และเริ่มไฟล์ใหม่");
  fs::g_mem.files.erase("/events.old.csv");
  fs::g_mem.files["/events.csv"] = "epoch,upSec,bed,event,detail\n1789000000,5,2,ALERT,OCCLUSION\n";
  const int opens0 = fs::g_mem.opens; sdLogBegin();
  CHECK(fs::g_mem.files["/events.csv"].find("epoch,upSec") == 0 && fs::g_mem.files["/events.csv"].find("epoch,upSec", 1) == std::string::npos, "เริ่มซ้ำต้องไม่เขียนหัวคอลัมน์ซ้ำ (ไม่ทับไฟล์เดิม)");
  (void)opens0;
  // ไฟล์แนวโน้มรายวัน
  TrendSample sm[MAX_BEDS] = {};
  for (int i = 0; i < 3; i++) { sm[i].flags = TS_VALID | TS_RUNNING; sm[i].rate = (uint16_t)(60 + i); sm[i].target = 60; }
  sm[1].flags = 0;                                                        // เตียง 2 ไม่มีข้อมูล ห้ามเขียนแถว
  sdLogTrend(sm, 3, 1789000000, 100, 420);
  const std::string f1 = fs::g_mem.files["/trend/20260910.csv"];
  CHECK(f1.find(TREND_CSV_HEADER) == 0, "ไฟล์ใหม่มีหัวคอลัมน์");
  auto l1 = splitLines(f1);
  CHECK(l1.size() == 1 + 2 && l1[1].find("1789000000,100,1,60,") == 0 && l1[2].find("1789000000,100,3,62,") == 0, "เขียนเฉพาะเตียงที่มีข้อมูล (เตียง 1 และ 3)");
  sdLogTrend(sm, 3, 1789000060, 101, 420);
  CHECK(splitLines(fs::g_mem.files["/trend/20260910.csv"]).size() == 1 + 4, "นาทีถัดไปต่อท้าย ไม่เขียนหัวซ้ำ");
  sdLogTrend(sm, 3, 1789000000, 102, -60);
  CHECK(fs::g_mem.files.count("/trend/20260909.csv") == 1, "เขตเวลาต่างกันได้ไฟล์คนละวัน (เที่ยงคืนท้องถิ่น)");
  sdLogTrend(sm, 3, 0, 5, 420);
  CHECK(fs::g_mem.files.count("/trend/unsync.csv") == 1, "ยังไม่ได้ตั้งนาฬิกา = unsync.csv");
  // รายชื่อวัน (ทั้งชื่อเต็มพาธและชื่อสั้น)
  fs::g_mem.files["/trend/notes.csv"] = "x"; fs::g_mem.files["/trend/2026112.csv"] = "x"; fs::g_mem.files["/trend/20261127.txt"] = "x"; fs::g_mem.files["/trend/2026111a.csv"] = "x";
  for (int mode = 0; mode < 2; mode++) {
    fs::g_shortNames = mode == 1;
    char days[40][9];
    const int nd = sdListDays(days, 40);
    std::vector<std::string> got; for (int i = 0; i < nd; i++) got.push_back(days[i]);
    std::sort(got.begin(), got.end());
    CHECK(got == std::vector<std::string>({"20260909", "20260910", "unsync"}), "รายชื่อวัน: เฉพาะไฟล์ที่ชื่อถูกรูปแบบ (ทั้งสองรูปแบบชื่อไฟล์ของ core)");
  }
  { char days[2][9]; CHECK(sdListDays(days, 2) <= 2, "ไม่เกินความจุอาร์เรย์"); }
  // API: /api/sd
  fs::g_shortNames = false;
  CHECK(webCall(HTTP_GET, "/api/sd") == 200 && g_web.body.find("\"ok\":true") != std::string::npos && g_web.body.find("20260909") != std::string::npos, "รายการไฟล์บน SD");
  dump("/tmp/cyd_web_sd.json", g_web.body);
  // ดาวน์โหลดจาก SD
  CHECK(webCall(HTTP_GET, "/api/sdfile", {{"file", "events"}}) == 401, "ดาวน์โหลดจาก SD ต้องล็อกอิน");
  CHECK(webCall(HTTP_GET, "/api/sdfile", {{"file", "events"}}, U, P) == 200 && g_web.body.find("ALERT,OCCLUSION") != std::string::npos && g_web.streamed == 1, "ดาวน์โหลด events.csv");
  CHECK(webCall(HTTP_GET, "/api/sdfile", {{"file", "trend"}, {"day", "20260910"}}, U, P) == 200 && g_web.body == fs::g_mem.files["/trend/20260910.csv"], "ดาวน์โหลดไฟล์รายวันตรงเนื้อหา");
  CHECK(webCall(HTTP_GET, "/api/sdfile", {{"file", "trend"}, {"day", "unsync"}}, U, P) == 200, "ดาวน์โหลด unsync");
  CHECK(webCall(HTTP_GET, "/api/sdfile", {{"file", "trend"}, {"day", "20990101"}}, U, P) == 404, "ไม่มีไฟล์ต้อง 404");
  const char *badDays[] = {"../events", "..", "2026-09-10", "", "202609100", "2026091", "20260910/../..", "unsync/..", "%2e%2e", "2026091a", "/etc/passwd", "20260910.csv"};
  for (const char *d : badDays) CHECK(webCall(HTTP_GET, "/api/sdfile", {{"file", "trend"}, {"day", d}}, U, P) == 400, "ชื่อวันที่ผิดรูปแบบ/พยายามออกนอกโฟลเดอร์ต้อง 400");
  for (const char *f : {"", "other", "../events", "events.csv", "/events.csv", "EVENTS"}) CHECK(webCall(HTTP_GET, "/api/sdfile", {{"file", f}}, U, P) == 400, "ชนิดไฟล์ที่ไม่รู้จักต้อง 400 (ไม่เคยใช้ข้อความจากผู้ใช้เป็นพาธ)");
  CHECK(g_web.streamed == 0, "คำขอที่ถูกปฏิเสธต้องไม่ส่งไฟล์ใดๆ");
  // ไม่มีการ์ด
  fs::g_mem.cardPresent = false; sdOk = false;
  CHECK(webCall(HTTP_GET, "/api/sdfile", {{"file", "events"}}, U, P) == 503, "ไม่มีการ์ดต้อง 503");
  CHECK(webCall(HTTP_GET, "/api/sd") == 200 && g_web.body.find("\"ok\":false") != std::string::npos && g_web.body.find("\"days\":[]") != std::string::npos, "ไม่มีการ์ดบอกว่าไม่พร้อม");
  CHECK(!sdLogBegin() && !sdOk, "ไม่มีการ์ดต้องเริ่มไม่ได้ และไม่ล้ม");
  const int op = fs::g_mem.opens; sdLogEvent({1, 0, 0, EV_BOOT, 0}); sdLogTrend(sm, 3, 1789000000, 1, 0);
  CHECK(fs::g_mem.opens == op, "ไม่มีการ์ดต้องไม่พยายามเปิดไฟล์เลย (ไม่หน่วงลูป)");
  // การ์ดเต็ม/ถอดกลางทาง: เขียนล้มเหลวติดกัน 3 ครั้งแล้วเลิกใช้การ์ดเงียบๆ
  fs::g_mem = fs::MemFS(); sdOk = false; sdFails = 0;
  CHECK(sdLogBegin(), "เริ่มการ์ดอีกครั้ง");
  fs::g_mem.failWrites = true;
  sdLogTrend(sm, 3, 1789000000, 1, 0); CHECK(sdOk, "ล้มเหลวครั้งที่ 1 ยังไม่เลิกใช้");
  sdLogTrend(sm, 3, 1789000000, 2, 0); CHECK(sdOk, "ล้มเหลวครั้งที่ 2 ยังไม่เลิกใช้");
  sdLogTrend(sm, 3, 1789000000, 3, 0); CHECK(!sdOk, "ล้มเหลวติดกัน 3 ครั้งต้องเลิกใช้การ์ด");
  const int op2 = fs::g_mem.opens; sdLogEvent({1, 0, 0, EV_BOOT, 0});
  CHECK(fs::g_mem.opens == op2, "เลิกใช้แล้วต้องไม่แตะการ์ดอีก");
  // หลุดแล้วต่อใหม่: ลองทุก SD_RETRY_MS เท่านั้น (ไม่หน่วงลูป) และกลับมาเขียนต่อได้เมื่อการ์ดกลับมา
  CHECK(sdWasOk && std::string(sdStatusText()) == "RETRY", "เคยใช้ได้แต่หลุด ต้องแสดงสถานะ RETRY");
  sdLastRetryMs = 100000;
  const int op3 = fs::g_mem.opens;
  sdLogService(100000 + SD_RETRY_MS - 1);
  CHECK(!sdOk && fs::g_mem.opens == op3, "ยังไม่ถึงเวลาลองต่อ ต้องไม่แตะการ์ด");
  fs::g_mem.failWrites = false;
  sdLogService(100000 + SD_RETRY_MS);
  CHECK(sdOk && std::string(sdStatusText()) == "OK", "การ์ดกลับมาแล้วต้องต่อใหม่ได้เอง");
  g_ms = 200000; sdLogEvent({200000, 1789000099, 1, EV_ONLINE, 0}); sdLogService(g_ms + SD_EV_FLUSH_MS);
  CHECK(fs::g_mem.files["/events.csv"].find("1789000099,200,1,ONLINE") != std::string::npos, "ต่อใหม่แล้วกลับมาบันทึกเหตุการณ์ได้");
  // ไม่เคยมีการ์ด: ห้ามลองต่อ (กันหน่วงลูปทุกนาทีบนเครื่องที่ไม่ใส่การ์ด)
  fs::g_mem = fs::MemFS(); fs::g_mem.cardPresent = false; sdOk = false; sdWasOk = false; sdLogBegin();
  const int op4 = fs::g_mem.opens; sdLastRetryMs = 0; sdLogService(10 * SD_RETRY_MS);
  CHECK(!sdOk && fs::g_mem.opens == op4 && std::string(sdStatusText()) == "NO", "ไม่เคยมีการ์ดต้องไม่ลองต่อ และแสดงสถานะ NO");
  fs::g_mem.cardPresent = true;
  // ล้มเหลวสลับสำเร็จ ต้องรีเซ็ตตัวนับ (ไม่สะสมข้ามเวลา)
  fs::g_mem = fs::MemFS(); sdOk = false; sdFails = 0; sdLogBegin();
  fs::g_mem.failWrites = true; sdLogTrend(sm, 3, 1789000000, 1, 0); sdLogTrend(sm, 3, 1789000000, 2, 0);
  fs::g_mem.failWrites = false; sdLogTrend(sm, 3, 1789000000, 3, 0);
  fs::g_mem.failWrites = true; sdLogTrend(sm, 3, 1789000000, 4, 0); sdLogTrend(sm, 3, 1789000000, 5, 0);
  CHECK(sdOk, "ล้มเหลวสองครั้งสลับสำเร็จหนึ่งครั้ง ต้องยังใช้การ์ดได้ (นับเฉพาะที่ติดกัน)");
}

static void testTimeZoneParam() {
  reset();
  clockSynced = false;
  CHECK(webCall(HTTP_POST, "/api/time", {{"epoch", "1789000000"}, {"tz", "420"}}) == 200 && clockTzMin == 420, "ส่งเขตเวลาไทย 420 นาที");
  CHECK(webCall(HTTP_POST, "/api/time", {{"epoch", "1789000100"}, {"tz", "420"}}, U, P) == 200 && clockTzMin == 420, "ตั้งซ้ำต้องล็อกอิน");
  const char *badTz[] = {"841", "-841", "abc", "1e2", "5.5", "0x10", "99999"};
  for (const char *z : badTz) CHECK(webCall(HTTP_POST, "/api/time", {{"epoch", "1789000200"}, {"tz", z}}, U, P) == 400 && clockTzMin == 420, "เขตเวลาที่เป็นไปไม่ได้ต้อง 400 และไม่เปลี่ยนค่าเดิม");
  CHECK(webCall(HTTP_POST, "/api/time", {{"epoch", "1789000300"}}, U, P) == 200 && clockTzMin == 420, "ไม่ส่ง tz = คงค่าเดิม");
  CHECK(webCall(HTTP_POST, "/api/time", {{"epoch", "1789000400"}, {"tz", "-840"}}, U, P) == 200 && clockTzMin == -840, "ขอบเขตลบ");
  CHECK(webCall(HTTP_POST, "/api/time", {{"epoch", "1789000500"}, {"tz", "840"}}, U, P) == 200 && clockTzMin == 840, "ขอบเขตบวก");
  clockSynced = false; clockTzMin = 0;
}

static void testRingAndCsv() {
  EventLog log;
  for (int i = 0; i < 60; i++) log.add(1000u * i, 0, (uint8_t)(1 + i % 5), EV_ALERT, ALERT_TOO_FAST);
  CHECK(log.size() == EventLog::CAP, "วงแหวนต้องไม่เกินความจุ");
  CHECK(log.newest(0).upMs == 59000u && log.newest(EventLog::CAP - 1).upMs == 12000u, "ต้องทิ้งรายการเก่าสุดและเรียงใหม่สุดก่อน");
  char line[64];
  EventRec a = {12000, 1789000000, 3, EV_ALERT, ALERT_OCCLUSION};
  CHECK(eventCsvLine(line, sizeof(line), a) > 0 && strcmp(line, "1789000000,12,3,ALERT,OCCLUSION\n") == 0, "รูปแบบ CSV ของเหตุเตือน");
  EventRec b = {5000, 0, 0, EV_BOOT, 0};
  CHECK(eventCsvLine(line, sizeof(line), b) > 0 && strcmp(line, "0,5,0,BOOT,\n") == 0, "รูปแบบ CSV ของเหตุระบบ");
  char tiny[8];
  CHECK(eventCsvLine(tiny, sizeof(tiny), a) == 0, "บัฟเฟอร์เล็กเกินต้องคืน 0 ไม่ล้น");
  EventRec c = {1, 2, 3, EV_NEWBAG, ALERT_OCCLUSION};
  CHECK(eventCsvLine(line, sizeof(line), c) > 0 && strstr(line, "OCCLUSION") == nullptr, "เหตุที่ไม่ใช่เตือนต้องไม่ใส่รหัสเตือนใน detail");
  // JSON ของ 48 เหตุการณ์ต้องพอดีบัฟเฟอร์
  char big[6144];
  CHECK(apiEventsJson(big, sizeof(big), log) == API_OK, "JSON ประวัติเต็มวงแหวนต้องไม่ล้นบัฟเฟอร์ของเซิร์ฟเวอร์");
  char small[64];
  CHECK(apiEventsJson(small, sizeof(small), log) == API_OVERFLOW, "บัฟเฟอร์เล็กต้องรายงานล้น");
  JsonOut j(small, sizeof(small)); j.str("a\"b\\c\n");
  CHECK(strcmp(small, "\"a\\\"b\\\\c\\u000a\"") == 0, "JSON ต้องหลีกอักขระพิเศษ");
}

static void testStateJsonWorstCase() {
  reset();
  activeBeds = 8;
  for (int i = 0; i < 8; i++) {
    beds[i].cfg.targetRateHr = 3000; beds[i].cfg.planVolumeMl = 10000;
    bedIngest(beds[i], mk(i + 1, 1, 4000000000u, 2999, 400), -99, MAC, g_ms);
    beds[i].batteryV = 5.99f; beds[i].linkPct = 100; beds[i].idConflict = true; beds[i].alert = ALERT_OCCLUSION;
  }
  for (int i = 0; i < 8; i++) { memset(beds[i].name, '"', BED_NAME_MAX); beds[i].name[BED_NAME_MAX] = 0; }   // ชื่อเลวร้ายสุด: ทุกตัวต้องถูกหลีก (2 เท่า)
  beds[3].flowRateHr = NAN; beds[4].batteryV = INFINITY;   // ค่าที่ไม่ควรเกิดแต่ต้องไม่ทำ JSON พัง
  CHECK(webCall(HTTP_GET, "/api/state") == 200, "สภาพเลวร้ายสุด: state ต้องไม่ล้นบัฟเฟอร์");
  CHECK(g_web.body.find("nan") == std::string::npos && g_web.body.find("inf") == std::string::npos, "ห้ามมี nan/inf ใน JSON");
  dump("/tmp/cyd_web_state_worst.json", g_web.body);
  printf("  state JSON เลวร้ายสุด %zu ไบต์ จากบัฟเฟอร์ %zu\n", g_web.body.size(), sizeof(webBuf));
}

static void testNumbers() {
  double v;
  CHECK(apiParseNumber("12.5", v) && v == 12.5, "ทศนิยมปกติ");
  CHECK(!apiParseNumber(nullptr, v) && !apiParseNumber("", v) && !apiParseNumber("   ", v) && !apiParseNumber("1 2", v), "ว่าง/ช่องว่างกลางต้องไม่ผ่าน");
  CHECK(!apiParseNumber("1,5", v) && !apiParseNumber("--1", v) && !apiParseNumber("+-1", v), "รูปแบบแปลกต้องไม่ผ่าน");
}

// ภาพหน้าเว็บ: ใช้ JSON จริงจากเฟิร์มแวร์ (สถานการณ์ผสมหลายสถานะ) ให้ run.sh ไปสร้างภาพ
static void makeShowcase() {
  reset();
  clockSynced = true; clockSet(1789000000);
  strcpy(beds[0].name, "นายสมชาย ใจดี"); strcpy(beds[1].name, "Ms. Malee R."); strcpy(beds[3].name, "ป้ายัน เตียงริมหน้าต่าง");
  beds[0].cfg = {60, 500, 20, 80, 3};  bedIngest(beds[0], mk(1, 1, 3100, 60, 400), -48, MAC, g_ms); beds[0].linkPct = 100;
  beds[1].cfg = {100, 1000, 20, 80, 0}; bedIngest(beds[1], mk(2, 1, 9000, 142, 380), -61, MAC, g_ms); beds[1].alert = ALERT_TOO_FAST; beds[1].linkPct = 90;
  beds[3].cfg = {80, 500, 20, 80, 0};  bedIngest(beds[3], mk(4, 1, 8200, 80, 700), -70, MAC, g_ms); beds[3].alert = ALERT_NEAR_END; beds[3].linkPct = 50;
  bedIngest(beds[4], mk(5, 0, 120, 0, 0), -55, MAC, g_ms); beds[4].linkPct = 100;
  logEvent(0, EV_BOOT); logEvent(1, EV_ONLINE); logEvent(2, EV_ONLINE); logEvent(2, EV_ALERT, ALERT_TOO_FAST);
  logEvent(4, EV_ALERT, ALERT_NEAR_END); logEvent(1, EV_CONFIG); logEvent(0, EV_SILENCE);
  webCall(HTTP_GET, "/api/state"); dump("/tmp/cyd_web_state_show.json", g_web.body);
  webCall(HTTP_GET, "/api/events"); dump("/tmp/cyd_web_events_show.json", g_web.body);
  { // แนวโน้มจริงจากสถานการณ์มาตรฐานใส่เตียง 1 สำหรับภาพหน้าเว็บ (เปิดการ์ดเตียง 1 ให้เห็นกราฟและรายงาน)
    TrendState save = trend; trend = TrendState();
    uint32_t drops = 0, now = g_ms; double acc = 0; int n; const Seg *segs = standardScenario(n);
    Bed tmp[MAX_BEDS]; tmp[0].cfg.targetRateHr = 60;
    runScenario(trend, tmp, 0, segs, n, now, acc, drops);
    webCall(HTTP_GET, "/api/trend", {{"bed", "1"}, {"mins", "80"}}); dump("/tmp/cyd_web_trend_show.json", g_web.body);
    webCall(HTTP_GET, "/api/report", {{"bed", "1"}, {"mins", "80"}}); dump("/tmp/cyd_web_report_show.json", g_web.body);
    trend = save;
  }
  fs::g_mem.cardPresent = true; sdOk = false; sdLogBegin();
  { TrendSample one[1] = {}; one[0].flags = TS_VALID; sdLogTrend(one, 1, 1789000000, 1, 420); sdLogTrend(one, 1, 1788913600, 1, 420); }
  webCall(HTTP_GET, "/api/sd"); dump("/tmp/cyd_web_sd_show.json", g_web.body);
  webCall(HTTP_GET, "/api/wifi"); dump("/tmp/cyd_web_wifi_show.json", g_web.body);
  webCall(HTTP_GET, "/"); dump("/tmp/cyd_web_page.html", g_web.body);
  clockSynced = false;
}

int main() {
  g_ops = fopen("/dev/null", "w");
  linkBegin(true);
  webBegin();
  cal.valid = true;
  testStaticAndReadOnly();
  testAuthAndBedUpdate();
  testValidationIsAtomic();
  testNewBagAndAck();
  testClock();
  testEventsFlow();
  testNoDropEndToEnd();
  testNames();
  testWifiSettings();
  testBootReset();
  testTrendEndpoints();
  testSdCard();
  testTimeZoneParam();
  testRingAndCsv();
  testStateJsonWorstCase();
  testNumbers();
  makeShowcase();
  fclose(g_ops);
  if (fails) { printf("รวมล้มเหลว %d รายการ\n", fails); return 1; }
  printf("ผ่านทั้งหมด\n");
  return 0;
}
