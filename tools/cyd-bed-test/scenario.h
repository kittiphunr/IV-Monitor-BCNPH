// สถานการณ์จำลองรายนาทีสำหรับทดสอบกราฟแนวโน้ม: ใช้ร่วมกันระหว่าง cyd-bed-test (ตรรกะล้วน) และ cyd-web-test (เว็บ/SD)
#pragma once
#include <cmath>
#include "trend.h"

struct Seg { int minutes; bool send; bool running; double rate; uint8_t alert; };

// เล่นสถานการณ์กับเตียง idx วินาทีต่อวินาที: ส่งแพ็กเก็ตทุกวินาทีเมื่อ send (หรือเงียบเมื่อไม่ send) แล้วเรียก trendObserve/trendClose
// nowMs เดินหน้าตามเวลาที่ใช้ drops/dropAcc คือตัวนับหยดของ Station จำลอง
inline int runScenario(TrendState &t, Bed *beds, int idx, const Seg *segs, int nsegs, uint32_t &nowMs, double &dropAcc, uint32_t &drops,
                       TrendSample *lastClosed = nullptr, int active = 1) {
  int closes = 0;
  const uint8_t mac[6] = {1, 2, 3, 4, 5, 6};
  for (int s = 0; s < nsegs; s++) {
    for (int sec = 0; sec < segs[s].minutes * 60; sec++) {
      if (segs[s].send) {
        if (segs[s].running) { dropAcc += segs[s].rate * safeDropFactor(beds[idx].cfg.dropFactor) / 3600.0; drops = (uint32_t)dropAcc; }
        struct_message m = {};
        m.stationId = (uint8_t)(idx + 1); m.isRunning = segs[s].running ? 1 : 0; m.totalDrops = drops;
        m.flowRateHr = (float)(segs[s].running ? segs[s].rate : 0); m.msSinceLastDrop = 300; m.batteryVolts = 4.0f;
        bedIngest(beds[idx], m, -50, mac, nowMs);
        beds[idx].alert = segs[s].alert;
        beds[idx].linkPct = 100;
      }
      TrendSample out[MAX_BEDS];                                           // ปิดนาทีก่อนเก็บสถิติ: ข้อมูลของวินาทีที่ครบนาทีพอดีเป็นของนาทีถัดไป
      if (trendClose(t, beds, active, nowMs, out)) { closes++; if (lastClosed) memcpy(lastClosed, out, sizeof(TrendSample) * active); }
      trendObserve(t, beds, active, nowMs);
      nowMs += 1000;
    }
  }
  TrendSample out[MAX_BEDS];                                               // ปิดนาทีสุดท้ายที่เพิ่งครบพอดี
  if (trendClose(t, beds, active, nowMs, out)) { closes++; if (lastClosed) memcpy(lastClosed, out, sizeof(TrendSample) * active); }
  return closes;
}

// สถานการณ์มาตรฐาน 80 นาที (เป้าหมาย 60 mL/h df 20): นาที 0-29 ไหล 60 · 30-39 Station เงียบ · 40-49 ไหล 90 · 50-59 ไหล 90 + เตือนเร็วเกิน
// 60-69 หยุดชั่วคราว · 70-79 ไหล 60 + เตือนใกล้หมด
inline const Seg *standardScenario(int &n) {
  static const Seg segs[] = {
    {30, true, true, 60, ALERT_NONE},   {10, false, false, 0, ALERT_NONE},   {10, true, true, 90, ALERT_NONE},
    {10, true, true, 90, ALERT_TOO_FAST}, {10, true, false, 0, ALERT_NONE},  {10, true, true, 60, ALERT_NEAR_END}};
  n = (int)(sizeof(segs) / sizeof(segs[0]));
  return segs;
}
