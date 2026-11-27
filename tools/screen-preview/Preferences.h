#pragma once
#include "Arduino.h"
#include <map>
#include <string>
#include <vector>
// ก้อนข้อมูลแบบ bytes เก็บไว้ในหน่วยความจำจริงตลอดอายุโปรเซส เพื่อทดสอบการบันทึก/โหลด/คืนค่าโรงงานได้ (เมธอดอื่นยังคืนค่าปริยายเหมือนเดิม)
inline std::map<std::string, std::vector<unsigned char>>& g_prefBlobs() { static std::map<std::string, std::vector<unsigned char>> m; return m; }
// สตับ Preferences (NVS) ฝั่ง PC — รวมทุกเมธอดที่เฟิร์มแวร์ทั้งสามตัวเรียกใช้
class Preferences {
  std::string _ns;
public:
  bool begin(const char* ns, bool = false) { _ns = ns ? ns : ""; return true; }
  void end() {} void clear() {}
  bool isKey(const char*) { return false; }
  int           getInt(const char* k, int d = 0) { return d; }
  unsigned int  getUInt(const char*, unsigned int d = 0) { return d; }
  unsigned char getUChar(const char*, unsigned char d = 0) { return d; }
  String        getString(const char*, const char* d = "") { return String(d); }
  size_t        getBytesLength(const char* k) { auto it = g_prefBlobs().find(_ns + "/" + k); return it == g_prefBlobs().end() ? 0 : it->second.size(); }
  size_t        getBytes(const char* k, void* b, size_t n) { auto it = g_prefBlobs().find(_ns + "/" + k); if (it == g_prefBlobs().end()) return 0; size_t m = it->second.size() < n ? it->second.size() : n; memcpy(b, it->second.data(), m); return m; }
  bool remove(const char* k) { return g_prefBlobs().erase(_ns + "/" + k) > 0; }
  void putInt(const char*, int) {}
  void putUInt(const char*, unsigned int) {}
  void putUChar(const char*, unsigned char) {}
  void putString(const char*, const String&) {}
  void putBytes(const char* k, const void* b, size_t n) { g_prefBlobs()[_ns + "/" + k].assign((const unsigned char*)b, (const unsigned char*)b + n); }
};
