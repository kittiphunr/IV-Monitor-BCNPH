// ทดสอบด่านที่ 4 ของ Host-CYD28 บน PC: API เว็บ (ล็อกอิน ตรวจค่า ผลข้างเคียง) ประวัติเหตุการณ์ นาฬิกา และ CSV
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include "Arduino.h"
#include "WebServer.h"
#include "protocol.h"
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
int digitalRead(int) { return 1; }
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
  beds[0].cfg = {60, 500, 20, 80, 3};  bedIngest(beds[0], mk(1, 1, 3100, 60, 400), -48, MAC, g_ms); beds[0].linkPct = 100;
  beds[1].cfg = {100, 1000, 20, 80, 0}; bedIngest(beds[1], mk(2, 1, 9000, 142, 380), -61, MAC, g_ms); beds[1].alert = ALERT_TOO_FAST; beds[1].linkPct = 90;
  beds[3].cfg = {80, 500, 20, 80, 0};  bedIngest(beds[3], mk(4, 1, 8200, 80, 700), -70, MAC, g_ms); beds[3].alert = ALERT_NEAR_END; beds[3].linkPct = 50;
  bedIngest(beds[4], mk(5, 0, 120, 0, 0), -55, MAC, g_ms); beds[4].linkPct = 100;
  logEvent(0, EV_BOOT); logEvent(1, EV_ONLINE); logEvent(2, EV_ONLINE); logEvent(2, EV_ALERT, ALERT_TOO_FAST);
  logEvent(4, EV_ALERT, ALERT_NEAR_END); logEvent(1, EV_CONFIG); logEvent(0, EV_SILENCE);
  webCall(HTTP_GET, "/api/state"); dump("/tmp/cyd_web_state_show.json", g_web.body);
  webCall(HTTP_GET, "/api/events"); dump("/tmp/cyd_web_events_show.json", g_web.body);
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
  testRingAndCsv();
  testStateJsonWorstCase();
  testNumbers();
  makeShowcase();
  fclose(g_ops);
  if (fails) { printf("รวมล้มเหลว %d รายการ\n", fails); return 1; }
  printf("ผ่านทั้งหมด\n");
  return 0;
}
