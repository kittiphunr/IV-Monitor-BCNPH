#pragma once
// สตับ HTTPClient ฝั่ง PC — ไม่ได้ต่อเครือข่ายจริง
// การทดสอบเก็บเนื้อคำขอล่าสุดไว้ใน g_httpLastBody และกำหนดคำตอบเองผ่าน g_httpReply / g_httpCode
// เพื่อให้ตรวจเส้นทางส่งขึ้นพีซีของเฟิร์มแวร์ได้โดยไม่ต้องมีเครือข่าย
#include "Arduino.h"
#include "WiFi.h"
inline String g_httpLastBody, g_httpLastUrl, g_httpLastToken, g_httpReply;
inline int    g_httpCode = 200;
inline int    g_httpPosts = 0;
class HTTPClient {
public:
  bool begin(WiFiClient&, const String& url) { g_httpLastUrl = url; return true; }
  void setReuse(bool) {}
  void setConnectTimeout(int) {}
  void setTimeout(int) {}
  void addHeader(const char* k, const String& v) { if (String(k) == String("X-Gateway-Token")) g_httpLastToken = v; }
  int  POST(const String& body) { g_httpLastBody = body; g_httpPosts++; return g_httpCode; }
  String getString() { return g_httpReply; }
  void end() {}
};
