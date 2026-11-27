#pragma once
// ตัวแทน WebServer สำหรับทดสอบ: บันทึกเส้นทางที่ลงทะเบียน แล้วให้เทสต์เรียกตัวจัดการได้เหมือนมีคำขอ HTTP จริง
// (บังคับล็อกอินแบบ Basic ตามชื่อผู้ใช้/รหัสผ่านที่เทสต์กำหนด) วางไว้หน้า tools/screen-preview ในลำดับ -I จึงทับตัวเดิม
#include <map>
#include <string>
#include "Arduino.h"
#define HTTP_GET 1
#define HTTP_POST 2

struct WebTestState {
  std::map<std::string, void (*)()> routes[3];
  void (*notFound)() = nullptr;
  std::map<std::string, std::string> args;
  std::string user, pass;
  int code = 0;
  std::string type, body;
  bool begun = false;
  int authRequests = 0;
};
inline WebTestState g_web;

class WebServer {
 public:
  WebServer(int) {}
  void on(const char *p, void (*f)()) { g_web.routes[HTTP_GET][p] = f; }
  void on(const char *p, int m, void (*f)()) { g_web.routes[m][p] = f; }
  void onNotFound(void (*f)()) { g_web.notFound = f; }
  void begin() { g_web.begun = true; }
  void stop() {}
  void handleClient() {}
  bool authenticate(const char *u, const char *p) { return g_web.user == u && g_web.pass == p; }
  void requestAuthentication() { g_web.code = 401; g_web.body = "auth"; g_web.authRequests++; }
  bool hasArg(const char *n) { return g_web.args.count(n) > 0; }
  String arg(const char *n) { auto it = g_web.args.find(n); return String(it == g_web.args.end() ? "" : it->second.c_str()); }
  void send(int c, const char *t, const String &b) { g_web.code = c; g_web.type = t; g_web.body = b.c_str(); }
  void send(int c, const char *t, const char *b) { g_web.code = c; g_web.type = t; g_web.body = b; }
  void send_P(int c, const char *t, const char *b) { g_web.code = c; g_web.type = t; g_web.body = b; }
  void sendHeader(const char *, const String &) {}
};

// เรียกคำขอหนึ่งครั้ง
inline int webCall(int method, const char *path, std::map<std::string, std::string> args = {}, const char *user = "", const char *pass = "") {
  g_web.args = args; g_web.user = user; g_web.pass = pass; g_web.code = 0; g_web.type.clear(); g_web.body.clear();
  auto it = g_web.routes[method].find(path);
  if (it != g_web.routes[method].end()) it->second(); else if (g_web.notFound) g_web.notFound();
  return g_web.code;
}
