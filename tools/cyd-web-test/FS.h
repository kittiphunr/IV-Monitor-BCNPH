#pragma once
// ระบบไฟล์ในหน่วยความจำสำหรับทดสอบ (ทับ FS.h ตัวแทนของ tools/screen-preview ในลำดับ -I) เก็บไฟล์จริงตามที่เฟิร์มแวร์เขียน
// จึงตรวจเนื้อไฟล์ CSV บนการ์ด SD ได้ และจำลองการ์ดเต็ม/ถอดการ์ดได้ด้วย failWrites
#include <stdint.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "Arduino.h"
#define FILE_READ   "r"
#define FILE_WRITE  "w"
#define FILE_APPEND "a"

namespace fs {
struct MemFS {
  std::map<std::string, std::string> files;
  std::set<std::string> dirs;
  bool cardPresent = true;
  bool failWrites = false;
  int opens = 0;
};
inline MemFS g_mem;
inline bool g_shortNames = false;            // true = name() คืนชื่อสั้น (core รุ่นใหม่) false = พาธเต็ม (core รุ่นเก่า)

class File {
 public:
  File() {}
  operator bool() const { return _ok; }
  size_t print(const char *s) { if (!_ok || _read) return 0; g_mem.files[_path] += s; return strlen(s); }
  size_t print(const String &s) { return print(s.c_str()); }
  size_t print(char c) { char b[2] = {c, 0}; return print(b); }
  size_t print(int v) { char b[24]; snprintf(b, sizeof(b), "%d", v); return print(b); }
  size_t print(unsigned int v) { char b[24]; snprintf(b, sizeof(b), "%u", v); return print(b); }
  size_t print(long v) { char b[24]; snprintf(b, sizeof(b), "%ld", v); return print(b); }
  size_t print(unsigned long v) { char b[24]; snprintf(b, sizeof(b), "%lu", v); return print(b); }
  size_t print(float v, int d = 2) { char b[32]; snprintf(b, sizeof(b), "%.*f", d, v); return print(b); }
  size_t print(double v, int d = 2) { char b[32]; snprintf(b, sizeof(b), "%.*f", d, v); return print(b); }
  size_t println(const char *s = "") { size_t n = print(s); return n + print("\n"); }
  size_t println(const String &s) { return println(s.c_str()); }
  size_t println(int v) { size_t n = print(v); return n + print("\n"); }
  size_t println(float v, int d = 2) { size_t n = print(v, d); return n + print("\n"); }
  size_t println(double v, int d = 2) { size_t n = print(v, d); return n + print("\n"); }
  size_t write(const uint8_t *b, size_t n) { if (!_ok || _read) return 0; g_mem.files[_path].append((const char *)b, n); return n; }
  void close() { _ok = false; }
  void flush() {}
  const char *name() { if (g_shortNames) { size_t p = _path.rfind('/'); _name = p == std::string::npos ? _path : _path.substr(p + 1); return _name.c_str(); } return _path.c_str(); }
  const char *path() { return _path.c_str(); }
  size_t size() { auto it = g_mem.files.find(_path); return it == g_mem.files.end() ? 0 : it->second.size(); }
  bool isDirectory() { return _dir; }
  File openNextFile(const char * = FILE_READ) {
    if (!_dir) return File();
    while (_li < _list.size()) { File f; f._path = _list[_li++]; f._ok = true; f._read = true; return f; }
    return File();
  }
  int available() { auto it = g_mem.files.find(_path); return it == g_mem.files.end() ? 0 : (int)(it->second.size() - _pos); }
  int read() { auto it = g_mem.files.find(_path); if (it == g_mem.files.end() || _pos >= it->second.size()) return -1; return (unsigned char)it->second[_pos++]; }
  size_t read(uint8_t *b, size_t n) { size_t k = 0; int c; while (k < n && (c = read()) >= 0) b[k++] = (uint8_t)c; return k; }
  void seek(uint32_t p) { _pos = p; }

  std::string _path, _name;
  bool _ok = false, _read = false, _dir = false;
  size_t _pos = 0;
  std::vector<std::string> _list;
  size_t _li = 0;
};

class FS {
 public:
  File open(const char *path, const char *mode = FILE_READ) {
    g_mem.opens++;
    File f;
    f._path = path;
    if (!g_mem.cardPresent) return f;
    const bool wr = mode[0] == 'a' || mode[0] == 'w';
    if (!wr && g_mem.dirs.count(path)) {
      f._ok = true; f._dir = true; f._read = true;
      const std::string prefix = std::string(path) + "/";
      for (auto &kv : g_mem.files) if (kv.first.compare(0, prefix.size(), prefix) == 0 && kv.first.find('/', prefix.size()) == std::string::npos) f._list.push_back(kv.first);
      return f;
    }
    if (wr) {
      if (g_mem.failWrites) return f;
      if (mode[0] == 'w') g_mem.files[path] = "";
      else if (!g_mem.files.count(path)) g_mem.files[path] = "";
      f._ok = true;
      return f;
    }
    if (g_mem.files.count(path)) { f._ok = true; f._read = true; }
    return f;
  }
  File open(const String &p, const char *m = FILE_READ) { return open(p.c_str(), m); }
  bool exists(const char *p) { return g_mem.cardPresent && (g_mem.files.count(p) || g_mem.dirs.count(p)); }
  bool exists(const String &p) { return exists(p.c_str()); }
  bool mkdir(const char *p) { if (!g_mem.cardPresent || g_mem.failWrites) return false; g_mem.dirs.insert(p); return true; }
  bool mkdir(const String &p) { return mkdir(p.c_str()); }
  bool remove(const char *p) { return g_mem.files.erase(p) > 0; }
  bool remove(const String &p) { return remove(p.c_str()); }
};
}  // namespace fs

using fs::File;
