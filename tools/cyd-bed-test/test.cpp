// ทดสอบ bed_model.h / protocol.h บนเครื่อง PC (เวลาส่งเข้าเป็นพารามิเตอร์ จึงจำลองได้แม่น รวมถึงตัวนับเวลาวนรอบ)
#include <cstdio>
#include <cmath>
#include <limits>
#include "bed_model.h"
#include "scenario.h"

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { printf("ล้มเหลว: %s (บรรทัด %d)\n", msg, __LINE__); fails++; } } while (0)

static struct_message pkt(int id, bool run, uint32_t drops, float rate, uint32_t since, uint8_t flags = 0) {
  struct_message m = {};
  m.stationId = (uint8_t)id; m.isRunning = run; m.totalDrops = drops; m.flowRateHr = rate;
  m.msSinceLastDrop = since; m.batteryVolts = 4.0f; m.flags = flags;
  return m;
}
static const uint8_t MAC_A[6] = {1, 2, 3, 4, 5, 6}, MAC_B[6] = {6, 5, 4, 3, 2, 1};

// ป้อนแพ็กเก็ตเป็นจังหวะ 1 วินาทีจนครบ ms แล้วประเมินรหัสเตือนทุกจังหวะ คืนรหัสสุดท้าย
static uint8_t feed(Bed &b, uint32_t &t, uint32_t ms, uint32_t drops, float rate, uint32_t since = 500) {
  uint8_t code = 0;
  for (uint32_t e = 0; e < ms; e += 1000) {
    t += 1000;
    bedIngest(b, pkt(1, true, drops, rate, since), -50, MAC_A, t);
    code = bedEvaluate(b, t);
    b.alert = code;
  }
  return code;
}

static void testOnlineAndWrap() {
  Bed b; uint32_t t = 0xFFFFFF00u;                        // ใกล้ตัวนับเวลาวนรอบ (ประมาณ 49 วัน)
  CHECK(!bedOnline(b, t), "ยังไม่เคยได้ยินต้อง OFFLINE");
  bedIngest(b, pkt(1, true, 0, 0, 0), -50, MAC_A, t);
  CHECK(bedOnline(b, t + 100), "ได้ยินแล้วต้อง ONLINE");
  CHECK(bedOnline(b, t + 7999), "ใกล้ครบเวลายังต้อง ONLINE");
  CHECK(!bedOnline(b, t + 8000), "ครบเวลาต้อง OFFLINE");
  CHECK(bedOnline(b, 0x00000100u) == false || (uint32_t)(0x100u - t) < ONLINE_TIMEOUT_MS, "ข้ามจุดวนรอบต้องคำนวณถูก");
  CHECK(bedOnline(b, t + 400u) , "ข้ามจุดวนรอบแล้วยัง ONLINE ถูกต้อง (เวลาผ่านไป 400 ms)");
}

static void testLostLink() {
  Bed b; b.cfg.targetRateHr = 60; uint32_t t = 1000;
  feed(b, t, 3000, 10, 60);
  CHECK(b.wasMonitoring, "ต้องจำว่าเคยเฝ้าอยู่");
  uint32_t last = t;
  CHECK(bedEvaluate(b, last + 20000) == ALERT_NONE, "เพิ่งหาย 20 วินาที ยังอยู่ในช่วงผ่อนผัน");
  CHECK(bedEvaluate(b, last + 39000) == ALERT_LOST_LINK, "หายเกิน 38 วินาทีต้องเป็น LOST_LINK");
  CHECK(alertCodeForStation(ALERT_LOST_LINK) == ALERT_NONE, "LOST_LINK ต้องไม่ถูกส่งไป Station");
  CHECK(alertCodeForStation(ALERT_NO_SIGNAL) == ALERT_NONE, "NO_SIGNAL ต้องไม่ถูกส่งไป Station");
  b.lostLinkAck = true;
  CHECK(bedEvaluate(b, last + 39000) == ALERT_NONE, "รับทราบแล้วต้องเงียบ");
  Bed idle;                                               // เตียงที่ไม่เคยใช้งาน ไม่ต้องร้อง
  bedIngest(idle, pkt(2, false, 0, 0, 0), -60, MAC_A, 1000);
  bedEvaluate(idle, 2000);
  CHECK(bedEvaluate(idle, 200000) == ALERT_NONE, "เตียงที่ไม่ได้นับ หายไปแล้วต้องไม่ร้อง");
}

static void testOcclusion() {
  CHECK(occlusionThresholdMs(60, 20) == 8000, "อัตราสูงต้องได้ขั้นต่ำ 8 วินาที");
  CHECK(occlusionThresholdMs(10, 20) == 45000, "10 mL/h df20 = 200 หยด/ชม. ช่วง 18 วินาที x2.5 = 45 วินาที");
  CHECK(occlusionThresholdMs(0.1f, 60) == MAX_OCCLUSION_MS, "อัตราต่ำมากต้องชนเพดาน 5 นาที");
  CHECK(occlusionThresholdMs(0, 20) == MIN_OCCLUSION_MS, "ไม่มีเป้าหมายใช้ขั้นต่ำ");
  Bed b; b.cfg.targetRateHr = 60; uint32_t t = 1000;
  CHECK(feed(b, t, 2000, 10, 60, 1000) == ALERT_NONE, "ไหลปกติต้องไม่เตือน");
  bedIngest(b, pkt(1, true, 10, 60, 9000), -50, MAC_A, t + 1000);
  CHECK(bedEvaluate(b, t + 1000) == ALERT_OCCLUSION, "ไม่มีหยดเกินเกณฑ์ต้องเป็น OCCLUSION");
  bedIngest(b, pkt(1, true, 11, 60, 300), -50, MAC_A, t + 2000);
  CHECK(bedEvaluate(b, t + 2000) == ALERT_NONE, "หยดกลับมาแล้วต้องหายเตือน");
}

static void testRateDeviation() {
  Bed b; b.cfg.targetRateHr = 100; uint32_t t = 1000;
  CHECK(feed(b, t, 19000, 50, 140) == ALERT_NONE, "เร็วเกินแค่ 19 วินาทียังไม่เตือน");
  CHECK(feed(b, t, 2000, 51, 140) == ALERT_TOO_FAST, "เร็วเกินครบ 20 วินาทีต้องเตือน");
  CHECK(feed(b, t, 1000, 52, 100) == ALERT_NONE, "กลับมาปกติต้องหาย");
  CHECK(feed(b, t, 10000, 60, 140) == ALERT_NONE, "นับ 20 วินาทีใหม่ ไม่สะสมจากรอบก่อน");
  CHECK(feed(b, t, 1000, 61, 100) == ALERT_NONE && b.deviationSince == 0, "คั่นด้วยค่าปกติต้องรีเซ็ตตัวจับเวลา");
  CHECK(feed(b, t, 25000, 70, 50) == ALERT_TOO_SLOW, "ช้าเกินต่อเนื่องต้องเตือน TOO_SLOW");
  Bed few; few.cfg.targetRateHr = 100; uint32_t u = 1000;
  CHECK(feed(few, u, 30000, 2, 140) == ALERT_NONE, "น้อยกว่า 3 หยดยังไม่ตัดสินอัตรา");
}

static void testVolumeAlerts() {
  Bed b; b.cfg.targetRateHr = 100; b.cfg.planVolumeMl = 500; uint32_t t = 1000;   // df 20 -> 10000 หยด
  CHECK(feed(b, t, 1000, 7900, 100) == ALERT_NONE, "79% ยังไม่เตือน");
  CHECK(feed(b, t, 1000, 8000, 100) == ALERT_NEAR_END, "80% ต้องเตือนใกล้หมด");
  b.cfg.nearEndPct = 90;
  CHECK(feed(b, t, 1000, 8000, 100) == ALERT_NONE, "ตั้งเกณฑ์ 90% แล้ว 80% ต้องไม่เตือน");
  CHECK(feed(b, t, 1000, 10000, 100) == ALERT_COMPLETE, "ครบแผนต้องเป็น COMPLETE");
  Bed p; p.cfg.planVolumeMl = 500; uint32_t u = 1000;
  bedIngest(p, pkt(1, false, 10000, 0, 0), -50, MAC_A, u);
  CHECK(bedEvaluate(p, u) == ALERT_NONE, "หยุดชั่วคราวต้องไม่เตือน");
  struct_message ack = pkt(1, true, 8000, 100, 500, MSG_FLAG_NEAR_END_ACK);
  Bed a; a.cfg.planVolumeMl = 500; bedIngest(a, ack, -50, MAC_A, u);
  CHECK(a.nearEndAck, "Station กดรับทราบต้องจำไว้");
  Bed noPlan; bedIngest(noPlan, ack, -50, MAC_A, u);
  CHECK(!noPlan.nearEndAck, "ไม่มีแผนปริมาตร ต้องไม่รับค่ารับทราบ");
}

static void testNoSignal() {
  Bed b; b.cfg.targetRateHr = 60; uint32_t t = 1000;
  CHECK(feed(b, t, 89000, 0, 0, 0) == ALERT_NONE, "ติดต่อได้ 89 วินาทียังไม่เตือน");
  CHECK(feed(b, t, 1000, 0, 0, 0) == ALERT_NONE, "ใกล้ครบ 90 วินาทียังไม่เตือน (เกณฑ์คือเกิน)");
  CHECK(feed(b, t, 2000, 0, 0, 0) == ALERT_NO_SIGNAL, "เกิน 90 วินาทีไม่เคยมีหยดต้อง NO_SIGNAL");
  CHECK(feed(b, t, 1000, 1, 60, 200) != ALERT_NO_SIGNAL, "มีหยดแล้วต้องหาย NO_SIGNAL");
}

static void testNewBagAndSync() {
  Bed b; b.cfg.targetRateHr = 80; b.cfg.planVolumeMl = 500; b.cfg.resetSeq = 255; uint32_t t = 5000;
  feed(b, t, 2000, 8000, 80);
  b.nearEndAck = true; b.alert = ALERT_NEAR_END;
  bedNewBag(b, t);
  CHECK(b.cfg.resetSeq == 0, "resetSeq ต้องวนจาก 255 เป็น 0");
  CHECK(b.totalDrops == 0 && !b.nearEndAck && b.alert == ALERT_NONE, "เริ่มถุงใหม่ต้องล้างสถานะผูกถุง");
  struct_host_sync s = bedBuildSync(b, 2, 6, 1789000000UL, t);
  CHECK(s.stationId == 3 && s.resetSeq == 0 && s.hostChannel == 6 && s.isSynced == 1, "ฟิลด์พื้นฐานของ Sync");
  CHECK(s.dropFactor == 20 && s.nearEndPct == 80 && s.targetRateHr == 80 && s.totalPlanMl == 500, "ค่าตั้งใน Sync");
  CHECK((s.flags & SYNC_FLAG_REPORTS) && (s.flags & SYNC_FLAG_HEARD) && !(s.flags & SYNC_FLAG_NEAR_END_ACK), "ธง Sync ของเตียงที่ได้ยินอยู่");
  b.nearEndAck = true;
  CHECK(bedBuildSync(b, 0, 1, 0, t).flags & SYNC_FLAG_NEAR_END_ACK, "ธงรับทราบ");
  CHECK(bedBuildSync(b, 0, 1, 0, t).isSynced == 0, "ไม่มีเวลาจริงต้อง isSynced=0");
  CHECK(!(bedBuildSync(b, 0, 1, 0, t + 60000).flags & SYNC_FLAG_HEARD), "เตียงเงียบต้องไม่มีธง HEARD");
  b.alert = ALERT_LOST_LINK;
  CHECK(bedBuildSync(b, 0, 1, 0, t).alertCode == ALERT_NONE, "รหัสภายในต้องไม่หลุดออกไป");
  b.alert = ALERT_OCCLUSION;
  CHECK(bedBuildSync(b, 0, 1, 0, t).alertCode == ALERT_OCCLUSION, "รหัสสายพับต้องส่งออก");
  b.cfg.dropFactor = 17; b.cfg.nearEndPct = 10;
  struct_host_sync bad = bedBuildSync(b, 0, 1, 0, t);
  CHECK(bad.dropFactor == 20 && bad.nearEndPct == 80, "ค่านอกช่วงต้องถูกแก้เป็นค่าปริยายก่อนส่ง");
}

static void testPlausibility() {
  struct_message m = pkt(1, true, 5, 50, 100);
  CHECK(packetPlausible(m, 8), "แพ็กเก็ตปกติต้องผ่าน");
  m.stationId = 0; CHECK(!packetPlausible(m, 8), "เลขเตียง 0 ต้องตก");
  m.stationId = 9; CHECK(!packetPlausible(m, 8), "เลขเตียงเกินต้องตก");
  m = pkt(1, true, 5, 50, 100); m.isRunning = 2; CHECK(!packetPlausible(m, 8), "isRunning=2 ต้องตก");
  m = pkt(1, true, 5, 5000, 100); CHECK(!packetPlausible(m, 8), "อัตราเกินจริงต้องตก");
  m = pkt(1, true, 5, -1, 100); CHECK(!packetPlausible(m, 8), "อัตราติดลบต้องตก");
  m = pkt(1, true, 5, std::numeric_limits<float>::quiet_NaN(), 100); CHECK(!packetPlausible(m, 8), "NaN ต้องตก");
  m = pkt(1, true, 5, 50, 100); m.batteryVolts = 9; CHECK(!packetPlausible(m, 8), "แบตเกินจริงต้องตก");
}

static void testLinkAndConflict() {
  Bed beds[2];
  uint32_t t = 0;
  for (int i = 0; i < 7; i++) bedIngest(beds[0], pkt(1, true, 1, 1, 1), -50, MAC_A, t += 1000);
  for (int i = 0; i < 12; i++) bedIngest(beds[1], pkt(2, true, 1, 1, 1), -50, MAC_A, t += 10);
  bedsLinkTick(beds, 2);
  CHECK(beds[0].linkPct == 70 && beds[1].linkPct == 100, "คุณภาพลิงก์ 7/10 = 70% และเพดาน 100%");
  CHECK(beds[0].rxWindow == 0, "ต้องล้างตัวนับหน้าต่าง");
  Bed c; t = 0;
  for (int i = 0; i < 3; i++) { bedIngest(c, pkt(1, true, 1, 1, 1), -50, (i & 1) ? MAC_B : MAC_A, t += 1000); }
  CHECK(!c.idConflict, "สลับ MAC น้อยครั้งยังไม่ถือว่าซ้ำ");
  for (int i = 0; i < 4; i++) { bedIngest(c, pkt(1, true, 1, 1, 1), -50, (i & 1) ? MAC_A : MAC_B, t += 1000); }
  CHECK(c.idConflict, "MAC สลับถี่ต้องจับได้ว่ามีสองบอร์ดเลขซ้ำ");
}

static void testScheduler() {
  SyncScheduler s;
  uint32_t t = 10000;
  CHECK(s.slotMs(5) == 200 && s.slotMs(8) == 125 && s.slotMs(30) == 60 && s.slotMs(0) == 1000, "ขนาดช่องเวลา");
  bool seen[5] = {false};
  int sent = 0;
  for (int k = 0; k < 40; k++) {                           // เดินเวลาทีละ 10 ms
    t += 10;
    int i = s.next(t, 5);
    if (i >= 0) { CHECK(i < 5, "ต้องเป็นเตียงที่เปิดใช้"); seen[i] = true; sent++; }
  }
  CHECK(sent == 2, "ในเวลา 400 ms ต้องส่งแค่ 2 ใบ (ช่อง 200 ms) ไม่ใช่รัวทุกจังหวะ");
  for (int k = 0; k < 300; k++) { t += 10; int i = s.next(t, 5); if (i >= 0) seen[i] = true; }
  CHECK(seen[0] && seen[1] && seen[2] && seen[3] && seen[4], "วนส่งครบทุกเตียง");
  s.request(3);
  t += 250;
  CHECK(s.next(t, 5) == 3, "งานด่วนต้องมาก่อนรอบปกติ");
  s.request(9); s.request(-1);
  t += 250;
  CHECK(s.next(t, 5) != 9, "เลขเตียงเกินต้องถูกเมิน");
  SyncScheduler z;
  CHECK(z.next(t + 5000, 0) == -1, "ไม่มีเตียงเปิดใช้ต้องไม่ส่ง");
}


static void testNoDropCapability() {
  // Station ประกาศความสามารถผ่านบิต 0x02 ของ flags: ต้องจำเฉพาะเตียงที่ประกาศ และอ่านใหม่ทุกแพ็กเก็ต
  Bed b; b.cfg.targetRateHr = 60;
  uint32_t t = 1000;
  bedIngest(b, pkt(1, true, 0, 0, 0, 0x00), -50, MAC_A, t);
  CHECK(!b.supportsNoSignal, "Station รุ่นเก่า (ไม่มีบิต) ต้องไม่ถูกจดว่ารองรับ");
  bedIngest(b, pkt(1, true, 0, 0, 0, MSG_FLAG_NO_SIGNAL_OK), -50, MAC_A, t += 1000);
  CHECK(b.supportsNoSignal, "ประกาศแล้วต้องจดว่ารองรับ");
  CHECK(!b.nearEndAck, "บิต 0x02 ต้องไม่ถูกตีเป็นรับทราบใกล้หมด (บิต 0x01)");
  bedIngest(b, pkt(1, true, 0, 0, 0, MSG_FLAG_NO_SIGNAL_OK | MSG_FLAG_NEAR_END_ACK), -50, MAC_A, t += 1000);
  CHECK(b.supportsNoSignal, "สองบิตพร้อมกันต้องอ่านได้ทั้งคู่");
  bedIngest(b, pkt(1, true, 0, 0, 0, 0x00), -50, MAC_A, t += 1000);
  CHECK(!b.supportsNoSignal, "เปลี่ยนไปใช้ Station รุ่นเก่าในเตียงเดิม ต้องเลิกส่งรหัส 6 ทันที");
  // รหัสที่ส่งออก
  b.alert = ALERT_NO_SIGNAL;
  b.supportsNoSignal = true;
  CHECK(bedBuildSync(b, 0, 1, 0, t).alertCode == ALERT_NO_SIGNAL, "Station รองรับต้องได้รหัส 6");
  b.supportsNoSignal = false;
  CHECK(bedBuildSync(b, 0, 1, 0, t).alertCode == ALERT_NONE, "Station รุ่นเก่าต้องได้ 0 (ไม่รู้จักรหัส 6)");
  b.alert = ALERT_LOST_LINK; b.supportsNoSignal = true;
  CHECK(bedBuildSync(b, 0, 1, 0, t).alertCode == ALERT_NONE, "LOST_LINK เป็นรหัสภายใน Host ห้ามหลุดออกไปแม้ Station รองรับ");
  const uint8_t others[] = {ALERT_NONE, ALERT_TOO_FAST, ALERT_TOO_SLOW, ALERT_NEAR_END, ALERT_COMPLETE, ALERT_OCCLUSION};
  for (uint8_t code : others) {
    b.alert = code;
    b.supportsNoSignal = false; const uint8_t a = bedBuildSync(b, 0, 1, 0, t).alertCode;
    b.supportsNoSignal = true;  const uint8_t c = bedBuildSync(b, 0, 1, 0, t).alertCode;
    CHECK(a == code && c == code, "รหัส 0-5 ต้องส่งออกตามจริงไม่ว่า Station จะประกาศหรือไม่");
  }
  CHECK(alertCodeForBed(ALERT_NO_SIGNAL, true) == 6 && alertCodeForBed(ALERT_NO_SIGNAL, false) == 0, "alertCodeForBed ตรงๆ");
  CHECK(MSG_FLAG_NO_SIGNAL_OK == 0x02 && SYNC_FLAG_HEARD == 0x02, "ค่าบิต (อยู่คนละโครงสร้าง จึงใช้เลขเดียวกันได้)");
  // ตลอดเส้นทางจริง: เตียงที่ประกาศรองรับ ออนไลน์ ไม่เคยมีหยด 90 วินาที -> NO_SIGNAL -> Sync ส่งรหัส 6
  Bed n; n.cfg.targetRateHr = 60; uint32_t u = 1000;
  uint8_t code = 0;
  for (uint32_t e = 0; e < 92000; e += 1000) {
    u += 1000;
    bedIngest(n, pkt(1, true, 0, 0, 0, MSG_FLAG_NO_SIGNAL_OK), -50, MAC_A, u);
    code = bedEvaluate(n, u); n.alert = code;
  }
  CHECK(code == ALERT_NO_SIGNAL, "ไม่เคยมีหยด 90 วินาทีต้องเป็น NO_SIGNAL");
  CHECK(bedBuildSync(n, 0, 1, 0, u).alertCode == 6, "Sync ของเตียงนี้ต้องพก NO DROP ไปให้ Station");
  bedIngest(n, pkt(1, true, 3, 60, 200, MSG_FLAG_NO_SIGNAL_OK), -50, MAC_A, u += 1000);
  n.alert = bedEvaluate(n, u);
  CHECK(bedBuildSync(n, 0, 1, 0, u).alertCode == 0, "มีหยดแล้วต้องเลิกส่งรหัส 6");
}


static void testTrend() {
  // ---- สถานการณ์มาตรฐาน 80 นาที ----
  TrendState t;
  Bed beds[MAX_BEDS];
  beds[0].cfg.targetRateHr = 60;
  uint32_t now = 1000, drops = 0; double acc = 0;
  int n; const Seg *segs = standardScenario(n);
  const int closes = runScenario(t, beds, 0, segs, n, now, acc, drops);
  CHECK(closes == 80 && t.ring[0].count == 80, "ต้องปิดนาทีได้ 80 ครั้งและมี 80 ตัวอย่าง");
  const TrendSummary s = trendSummarize(t.ring[0], 80, 20);
  CHECK(s.minutes == 80 && s.gapMinutes == 9 && s.validMinutes == 71, "นาทีที่ Station เงียบ: 9 นาทีไม่มีข้อมูล (นาทีแรกของช่วงเงียบยังมีข้อมูล 8 วินาทีสุดท้ายก่อนออฟไลน์)");
  CHECK(s.runningMinutes == 61, "นาทีที่กำลังนับ = 71 - หยุดชั่วคราว 10");
  CHECK(fabsf(s.avgRate - 4260.0f / 61) < 0.5f && s.minRate == 60 && s.maxRate == 90, "อัตราเฉลี่ย ต่ำสุด สูงสุด (เฉลี่ยเฉพาะนาทีที่กำลังนับ ไม่เอาช่วงหยุดมาถ่วงเป็นศูนย์)");
  CHECK(fabsf(s.totalMl - 70.0f) < 0.3f, "ปริมาตรที่ให้ = หยดรวม / drop factor (ประมาณ 70 mL)");
  CHECK(s.withinPct == 67 && fabsf(s.devPct - 1000.0f / 61) < 0.5f, "อยู่ใน ±10% ของเป้า 67% และคลาดเฉลี่ย 16.4%");
  CHECK(s.alertEpisodes == 1 && s.alertMinutes[ALERT_TOO_FAST] == 10 && s.alertMinutes[ALERT_NEAR_END] == 10, "เตือนเร็วเกิน 1 ครั้ง 10 นาที และใกล้หมด 10 นาที (ใกล้หมดไม่นับเป็นเหตุวิกฤต)");
  // ช่วงเงียบต้องเป็น "ไม่มีข้อมูล" ไม่ใช่ศูนย์
  int gapSamples = 0, zeroValid = 0;
  for (int i = 0; i < t.ring[0].count; i++) { const TrendSample &x = t.ring[0].newest(i); if (!(x.flags & TS_VALID)) gapSamples++; else if (x.rate == 0 && (x.flags & TS_RUNNING)) zeroValid++; }
  CHECK(gapSamples == 9 && zeroValid == 0, "ช่วงเงียบไม่ใช่อัตราไหลศูนย์ และไม่มีนาทีที่กำลังนับแล้วอัตราเป็นศูนย์");
  // ช่วงสั้นกว่าที่มี และยาวกว่าที่มี
  const TrendSummary last10 = trendSummarize(t.ring[0], 10, 20);
  CHECK(last10.minutes == 10 && last10.avgRate == 60 && last10.alertMinutes[ALERT_NEAR_END] == 10, "สรุป 10 นาทีล่าสุด");
  CHECK(trendSummarize(t.ring[0], 240, 20).minutes == 80, "ขอมากกว่าที่มีต้องได้เท่าที่มี");
  // ตัวอย่างล่าสุดต้องเป็นเป้า 60 และมีลิงก์
  CHECK(t.ring[0].newest(0).target == 60 && t.ring[0].newest(0).link == 100, "ตัวอย่างต้องเก็บเป้าหมายและลิงก์");

  // ---- วงแหวน: ล้นแล้วต้องทิ้งของเก่าสุด เรียงใหม่สุดก่อน ----
  TrendRing r;
  for (int i = 0; i < 300; i++) { TrendSample x = {}; x.flags = TS_VALID | TS_RUNNING; x.rate = (uint16_t)i; r.add(x); }
  CHECK(r.count == TREND_MINUTES && r.totalMinutes == 300 && r.newest(0).rate == 299 && r.newest(TREND_MINUTES - 1).rate == 60, "วงแหวนล้นต้องเก็บ 240 ตัวอย่างล่าสุด");

  // ---- ตัวนับหยดรีเซ็ต (เริ่มถุงใหม่) กลางนาที ต้องไม่ทำให้ปริมาตรติดลบหรือพุ่ง ----
  TrendState t2; Bed b2[MAX_BEDS]; b2[0].cfg.targetRateHr = 60;
  uint32_t n2 = 5000; const uint8_t mac[6] = {1, 2, 3, 4, 5, 6};
  struct_message m = {}; m.stationId = 1; m.isRunning = 1; m.flowRateHr = 60; m.msSinceLastDrop = 100; m.batteryVolts = 4;
  m.totalDrops = 1000; bedIngest(b2[0], m, -50, mac, n2); trendObserve(t2, b2, 1, n2);
  n2 += 1000; m.totalDrops = 1040; bedIngest(b2[0], m, -50, mac, n2); trendObserve(t2, b2, 1, n2);   // +40
  n2 += 1000; m.totalDrops = 10; bedIngest(b2[0], m, -50, mac, n2); trendObserve(t2, b2, 1, n2);      // รีเซ็ต: นับ 10 ใหม่
  CHECK(t2.acc[0].dropsAdded == 50, "ตัวนับรีเซ็ตต้องนับหยดใหม่จากศูนย์ ไม่ลบไม่ข้าม (40 + 10)");
  CHECK(fabsf(t2.acc[0].volAdded - 50.0f / 20) < 0.01f, "ปริมาตรที่เพิ่มคิดตาม drop factor");

  // ---- เปลี่ยนเป้าหมายระหว่างถุง: ตัวอย่างเก็บเป้า ณ ตอนนั้น ----
  TrendState t3; Bed b3[MAX_BEDS]; b3[0].cfg.targetRateHr = 50; uint32_t n3 = 0, d3 = 0; double a3 = 0;
  const Seg a[] = {{5, true, true, 50, ALERT_NONE}};
  runScenario(t3, b3, 0, a, 1, n3, a3, d3);
  b3[0].cfg.targetRateHr = 80;
  const Seg bb[] = {{5, true, true, 80, ALERT_NONE}};
  runScenario(t3, b3, 0, bb, 1, n3, a3, d3);
  CHECK(t3.ring[0].newest(9).target == 50 && t3.ring[0].newest(0).target == 80, "เป้าหมายแต่ละนาทีต้องเป็นค่า ณ ตอนนั้น");
  const TrendSummary s3 = trendSummarize(t3.ring[0], 10, 20);
  CHECK(s3.withinPct >= 95, "เปลี่ยนเป้าแล้วอัตราตามเป้าใหม่ ต้องยังนับว่าอยู่ในช่วง (ไม่เทียบกับเป้าเก่า)");

  // ---- หลายเตียงไม่ปนกัน และเตียงที่ไม่เคยได้ยินได้แต่ 'ไม่มีข้อมูล' ----
  TrendState t4; Bed b4[MAX_BEDS]; b4[0].cfg.targetRateHr = 60; b4[1].cfg.targetRateHr = 100; uint32_t n4 = 0, d4 = 0; double a4 = 0;
  const Seg only0[] = {{3, true, true, 60, ALERT_NONE}};
  runScenario(t4, b4, 0, only0, 1, n4, a4, d4, nullptr, 2);
  CHECK(t4.ring[0].count == 3 && t4.ring[1].count == 3, "ทุกเตียงที่เปิดใช้ได้ตัวอย่างทุกนาที");
  CHECK((t4.ring[0].newest(0).flags & TS_VALID) && !(t4.ring[1].newest(0).flags & TS_VALID), "เตียงที่ไม่เคยได้ยินต้องเป็นไม่มีข้อมูล");
  CHECK(t4.ring[1].newest(0).target == 100, "เตียงที่เงียบก็เก็บเป้าหมายไว้");

  // ---- ค่าสุดขั้วและสรุปว่าง ----
  TrendRing empty;
  const TrendSummary se = trendSummarize(empty, 60, 20);
  CHECK(se.minutes == 0 && se.avgRate == 0 && se.totalMl == 0 && se.withinPct == 0, "วงแหวนว่างต้องสรุปเป็นศูนย์ ไม่หารศูนย์");
  CHECK(trendClamp16(-5) == 0 && trendClamp16(1e9f) == 65535 && trendClamp16(59.6f) == 60, "ตัดค่าเข้าช่วง 16 บิต");
  TrendRing allGap; for (int i = 0; i < 20; i++) { TrendSample x = {}; allGap.add(x); }
  const TrendSummary sg = trendSummarize(allGap, 20, 20);
  CHECK(sg.gapMinutes == 20 && sg.validMinutes == 0 && sg.avgRate == 0 && sg.alertEpisodes == 0, "ไม่มีข้อมูลเลยต้องไม่คิดเป็นอัตราไหลศูนย์");
  // เหตุเตือนต่อเนื่องหลายนาทีนับเป็นหนึ่งครั้ง แต่คั่นด้วยช่วงปกติหรือช่วงเงียบนับเป็นครั้งใหม่
  TrendRing ep;
  const uint8_t pat[] = {0, 5, 5, 5, 0, 5, 5, 99 /*เงียบ*/, 5, 3, 3};
  for (uint8_t a2 : pat) { TrendSample x = {}; if (a2 != 99) { x.flags = TS_VALID | TS_RUNNING; x.rate = 60; x.target = 60; x.alert = a2; } ep.add(x); }
  const TrendSummary sp = trendSummarize(ep, 11, 20);
  CHECK(sp.alertEpisodes == 3, "เตือน 5 ต่อเนื่องนับหนึ่งครั้ง คั่นด้วยปกติหรือช่วงเงียบนับครั้งใหม่ (3 ครั้ง)");
  CHECK(sp.alertMinutes[ALERT_OCCLUSION] == 6 && sp.alertMinutes[ALERT_NEAR_END] == 2, "นาทีของแต่ละรหัส");

  // ---- บรรทัด CSV และชื่อวัน ----
  char line[96];
  TrendSample cs = {}; cs.flags = TS_VALID | TS_RUNNING; cs.rate = 61; cs.target = 60; cs.volMl = 120; cs.drops = 20; cs.alert = ALERT_TOO_FAST; cs.link = 90; cs.rssi = -61;
  CHECK(trendCsvLine(line, sizeof(line), 1789000000, 12345, 3, cs) > 0 && strcmp(line, "1789000000,12345,3,61,60,120,20,TOO_FAST,90,-61,1\n") == 0, "รูปแบบบรรทัด CSV");
  CHECK(trendCsvLine(line, 10, 1789000000, 1, 1, cs) == 0, "บัฟเฟอร์เล็กต้องคืน 0");
  char day[9];
  const struct { uint32_t e; int tz; const char *want; } days[] = {
    {1789000000, 420, "20260910"}, {1789000000, 0, "20260910"}, {1789000000, -60, "20260909"}, {1789072000, 420, "20260911"},
    {1600000000, 0, "20200913"}, {4100000000u, 0, "20991203"}, {1709164800, 0, "20240229"}, {1709164799, 0, "20240228"}, {1709251199, 0, "20240229"}, {1709251200, 0, "20240301"}};
  for (auto &d : days) { trendDayName(d.e, d.tz, day); CHECK(strcmp(day, d.want) == 0, "ชื่อวัน (รวมปีอธิกสุรทิน 29 ก.พ. และรอยต่อเที่ยงคืน)"); }
  CHECK(trendAlertCritical(ALERT_TOO_FAST) && trendAlertCritical(ALERT_NO_SIGNAL) && trendAlertCritical(ALERT_LOST_LINK) && !trendAlertCritical(ALERT_NEAR_END) && !trendAlertCritical(ALERT_NONE), "รหัสที่ถือเป็นเหตุวิกฤตในรายงาน");
}

int main() {
  testOnlineAndWrap(); testLostLink(); testOcclusion(); testRateDeviation(); testVolumeAlerts();
  testNoSignal(); testTrend(); testNoDropCapability(); testNewBagAndSync(); testPlausibility(); testLinkAndConflict(); testScheduler();
  if (fails) { printf("รวมล้มเหลว %d รายการ\n", fails); return 1; }
  printf("ผ่านทั้งหมด\n");
  return 0;
}
