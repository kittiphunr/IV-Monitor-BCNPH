#include "Arduino.h"
#include "WiFi.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include <cstdio>
#include <cstring>
FILE* g_ops = nullptr;
SerialClass Serial;
WiFiClassStub WiFi;
unsigned long g_millis = 0;
unsigned long millis(){ return g_millis; }
unsigned long micros(){ return g_millis*1000UL; }
void delay(unsigned long ms){ g_millis += ms; }
void delayMicroseconds(unsigned long us){ g_millis += us/1000; }
void pinMode(int,int){} void digitalWrite(int,int){}
int g_adc = 2731;
bool g_simSensor = false;      // true = จำลองสัญญาณเซนเซอร์ IR จริง (ใช้ตอนเรนเดอร์หน้าคาลิเบรต)
bool g_simDrops  = true;       // false = ช่วงที่บอกผู้ใช้ว่า "อย่าให้มีหยด"
int analogRead(int){
  if (!g_simSensor) return g_adc;
  // เส้นฐาน 2700, สัญญาณรบกวนยอดถึงยอด ~36, พัลส์หยดทุก 900 ms กว้าง 12 ms ลึก 220
  int v = 2700 + ((int)((g_millis * 37) % 13) - 6) * 3;
  if (g_simDrops) {
    unsigned long t = g_millis % 900;
    if (t < 12) v -= (int)(220.0f * sinf(3.14159f * (float)t / 12.0f));
  }
  return v;
}
void analogReadResolution(int){} void analogSetAttenuation(adc_attenuation_t){}
void tone(int,unsigned int,unsigned long){} void noTone(int){}
void randomSeed(unsigned long){} long random(long m){return 0;} long random(long a,long){return a;}
void rgbLedWrite(int,int,int,int){} void neopixelWrite(int,int,int,int){}
long map(long x,long a,long b,long c,long d){return (x-a)*(d-c)/(b-a)+c;}
int esp_now_init(){return 0;}
int esp_now_register_recv_cb(esp_now_recv_cb_t){return 0;}
int esp_now_add_peer(const esp_now_peer_info_t*){return 0;}
int esp_now_del_peer(const uint8_t*){return 0;}
uint8_t g_lastUp[64]; int g_lastUpLen = 0; int g_upCount = 0;
int esp_now_send(const uint8_t*,const uint8_t* d,int n){ if (n > 0 && n <= (int)sizeof(g_lastUp)) { memcpy(g_lastUp, d, n); g_lastUpLen = n; } g_upCount++; return 0; }
int esp_wifi_set_channel(uint8_t,wifi_second_chan_t){return 0;}
int esp_wifi_get_channel(uint8_t* p,wifi_second_chan_t*){ *p=1; return 0; }
void emitMark(const char* n){ fprintf(g_ops,"MARK %s\n",n); }
#include "station_thin.cpp"   // สำเนาของ Station-Thin.ino ที่ run.sh คัดลอกมาให้

int digitalRead(int pin) { return 1; }

static int failures = 0;
static void check(bool ok, const char* what) {
  printf("  %-70s %s\n", what, ok ? "ผ่าน" : "ไม่ผ่าน");
  if (!ok) failures++;
}

// ป้อนแพ็กเก็ตลงของ Host (Protocol v4) ของเตียงตัวเอง
static void feedDown(uint8_t flags, uint8_t alert, float rate, float plan, float infused,
                     uint16_t minutes, uint8_t pct, uint8_t resetSeq = 0, uint32_t epoch = 0) {
  struct_down_v4 d; memset(&d, 0, sizeof(d));
  d.magic = V4_MAGIC_DOWN; d.stationId = currentStationId; d.flags = flags; d.alertCode = alert;
  d.resetSeq = resetSeq; d.dropFactor = 20; d.nearEndPct = 80; d.hostChannel = 6; d.epochTime = epoch;
  d.rateHr = rate; d.targetRateHr = 100; d.planMl = plan; d.infusedMl = infused;
  d.minutesLeft = minutes; d.infusedPct = pct;
  esp_now_recv_info_t info; memset(&info, 0, sizeof(info));
  OnHostDisplayRecv(&info, (const uint8_t*)&d, (int)sizeof(d));
}
static const uint8_t F_OK = V4_DOWN_HEARS_YOU | V4_DOWN_TIME_OK | V4_DOWN_PLAN_SET | V4_DOWN_RATE_OK;

static struct_up_v4 lastUp() { struct_up_v4 u; memcpy(&u, g_lastUp, sizeof(u)); return u; }
// เดินนาฬิกาจำลองจนถึงเวลาส่งรอบถัดไป แล้วคืนแพ็กเก็ตขึ้นล่าสุด
static struct_up_v4 stepAndSend() {
  int before = g_upCount;
  for (int i = 0; i < 400 && g_upCount == before; i++) { g_millis += 10; loop(); }
  return lastUp();
}


// ---- โหมดจำลองต่อเนื่อง (ใช้กับ closed_loop.py): อ่านคำสั่งจาก stdin ทีละบรรทัด ----
static void hexOut(const char* tag, const uint8_t* p, int n) {
  printf("%s ", tag); for (int i = 0; i < n; i++) printf("%02x", p[i]); printf("\n");
}
static int simMain() {
  g_ops = fopen("/dev/null", "w");
  g_millis = 200000;
  setup();
  isClockSynced = true;
  char line[512];
  int lastCount = g_upCount;
  while (fgets(line, sizeof(line), stdin)) {
    if (line[0] == 'T') { g_millis = strtoul(line + 2, 0, 10); loop();
      if (g_upCount != lastCount) { hexOut("S", g_lastUp, g_lastUpLen); lastCount = g_upCount; }
      printf("OK\n"); }
    else if (line[0] == 'D') { onDropDetected(); }
    else if (line[0] == 'P') {
      uint8_t buf[64]; int n = 0; const char* h = line + 2;
      while (h[0] && h[1] && h[0] != '\n') { unsigned v; sscanf(h, "%2x", &v); buf[n++] = (uint8_t)v; h += 2; }
      esp_now_recv_info_t info; memset(&info, 0, sizeof(info));
      OnHostDisplayRecv(&info, buf, n);
    }
    else if (line[0] == 'G') { togglePause(); }
    else if (line[0] == 'Q') {
      printf("STATE rate=%.2f vol=%.2f alert=%d minutes=%d pct=%d host=%d drops=%u status=%s running=%d target=%.1f\n",
             currentFlowRate_ml_hr, totalVolumeMl, (int)effectiveAlertCode(), minutesLeft(), infusedPct(),
             isHostOnline ? 1 : 0, (unsigned)totalDrops, uiStatusWord(uiStatusOf(effectiveAlertCode())), isRunning ? 1 : 0, (float)targetRateHr);
    }
    else if (line[0] == 'X') break;
    fflush(stdout);
  }
  return 0;
}

int main(int argc,char** argv){
  if (argc > 1 && !strcmp(argv[1], "sim")) return simMain();
  g_ops = fopen(argc>1?argv[1]:"ops.txt","w");
  setup();
  struct timeval tv = { (time_t)1789200300, 0 };   // 14:05 ตามเขตเวลาไทย
  settimeofday(&tv, NULL);
  isClockSynced = true;
  lastHostRssi = -62;
  uiAlertCode = ALERT_NONE;

  printf("ขนาดแพ็กเก็ต v4: ขึ้น %d ไบต์ ลง %d ไบต์\n", (int)sizeof(struct_up_v4), (int)sizeof(struct_down_v4));

  // ================= พฤติกรรมของเครื่อง =================
  printf("\n1) แพ็กเก็ตขึ้น: ส่งแค่ข้อเท็จจริงจากเซนเซอร์\n");
  g_millis = 50000;
  feedDown(F_OK, ALERT_NONE, 98, 1000, 0, 600, 0);
  check(isHostOnline, "ได้ยิน Host หลังรับแพ็กเก็ตลง v4");
  for (int i = 0; i < 5; i++) { g_millis += 1800; onDropDetected(); }
  struct_up_v4 u = stepAndSend();
  check(g_lastUpLen == (int)sizeof(struct_up_v4) && u.magic == V4_MAGIC_UP, "ขนาดและรหัสนำหน้าเป็น v4 ขึ้น");
  check(u.stationId == currentStationId, "ระบุเลขเตียง");
  check(u.totalDrops == 5, "จำนวนหยดสะสมตรง (5)");
  check(u.msSinceLastDrop > 0 && u.msSinceLastDrop < 3000, "เวลาจากหยดล่าสุดมีค่า");
  check((u.flags & V4_UP_RUNNING) && (u.flags & V4_UP_SENSOR_SEEN), "ธง running + เซนเซอร์เคยจับหยด");
  uint8_t s0 = u.seq; u = stepAndSend();
  check((uint8_t)(s0 + 1) == u.seq, "เลขลำดับเพิ่มทีละ 1");
  check(u.batteryMv == 0 || u.batteryMv > 2000, "แบตเตอรี่เป็นมิลลิโวลต์หรือ 0");

  printf("\n2) ผลคำนวณมาจาก Host ทั้งหมด\n");
  feedDown(F_OK | V4_DOWN_NEAR_ACK, ALERT_TOO_FAST, 137.5f, 1000, 620, 380, 62);
  check(fabsf(currentFlowRate_ml_hr - 137.5f) < 0.01f, "อัตราไหลที่แสดง = ที่ Host ส่งมา");
  check(fabsf(totalVolumeMl - 620) < 0.01f, "ปริมาตรที่ให้แล้ว = ที่ Host ส่งมา (ไม่ใช่ drops/df ในเครื่อง)");
  check(minutesLeft() == 380 && infusedPct() == 62, "นาทีที่เหลือและ % = ที่ Host ส่งมา");
  check(effectiveAlertCode() == ALERT_TOO_FAST, "รหัสเตือน = ที่ Host ส่งมา");
  check(hostNearEndAck, "ธงรับทราบเตือนใกล้หมดจาก Host");
  feedDown(F_OK & ~V4_DOWN_RATE_OK, ALERT_NONE, 55, 1000, 620, 380, 62);
  check(currentFlowRate_ml_hr == 0, "Host บอกว่าอัตรายังไม่ valid -> แสดง 0 ไม่ใช้เลขที่แนบมา");
  feedDown(F_OK & ~V4_DOWN_PLAN_SET, ALERT_NONE, 98, 0, 20, V4_MINUTES_UNKNOWN, 0);
  check(minutesLeft() == -1 && infusedPct() == -1, "ไม่มีแผน/คำนวณไม่ได้ -> แสดง --");
  // เครื่องไม่ตัดสินเตือนเอง: ไม่มีหยดเลย 10 นาทีแต่ Host ไม่เตือน = ไม่เตือน
  feedDown(F_OK, ALERT_NONE, 98, 1000, 620, 380, 62);
  g_millis += 600000; feedDown(F_OK, ALERT_NONE, 98, 1000, 620, 380, 62);
  check(effectiveAlertCode() == ALERT_NONE, "ไม่ตัดสินสายพับเองแม้เงียบนาน (เดิมตัดสินเองได้)");
  feedDown(F_OK, ALERT_OCCLUSION, 98, 1000, 620, 380, 62);
  check(effectiveAlertCode() == ALERT_OCCLUSION, "รหัสสายพับจาก Host แสดงผลได้");

  printf("\n3) หยุดชั่วคราว / กลับมานับ\n");
  togglePause();
  u = stepAndSend();
  check(!(u.flags & V4_UP_RUNNING), "หยุดชั่วคราว -> ธง running ดับ");
  check(effectiveAlertCode() == ALERT_NONE, "หยุดชั่วคราว -> ไม่มีรหัสเตือน");
  g_millis += 30000; togglePause();
  u = stepAndSend();
  check((u.flags & V4_UP_RUNNING) && u.msSinceLastDrop < 2500, "กลับมานับ: เวลาจากหยดล่าสุดเริ่มนับใหม่ ไม่รวมช่วงที่หยุด (กัน Host เห็นเป็นสายพับ)");

  printf("\n4) เริ่มถุงใหม่จาก Host\n");
  feedDown(F_OK, ALERT_NONE, 98, 1000, 100, 500, 10, 7);       // ครั้งแรกรับค่าโดยไม่รีเซ็ต
  uint32_t dropsBefore = totalDrops;
  check(dropsBefore > 0, "มียอดหยดอยู่ก่อนรีเซ็ต");
  feedDown(F_OK, ALERT_NONE, 98, 1000, 100, 500, 10, 8);       // resetSeq เปลี่ยน
  loop();
  check(totalDrops == 0, "resetSeq เปลี่ยน -> ล้างยอดหยดในเครื่อง");
  u = stepAndSend();
  check(u.totalDrops == 0 && u.msSinceLastDrop == 0, "แพ็กเก็ตขึ้นถัดไปเป็น 0 หยด ไม่มีเวลาจากหยดล่าสุด");

  printf("\n5) Host หาย\n");
  feedDown(F_OK, ALERT_TOO_SLOW, 40, 1000, 300, 900, 30);
  g_millis += HOST_TIMEOUT_MS + 500; loop();
  check(!isHostOnline, "เกินเวลา -> ขึ้นขาดการติดต่อ");
  check(currentFlowRate_ml_hr == 0 && effectiveAlertCode() == ALERT_NONE, "ไม่โชว์อัตราเก่าและไม่ถือรหัสเตือนเก่า");
  check(minutesLeft() == -1 && infusedPct() == -1, "ไม่โชว์นาทีที่เหลือและ % ที่ไม่มีใครยืนยัน");
  check(uiStatusOf(ALERT_NONE) == UI_NOHOST && String(uiStatusWord(UI_NOHOST)) == String("NO HOST"),
        "การ์ดสถานะขึ้น NO HOST ไม่ใช่ NORMAL สีเขียว");
  check(uiStatusColor(UI_NOHOST) != uiStatusColor(UI_OK), "สี NO HOST ต่างจากสี NORMAL");
  check(fabsf(totalVolumeMl - (float)totalDrops / 20.0f) < 0.01f, "ปริมาตรที่ให้แล้วตกกลับเป็น หยด/drop factor (ข้อเท็จจริง ไม่ใช่ค่าเก่าของ Host)");
  u = stepAndSend();
  check(u.magic == V4_MAGIC_UP, "ยังส่งแพ็กเก็ตขึ้นต่อเมื่อ Host หาย (Host จะได้ยินเมื่อกลับมา)");

  printf("\n6) กรองแพ็กเก็ตผิดชนิด\n");
  feedDown(F_OK, ALERT_NONE, 98, 1000, 100, 500, 10);
  float rateKeep = currentFlowRate_ml_hr;
  { uint8_t v3[20]; memset(v3, 0, sizeof(v3)); esp_now_recv_info_t info; memset(&info, 0, sizeof(info));
    ((uint32_t*)v3)[0] = 1789200300; v3[5] = currentStationId;
    OnHostDisplayRecv(&info, v3, 20); }
  check(currentFlowRate_ml_hr == rateKeep, "แพ็กเก็ต Sync v3 (20 ไบต์) ถูกกรองออก");
  { struct_down_v4 d; memset(&d, 0, sizeof(d)); d.magic = 0x5A; d.stationId = currentStationId; d.rateHr = 999; d.flags = F_OK;
    esp_now_recv_info_t info; memset(&info, 0, sizeof(info)); OnHostDisplayRecv(&info, (const uint8_t*)&d, sizeof(d)); }
  check(currentFlowRate_ml_hr == rateKeep, "ขนาดถูกแต่รหัสนำหน้าผิด ถูกกรองออก");
  { struct_down_v4 d; memset(&d, 0, sizeof(d)); d.magic = V4_MAGIC_DOWN; d.stationId = (uint8_t)(currentStationId + 1); d.rateHr = 999; d.flags = F_OK;
    esp_now_recv_info_t info; memset(&info, 0, sizeof(info)); OnHostDisplayRecv(&info, (const uint8_t*)&d, sizeof(d)); }
  check(currentFlowRate_ml_hr == rateKeep, "แพ็กเก็ตของเตียงอื่นไม่เปลี่ยนค่าบนจอเรา");

  printf("\n%s\n", failures ? "มีข้อที่ไม่ผ่าน" : "ผ่านทุกข้อ");
  if (failures) { fclose(g_ops); return 1; }

  // ================= ภาพหน้าจอ (เรนเดอร์ด้วยฟอนต์จริง) =================
  currentState = STATE_NORMAL_VIEW; isRunning = true; knownResetSeq = -1;
  g_millis = 1000000;
  feedDown(F_OK, ALERT_NONE, 98, 1000, 620, 232, 62, 0, 0);
  lastHostRssi = -62; targetRateHr = 100;
  for (int i = 0; i < TREND_BARS; i++) { trendBin[i] = 88 + (i % 7) * 4 + ((i > 20) ? -14 : 0); trendHas[i] = true; }
  trendHead = 0; trendLive = 96;
  hasFirstDropOccurred = true; lastDropTimestamp = g_millis; totalDrops = 12400;
  uiAlertCode = ALERT_NONE;
  stateNeedsRedraw = true; currentNursePage = 1; drawPage1Framework(); updatePage1Dynamic(); emitMark("p1_bag");
  stateNeedsRedraw = true; currentNursePage = 2; drawPage2Framework(); updatePage2Dynamic(); emitMark("p2_plan");
  stateNeedsRedraw = true; currentNursePage = 3; drawPage3Framework(); updatePage3Dynamic(); emitMark("p3_link");
  stateNeedsRedraw = true; currentNursePage = 4; drawPage4Framework(); updatePage4Dynamic(); emitMark("p4_trend");
  feedDown(F_OK, ALERT_NEAR_END, 98, 1000, 840, 98, 84); uiAlertCode = ALERT_NEAR_END;
  stateNeedsRedraw = true; currentNursePage = 1; drawPage1Framework(); updatePage1Dynamic(); emitMark("p1_nearend");
  feedDown(F_OK, ALERT_OCCLUSION, 0, 1000, 620, V4_MINUTES_UNKNOWN, 62); uiAlertCode = ALERT_OCCLUSION;
  drawEmergencyScreen(ALERT_OCCLUSION); updateEmergencyDynamic(); emitMark("emergency");
  // Host หาย: จอไม่มีอัตราไหล/เวลาที่เหลือ เหลือแต่จำนวนหยด
  uiAlertCode = ALERT_NONE; isHostOnline = false; currentFlowRate_ml_hr = 0; lastHostRecvTime = 1;
  currentState = STATE_NORMAL_VIEW; stateNeedsRedraw = true; currentNursePage = 1;
  drawPage1Framework(); updatePage1Dynamic(); emitMark("p1_hostlost");
  stateNeedsRedraw = true; currentNursePage = 2; drawPage2Framework(); updatePage2Dynamic(); emitMark("p2_hostlost");
  fclose(g_ops);
  return 0;
}
