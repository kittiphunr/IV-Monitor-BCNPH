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
int esp_now_send(const uint8_t*,const uint8_t*,int){return 0;}
int esp_wifi_set_channel(uint8_t,wifi_second_chan_t){return 0;}
int esp_wifi_get_channel(uint8_t* p,wifi_second_chan_t*){ *p=1; return 0; }
void emitMark(const char* n){ fprintf(g_ops,"MARK %s\n",n); }
#include "station.cpp"   // สำเนาของ .ino ที่ run.sh คัดลอกมาให้

// อ่านขาแบบจำลอง (นิยามหลัง include เพื่อให้เห็นค่า BTN_PIN / DROP_DO_PIN จากสเก็ตช์)
//  g_btnScript > 0  = ยังไม่กดปุ่ม (นับถอยหลังทีละครั้งที่อ่าน)
//  g_btnScript == 0 = กด 1 ครั้ง แล้วปล่อยค้างเป็น HIGH ต่อไป
int g_btnScript  = -1;   // -1 = ปุ่มปล่อยอยู่ตลอด (ตั้งเป็นจำนวนรอบเมื่ออยากให้ "กด" ครั้งหนึ่ง)
int g_dropLevel  = 0;     // ระดับสาย DO ของเซ็นเซอร์ (Active HIGH: 0 = ว่าง)
int digitalRead(int pin) {
  if (pin == BTN_PIN) {
    if (g_btnScript > 0) { g_btnScript--; return 1; }
    if (g_btnScript == 0) { g_btnScript = -1; return 0; }
    return 1;
  }
#ifdef DROP_DO_PIN
  if (pin == DROP_DO_PIN) return g_dropLevel;
#endif
  return 1;
}
// ป้อน Sync ของเตียงตัวเองหนึ่งแพ็กเก็ต (epoch = 0 จึงไม่ไปแตะนาฬิกาที่ตรึงไว้)
void feedHostSync(uint8_t flags, uint8_t alert = ALERT_NONE) {
  struct_host_sync s; memset(&s, 0, sizeof(s));
  s.stationId = currentStationId; s.flags = flags; s.alertCode = alert;
  s.dropFactor = 20; s.nearEndPct = 80; s.hostChannel = 1;
  // Host ตัวจริงส่งแผนการให้ของเตียงมาทุกแพ็กเก็ต ส่งค่าที่เครื่องถืออยู่กลับไป
  // ไม่อย่างนั้นแพ็กเก็ตจำลองจะล้างเป้าหมายและปริมาตรตามแผนเป็นศูนย์
  s.targetRateHr = targetRateHr; s.totalPlanMl = totalPlanMl;
  esp_now_recv_info_t info; memset(&info, 0, sizeof(info));
  OnTimeSyncRecv(&info, (const uint8_t*)&s, (int)sizeof(s));
}

int main(int argc,char** argv){
  g_ops = fopen(argc>1?argv[1]:"ops.txt","w");
  setup();
  // ---- ใส่ข้อมูลตัวอย่างเสมือนได้รับจาก Host ----
  struct timeval tv = { (time_t)1789200300, 0 };   // 14:05 ตามเขตเวลาไทย
  settimeofday(&tv, NULL);
  isClockSynced = true;
  // ได้ยิน Host ผ่านคอลแบ็กตัวจริง ไม่ตั้งค่าสถานะลัด เพราะตั้งแต่ 7.10.0 ไฟสถานะ
  // ดูทั้งสองทาง ต้องมี Sync ของเตียงตัวเองที่บอกว่า Host ได้ยินอยู่ (flags 0x06)
  feedHostSync(0x06);
  lastHostRssi = -62;
  targetRateHr = 100; totalPlanMl = 1000; dropFactor = 20; nearEndPct = 80;
  totalDrops = 620 * 20; totalVolumeMl = 620; currentFlowRate_ml_hr = 98; currentGttMin = 32.6f;
  hasFirstDropOccurred = true; lastDropTimestamp = g_millis;
  for (int i = 0; i < TREND_BARS; i++) {
    trendBin[i] = 88 + (i % 7) * 4 + ((i > 20) ? -14 : 0);
    if (i == 12 || i == 13) trendBin[i] = 40;
    trendHas[i] = true;
  }
  trendHead = 0; trendLive = 96;

  uiAlertCode = ALERT_NONE;
  stateNeedsRedraw = true; currentNursePage = 1;
  drawPage1Framework(); updatePage1Dynamic(); emitMark("p1_bag");
  stateNeedsRedraw = true; currentNursePage = 2;
  drawPage2Framework(); updatePage2Dynamic(); emitMark("p2_plan");
  stateNeedsRedraw = true; currentNursePage = 3;
  drawPage3Framework(); updatePage3Dynamic(); emitMark("p3_link");
  stateNeedsRedraw = true; currentNursePage = 4;
  drawPage4Framework(); updatePage4Dynamic(); emitMark("p4_trend");

  // ใกล้หมดถุง
  totalDrops = 840 * 20; totalVolumeMl = 840; uiAlertCode = ALERT_NEAR_END;
  stateNeedsRedraw = true; currentNursePage = 1;
  drawPage1Framework(); updatePage1Dynamic(); emitMark("p1_nearend");
  drawNearEndNoticeFramework(); updateNearEndNoticeDynamic(); emitMark("near_end");

  // เตือนสายพับ
  uiAlertCode = ALERT_OCCLUSION; currentFlowRate_ml_hr = 0;
  totalDrops = 620 * 20; totalVolumeMl = 620;
  stateNeedsRedraw = true; drawPage1Framework(); updatePage1Dynamic(); emitMark("p1_noflow");
  drawEmergencyScreen(ALERT_OCCLUSION); updateEmergencyDynamic(); emitMark("emergency");

  // หยุดชั่วคราว + screensaver + หน้าอื่น
  uiAlertCode = ALERT_NONE; isRunning = false;
  stateNeedsRedraw = true; drawPage1Framework(); updatePage1Dynamic(); emitMark("p1_paused");
  isRunning = true; currentFlowRate_ml_hr = 98;
  drawScreensaverFramework(); updateScreensaverDynamic(); emitMark("screensaver");
  drawDeveloperCreditScreen(); emitMark("credit");
  tempConfigStationId = 3; configAutoSaveTimeout = g_millis + 4000;
  drawStationIdConfigFramework(); updateStationIdConfigDynamic(); emitMark("config_id");
  drawHoldProgressHUD(3600); emitMark("hold_hud");

#ifdef DROP_DO_PIN
  // ---- เฟิร์มแวร์เซ็นเซอร์ดิจิทัล (v7.6.x): หน้าทดสอบเซ็นเซอร์ ----
  g_dropLevel = 1;
  dropIsrCount = 37;
  g_millis += 1000;
  g_btnScript = 200;                   // ค้างหน้าจอไว้ ~400 ms ก่อน "กดปุ่ม" ออก
  executeSensorTestScreen(); emitMark("sensor_test");
  g_dropLevel = 0;
#endif

#ifdef SCOPE_TOP
#ifdef STATION_STABLE_PREVIEW
  // ---- สายเสถียร: หน้าคาลิเบรต 3 ขั้น + ตรวจสอบ (อัลกอริทึมเดิมของ v7.5.x) ----
  // เรียกทีละขั้นเพื่อเก็บภาพแต่ละหน้า (ไม่กดปุ่ม ปล่อยให้แต่ละขั้นจบเองตามเวลา)
  // ภาพชุดนี้ใช้ในบทคาลิเบรตของคู่มือพยาบาล (tools/manual-build/make_images.py)
  g_simSensor = true;
  g_simDrops  = false;                 // ขั้นที่ 1 บอกผู้ใช้ว่าอย่าให้มีหยด
  calStepNoise();  emitMark("cal_1_noise");
  g_simDrops  = true;
  calStepLearn();  emitMark("cal_2_learn");
  calFinish();     emitMark("cal_3_result");
  calVerify();     emitMark("cal_4_verify");
  g_simSensor = false;
#else
  // ---- หน้าตรวจเซนเซอร์ (v7.7.x): จำลองสัญญาณจริงให้กราฟมีของให้ดู ----
  g_simSensor = true;
  g_btnScript = 6000;                  // ค้างหน้าจอไว้พอให้เรียนรู้และวาดหลายรอบ
  executeButtonCalibrationWizard(); emitMark("sensor_check");
  g_simSensor = false;
#endif
#endif
  // ---- ได้ยิน Host ทางเดียว: Host บอกว่าไม่ได้ยินเตียงนี้ (อาการในภาพถ่ายเครื่องจริง) ----
  // ไว้ท้ายสุด เพราะต้องเดินนาฬิกาจำลองไปข้างหน้า ภาพหน้าอื่นจึงไม่เปลี่ยน
  for (int k = 0; k < 7; k++) { g_millis += 1000; feedHostSync(0x04); }
  lastHostRssi = -14;
  currentState = STATE_NORMAL_VIEW;
  stateNeedsRedraw = true; currentNursePage = 3;
  drawPage3Framework(); updatePage3Dynamic(); emitMark("p3_link_oneway");


#ifdef FALLOUT_PREVIEW
  // ---- รุ่น Fallout: เก็บภาพหน้า 1 ทีละเฟรม เพื่อประกอบเป็นภาพเคลื่อนไหว ----
  // หยดทุกหยดผ่าน onDropDetected() ตัวจริง แล้วให้ updateDripAnimation() ตัวจริงวาด
  // หยดตามอัตรา 98 mL/h (ราว 1.8 วินาทีต่อหยด) หกหยด แล้วหยดหยุด
  // ภาพต้องเดินวนต่อเนื่องระหว่างหยด (ไม่ได้ก้าวตามหยด) และหยุดเองหลังหยดสุดท้ายราว 4.6 วินาที
  {
    static const unsigned gap[] = {1840, 1840, 1840, 1840, 1840, 1840};
    feedHostSync(0x06);
    lastHostRssi = -62;
    // หน้าตรวจเซนเซอร์ด้านบนล้างแผนการให้ไปแล้ว ตั้งกลับให้เหมือนภาพ p1_bag
    targetRateHr = 100; totalPlanMl = 1000; dropFactor = 20;
    totalDrops = 620 * 20; totalVolumeMl = 620; currentFlowRate_ml_hr = 98;
    isRunning = true; uiAlertCode = ALERT_NONE;
    currentState = STATE_NORMAL_VIEW; currentNursePage = 1; stateNeedsRedraw = true;
    drawPage1Framework(); updatePage1Dynamic();
    unsigned long t0 = g_millis, nextDrop = t0 + 300;
    size_t gi = 0;
    char name[32];
    for (int f = 0; f * 40UL < 16000UL; f++) {
      g_millis = t0 + f * 40UL;
      if (f % 25 == 0) feedHostSync(0x06);                 // Host ส่ง Sync ทุกวินาทีตามปกติ
      if (gi < sizeof(gap) / sizeof(gap[0]) && g_millis >= nextDrop) { onDropDetected(); nextDrop += gap[gi++]; }
      updatePage1Dynamic();
      updateDripAnimation();
      snprintf(name, sizeof(name), "fo_anim_%03d", f);
      emitMark(name);
    }
  }

  // ---- ภาพจำลองการแสดงผลทั้งหมดของภาพเดิน (docs/screens-fallout/anim/) ----
  // ทุกฉากเรียก loop() ตัวจริงทุก 30 ms: การตัดสินสัญญาณเตือน หน้าเตือนเต็มจอ ปุ่ม และการวาด
  // เป็นของเฟิร์มแวร์ทั้งหมด ตัวขับใส่ให้แค่ "หยด" (onDropDetected() ซึ่งเป็นปลายทางของ
  // ตัวตรวจจับหยด) การกดหยุดชั่วคราว (togglePause() ตัวเดียวกับปุ่มสั้น) และ Sync จาก Host
  {
    char name[48];
    auto due = [](unsigned long t, unsigned long from, unsigned long to, unsigned long gap) {
      return t >= from && t < to && (t - from) % gap == 0;
    };
    auto reset = [&](float target, float infusedMl) {
      currentState = STATE_NORMAL_VIEW; currentNursePage = 1; stateNeedsRedraw = true;
      isRunning = true; hostAlertCode = ALERT_NONE; uiAlertCode = ALERT_NONE; snoozeUntilTime = 0;
      targetRateHr = target; totalPlanMl = 1000; dropFactor = 20; nearEndPct = 80;
      totalVolumeMl = infusedMl; totalDrops = (uint32_t)(infusedMl * 20);
      currentFlowRate_ml_hr = 0; currentGttMin = 0; resetRateWindow();
      hasFirstDropOccurred = false; lastDropTimestamp = g_millis;
      foSeenInit = false;     // ตัวนับหยดถูกตั้งใหม่ระหว่างฉาก (ของจริงตั้งครั้งเดียวตอนบูต ก่อนวาดจอ)
    };
    auto run = [&](const char *tag, unsigned long durMs, uint8_t syncFlags, uint8_t hostAlert, auto events) {
      unsigned long t0 = g_millis;
      for (unsigned long t = 0; t <= durMs; t += 30) {
        g_millis = t0 + t;
        if (t % 990 == 0) feedHostSync(syncFlags, hostAlert);
        events(t);
        lastUserActivityTime = g_millis;       // ไม่ให้เข้าหน้าพักจอระหว่างถ่าย
        loop();
        snprintf(name, sizeof(name), "fo_%s_%04lu", tag, t / 30);
        emitMark(name);
      }
    };

    // ฉาก 1 เริ่มหยดแล้วหยดหยุด: STOPPED -> หยดแรก WALKING -> หยดหยุด ภาพค้าง STOPPED
    //        -> ครบเกณฑ์ NO FLOW (8 วินาทีหลังหยดสุดท้าย) เฟิร์มแวร์เปิดหน้าเตือนเต็มจอเอง
    reset(100, 620);
    run("s1", 22000, 0x06, ALERT_NONE, [&](unsigned long t) {
      if (due(t, 2010, 11100, 1800)) onDropDetected();
    });

    // ฉาก 2 หยุดชั่วคราวแล้วกลับมา: เดิน -> กดหยุด ยืนพร้อม || -> กดกลับ ภาพยังไม่เดินจนกว่า
    //        หยดแรกหลังกลับมาจะถึง -> เดินต่อ
    reset(100, 620);
    run("s2", 16000, 0x06, ALERT_NONE, [&](unsigned long t) {
      if (due(t, 600, 5400, 1800)) onDropDetected();
      if (t == 5700) togglePause();
      if (t == 10200) togglePause();
      if (due(t, 12000, 16000, 1800)) onDropDetected();
    });

    // ฉาก 3 หยดห่างแล้วหยดถี่: ห่าง 3 วินาที แล้วถี่ 0.6 วินาที ภาพเดินเร็วเท่าเดิมตลอด
    //        (ไม่ตั้งอัตราเป้าหมาย เพื่อไม่ให้ขึ้นเตือนเร็ว/ช้าเกินมาบังภาพ)
    reset(0, 620);
    run("s3", 16500, 0x06, ALERT_NONE, [&](unsigned long t) {
      if (due(t, 600, 9600, 3000)) onDropDetected();
      if (due(t, 9600, 15000, 600)) onDropDetected();
    });

    // ฉาก 4 ใกล้หมดถุง: Host เป็นผู้ตัดสินและส่งรหัส NEAR END มาใน Sync พร้อมบิตรับทราบแล้ว
    //        (ถ้ายังไม่รับทราบ เฟิร์มแวร์เปิดหน้าแจ้งเตือนเต็มจอแทน) เดินในหน้า 1 ที่เป็นสีส้ม
    reset(100, 840);
    run("s4", 7200, 0x07, ALERT_NEAR_END, [&](unsigned long t) {
      if (due(t, 300, 7200, 1800)) onDropDetected();
    });
  }
#endif
  fclose(g_ops);
  return 0;
}
