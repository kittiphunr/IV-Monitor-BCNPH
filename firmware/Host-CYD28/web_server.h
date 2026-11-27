/**
 * @file      web_server.h
 * @brief     เซิร์ฟเวอร์เว็บของ Host (จุดเชื่อมต่อ Wi-Fi ของตัวเอง ไม่มีอินเทอร์เน็ต)
 * @version   2.2.0-CYD
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
 * | 2.2.0-CYD | 2026-11-27 | GET /api/trend (กราฟ) · GET /api/report (สรุป) · GET /api/trend.csv (ดาวน์โหลดจากหน่วยความจำ ต้องล็อกอิน ส่งเป็นก้อนๆ) · GET /api/sd (รายการไฟล์) · GET /api/sdfile (ดาวน์โหลดไฟล์จาก SD ต้องล็อกอิน ชื่อไฟล์ตรวจเข้มงวดกัน path traversal) · /api/time รับ tz |
 * | 2.1.0-CYD | 2026-11-27 | POST /api/name (ชื่อผู้ป่วย) · GET /api/wifi · POST /api/wifi (ตั้งค่า Wi-Fi/บัญชีเว็บ ต้องล็อกอิน มีผลหลังรีสตาร์ท) · ล็อกอินใช้บัญชีจาก NVS แทนค่าคงที่ |
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
  if (server.authenticate(netCfg.webUser, netCfg.webPass)) return false;
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

static void hName() {
  if (webNeedAuth()) return;
  int idx;
  ApiErr e = apiParseBed(server.arg("bed").c_str(), activeBeds, idx);
  if (e != API_OK) { sendErr(e); return; }
  if (!server.hasArg("name")) { sendErr(API_NO_FIELDS); return; }
  char clean[BED_NAME_MAX + 1];
  e = apiParseName(server.arg("name").c_str(), clean);
  if (e != API_OK) { sendErr(e); return; }
  commitBedName(idx, clean);
  sendBody(200, "text/plain", "ok");
}

static void hWifiGet() {
  char ip[20];
  snprintf(ip, sizeof(ip), "%s", WiFi.softAPIP().toString().c_str());
  const ApiErr e = apiWifiJson(webBuf, sizeof(webBuf), netCfg, ip, WiFi.softAPgetStationNum());
  if (e != API_OK) { sendErr(e); return; }
  sendBody(200, "application/json", webBuf);
}

// ตั้งค่า Wi-Fi/บัญชีเว็บ: ตรวจทั้งชุด เก็บลง NVS แล้วรีสตาร์ทเอง (ค่าใหม่มีผลหลังรีสตาร์ท) ไม่รีสตาร์ททันทีเพื่อให้ตอบกลับเบราว์เซอร์ได้ก่อน
static void hWifiPost() {
  if (webNeedAuth()) return;
  NetConfig n = netCfg;
  const ApiErr e = apiParseWifiUpdate(server.arg("ssid").c_str(), server.arg("pass").c_str(), server.arg("webuser").c_str(), server.arg("webpass").c_str(), n);
  if (e != API_OK) { sendErr(e); return; }
  netSave(n);
  logEvent(0, EV_CONFIG);
  restartAtMs = millis() + 2500;
  restartPending = true;
  sendBody(200, "text/plain", "saved, restarting");
}

// ---- กราฟแนวโน้มและรายงานย้อนหลัง ----
// อ่านเลขเตียงและจำนวนนาที (ค่าผิดตอบ 400 ไม่เดา) คืน false ถ้าตอบไปแล้ว
static bool webBedMins(int dfltMins, int &idx, int &mins) {
  ApiErr e = apiParseBed(server.arg("bed").c_str(), activeBeds, idx);
  if (e == API_OK) e = apiParseMins(server.arg("mins").c_str(), dfltMins, mins);
  if (e != API_OK) { sendErr(e); return false; }
  return true;
}

static void hTrend() {
  int idx, mins;
  if (!webBedMins(60, idx, mins)) return;
  const ApiErr e = apiTrendJson(webBuf, sizeof(webBuf), trend.ring[idx], idx + 1, mins);
  if (e != API_OK) { sendErr(e); return; }
  sendBody(200, "application/json", webBuf);
}

static void hReport() {
  int idx, mins;
  if (!webBedMins(60, idx, mins)) return;
  const TrendSummary s = trendSummarize(trend.ring[idx], mins, beds[idx].cfg.dropFactor);
  const ApiErr e = apiReportJson(webBuf, sizeof(webBuf), s, idx + 1, mins);
  if (e != API_OK) { sendErr(e); return; }
  sendBody(200, "application/json", webBuf);
}

// ดาวน์โหลดรายงานรายนาทีจากหน่วยความจำ (เรียงเก่า -> ใหม่) ส่งเป็นก้อนละไม่เกิน ~1 KB ไม่ต้องใช้บัฟเฟอร์ใหญ่ ต้องล็อกอินเพราะเป็นข้อมูลผู้ป่วย
static void hTrendCsv() {
  if (webNeedAuth()) return;
  int idx, mins;
  if (!webBedMins(TREND_MINUTES, idx, mins)) return;
  const TrendRing &r = trend.ring[idx];
  const int n = mins > r.count ? r.count : mins;
  const uint32_t now = millis();
  const uint32_t epochNow = clockEpoch();
  const uint32_t sinceBoundary = (now - trend.minuteStartMs) / 1000;
  char name[40];
  snprintf(name, sizeof(name), "attachment; filename=\"bed%d-trend.csv\"", idx + 1);
  server.sendHeader("Content-Disposition", name);
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "");
  server.sendContent(TREND_CSV_HEADER);
  char chunk[1024];
  size_t len = 0;
  for (int i = n - 1; i >= 0; i--) {
    const TrendSample &x = r.newest(i);
    if (!(x.flags & TS_VALID)) continue;
    const uint32_t ago = (uint32_t)i * 60 + sinceBoundary;
    char line[96];
    if (!trendCsvLine(line, sizeof(line), epochNow ? epochNow - ago : 0, trend.minuteStartMs / 60000 - (uint32_t)i, idx + 1, x)) continue;
    const size_t ll = strlen(line);
    if (len + ll >= sizeof(chunk)) { server.sendContent(chunk); len = 0; }
    memcpy(chunk + len, line, ll); len += ll; chunk[len] = 0;
  }
  if (len) server.sendContent(chunk);
}

static void hSdInfo() {
  char days[40][9];
  const int nd = sdListDays(days, 40);
  JsonOut j(webBuf, sizeof(webBuf));
  j.fmt("{\"ok\":%s,\"days\":[", sdOk ? "true" : "false");
  for (int i = 0; i < nd; i++) { j.fmt("%s", i ? "," : ""); j.str(days[i]); }
  j.fmt("]}");
  if (j.over) { sendErr(API_OVERFLOW); return; }
  sendBody(200, "application/json", webBuf);
}

// ดาวน์โหลดไฟล์จาก SD: file=events หรือ file=trend&day=YYYYMMDD|unsync ชื่อไฟล์สร้างจากค่าที่ตรวจแล้วเท่านั้น ไม่เคยใช้ข้อความจากผู้ใช้เป็นพาธ
static void hSdFile() {
  if (webNeedAuth()) return;
  if (!sdOk) { sendBody(503, "text/plain", "no sd card"); return; }
  char path[32];
  const String file = server.arg("file");
  if (file == "events") snprintf(path, sizeof(path), "/events.csv");
  else if (file == "trend") {
    const String day = server.arg("day");
    bool ok = day == "unsync";
    if (day.length() == 8) { ok = true; for (int i = 0; i < 8; i++) if (day.c_str()[i] < '0' || day.c_str()[i] > '9') ok = false; }
    if (!ok) { sendErr(API_BAD_VALUE); return; }
    snprintf(path, sizeof(path), "/trend/%s.csv", day.c_str());
  } else { sendErr(API_BAD_VALUE); return; }
  File f = SD.open(path, FILE_READ);
  if (!f) { sendBody(404, "text/plain", "not found"); return; }
  server.sendHeader("Cache-Control", "no-store");
  server.streamFile(f, "text/csv");
  f.close();
}

static void hTime() {
  if (clockSynced && webNeedAuth()) return;
  uint32_t epoch;
  const ApiErr e = apiParseEpoch(server.arg("epoch").c_str(), epoch);
  if (e != API_OK) { sendErr(e); return; }
  int tz = 0;                                                            // เขตเวลาเป็นนาที ไม่ส่งมา = ไม่เปลี่ยน ถ้าส่งต้องอยู่ใน -840..840
  if (apiHas(server.arg("tz").c_str())) {
    double tv;
    if (!apiParseNumber(server.arg("tz").c_str(), tv) || tv != floor(tv) || tv < -840 || tv > 840) { sendErr(API_BAD_VALUE); return; }
    tz = (int)tv;
    clockTzMin = tz;
  }
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
  server.on("/api/trend", hTrend);
  server.on("/api/report", hReport);
  server.on("/api/trend.csv", hTrendCsv);
  server.on("/api/sd", hSdInfo);
  server.on("/api/sdfile", hSdFile);
  server.on("/api/name", HTTP_POST, hName);
  server.on("/api/wifi", hWifiGet);
  server.on("/api/wifi", HTTP_POST, hWifiPost);
  server.onNotFound(hNotFound);
  server.begin();
}

static void webService() { server.handleClient(); }
