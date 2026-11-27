/**
 * @file      web_server.h
 * @brief     เซิร์ฟเวอร์เว็บของ Host (จุดเชื่อมต่อ Wi-Fi ของตัวเอง ไม่มีอินเทอร์เน็ต)
 * @version   2.0.3-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * ตัวต่อบางๆ ระหว่าง WebServer กับ web_api.h: ดูข้อมูลได้ไม่ต้องล็อกอิน (GET /api/state, /api/events) ส่วนการแก้ค่า เริ่มถุงใหม่ รับทราบ ต้องล็อกอินแบบ Basic เสมอ เริ่มถุงใหม่ต้องส่ง confirm=1 อีกชั้นกันสั่งพลาด ตั้งเวลาไม่ต้องล็อกอินเฉพาะตอนที่ Host ยังไม่เคยตั้งเวลา (หน้าเว็บส่งให้เองครั้งแรก) หลังจากนั้นต้องล็อกอิน
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.3-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 4 (เว็บ นาฬิกา SD ประวัติเหตุการณ์) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ถูก `#include` ท้ายไฟล์หลักก่อน `setup()` เพราะใช้ฟังก์ชัน commitBedConfig doNewBag doAck ที่ประกาศไว้ด้านบน
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <WebServer.h>
#include "web_api.h"
#include "web_dashboard.h"

static WebServer server(80);
static char webBuf[6144];

static void sendBody(int code, const char *type, const char *body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, type, body);
}

static void sendErr(ApiErr e) { sendBody(e == API_OVERFLOW ? 500 : 400, "text/plain", apiErrText(e)); }

// true = ยังไม่ผ่านการล็อกอิน (ตอบ 401 ไปแล้ว)
static bool webNeedAuth() {
  if (server.authenticate(CYD_WEB_USER, CYD_WEB_PASS)) return false;
  server.requestAuthentication();
  return true;
}

static void hRoot() { server.send_P(200, "text/html; charset=utf-8", DASHBOARD_HTML); }

static void hState() {
  const uint32_t now = millis();
  ApiCtx c = { beds, activeBeds, now, clockEpoch(), linkChannel(), snoozed(now) ? (uint32_t)((snoozeUntil - now) / 1000) : 0,
               sdOk, rxDropped, badPackets, rxWrongLen, APP_VERSION };
  const ApiErr e = apiStateJson(webBuf, sizeof(webBuf), c);
  if (e != API_OK) { sendErr(e); return; }
  sendBody(200, "application/json", webBuf);
}

static void hEvents() {
  const ApiErr e = apiEventsJson(webBuf, sizeof(webBuf), events);
  if (e != API_OK) { sendErr(e); return; }
  sendBody(200, "application/json", webBuf);
}

static void hBed() {
  if (webNeedAuth()) return;
  int idx;
  ApiErr e = apiParseBed(server.arg("bed").c_str(), activeBeds, idx);
  if (e != API_OK) { sendErr(e); return; }
  BedConfig n = beds[idx].cfg;
  e = apiParseBedUpdate(server.arg("rate").c_str(), server.arg("plan").c_str(), server.arg("df").c_str(), server.arg("near").c_str(), n);
  if (e != API_OK) { sendErr(e); return; }
  commitBedConfig(idx, n);
  sendBody(200, "text/plain", "ok");
}

static void hNewBag() {
  if (webNeedAuth()) return;
  int idx;
  ApiErr e = apiParseBed(server.arg("bed").c_str(), activeBeds, idx);
  if (e != API_OK) { sendErr(e); return; }
  if (server.arg("confirm") != "1") { sendErr(API_NEED_CONFIRM); return; }
  doNewBag(idx);
  sendBody(200, "text/plain", "ok");
}

static void hAck() {
  if (webNeedAuth()) return;
  int idx;
  const ApiErr e = apiParseBed(server.arg("bed").c_str(), activeBeds, idx);
  if (e != API_OK) { sendErr(e); return; }
  if (!doAck(idx)) { sendBody(409, "text/plain", "nothing to acknowledge"); return; }
  sendBody(200, "text/plain", "ok");
}

static void hTime() {
  if (clockSynced && webNeedAuth()) return;
  uint32_t epoch;
  const ApiErr e = apiParseEpoch(server.arg("epoch").c_str(), epoch);
  if (e != API_OK) { sendErr(e); return; }
  clockSet(epoch);
  sendBody(200, "text/plain", "ok");
}

static void hNotFound() { sendBody(404, "text/plain", "not found"); }

static void webBegin() {
  server.on("/", hRoot);
  server.on("/api/state", hState);
  server.on("/api/events", hEvents);
  server.on("/api/bed", HTTP_POST, hBed);
  server.on("/api/newbag", HTTP_POST, hNewBag);
  server.on("/api/ack", HTTP_POST, hAck);
  server.on("/api/time", HTTP_POST, hTime);
  server.onNotFound(hNotFound);
  server.begin();
}

static void webService() { server.handleClient(); }
