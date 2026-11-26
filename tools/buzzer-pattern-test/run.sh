#!/usr/bin/env bash
# ทดสอบจังหวะเสียงเตือนของ Station (สายหลัก) ทั้งโหมด Passive (ค่าปริยาย ต้องเหมือนรุ่นก่อน) และ Active (BUZZER_ACTIVE=1)
# จับการเปิด/ปิดขาบัซเซอร์ในตัวจำลอง จึงตรวจจังหวะได้ ไม่ได้ตรวจความดังหรือชนิดบัซเซอร์จริง
set -euo pipefail
cd "$(dirname "$0")"
SK=../../firmware/Station
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
cp "$SK/Station.ino" "$work/station.cpp"; for h in "$SK"/*.h; do cp "$h" "$work/"; done
n=$(grep -n '#include "station.cpp"' ../station-preview/driver.cpp | head -1 | cut -d: -f1)
head -$((n-1)) ../station-preview/driver.cpp \
  | sed 's/^void pinMode(int,int){} void digitalWrite(int,int){}$/void pinMode(int,int){}/' \
  | sed 's/^void tone(int,unsigned int,unsigned long){} void noTone(int){}$//' > "$work/drv.cpp"
cat >> "$work/drv.cpp" <<'CPP'
// ตัวจับเหตุการณ์ขาบัซเซอร์ (ขา 3) แทนตัวจำลองว่าง ๆ ของตัวเรนเดอร์
#include <vector>
struct BzEv; extern std::vector<struct BzEv> g_bz;
void bzPush(bool on, int hz);
void digitalWrite(int pin, int v) { if (pin == 3) bzPush(v == 1, 0); }
void tone(int pin, unsigned int hz, unsigned long) { if (pin == 3) bzPush(true, (int)hz); }
void noTone(int pin) { if (pin == 3) bzPush(false, 0); }
#include "station.cpp"
void bzPush(bool on, int hz) { g_bz.push_back({g_millis, on, hz}); }
CPP
# ส่วนของตัวจำลองที่อยู่หลัง include (digitalRead ฯลฯ) ถึงก่อน main() ของตัวเรนเดอร์
m=$(grep -n '^int main' ../station-preview/driver.cpp | head -1 | cut -d: -f1)
sed -n "$((n+1)),$((m-1))p" ../station-preview/driver.cpp >> "$work/drv.cpp"
python3 - "$work" <<'PY'
import sys
w=sys.argv[1]
t=open('cases.h',encoding='utf-8').read()
# g_bz ต้องประกาศก่อนใช้ใน bzPush: ย้ายนิยามชนิดและตัวแปรขึ้นก่อน station.cpp
head,rest=t.split('static int bzBad = 0;',1)
open(w+'/types.h','w',encoding='utf-8').write(head.replace('static std::vector<BzEv> g_bz;','std::vector<BzEv> g_bz;')+'\n')
open(w+'/body.h','w',encoding='utf-8').write('static int bzBad = 0;'+rest)
PY
python3 - "$work" <<'PY'
import sys,re
w=sys.argv[1]
d=open(w+'/drv.cpp',encoding='utf-8').read()
types=open(w+'/types.h',encoding='utf-8').read()
d=d.replace('struct BzEv; extern std::vector<struct BzEv> g_bz;\nvoid bzPush(bool on, int hz);',types+'void bzPush(bool on, int hz);')
d+=open(w+'/body.h',encoding='utf-8').read()
d+='\nint main(){ int r = bzRun(); printf("\\n%s\\n", r ? "มีข้อไม่ผ่าน" : "ผ่านทุกข้อ"); return r; }\n'
open(w+'/drv.cpp','w',encoding='utf-8').write(d)
PY
rc=0
for A in 0 1; do
  echo "================ BUZZER_ACTIVE=$A ================"
  g++ -std=gnu++17 -DBUZZER_ACTIVE=$A -I../screen-preview -I"$work" -DESP_ARDUINO_VERSION_MAJOR=3 -w -o "$work/t" "$work/drv.cpp"
  "$work/t" | grep -v "^ผ่าน:" || true
  "$work/t" >/dev/null || rc=1
done
exit $rc
