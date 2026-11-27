#!/usr/bin/env bash
# ทดสอบปุ่มเดียวและบัซเซอร์ของ Host-TFT28 ทั้ง Passive (BUZZER_ACTIVE=0) และ Active (=1)
# ป้อนระดับขาปุ่มตามจังหวะเวลาในตัวจำลอง แล้วตรวจสถานะและเสียงที่ออกที่ขาบัซเซอร์ ไม่ได้ตรวจฮาร์ดแวร์จริง
set -euo pipefail
cd "$(dirname "$0")"
SK="${1:-../../firmware/Host-TFT28}"      # รับโฟลเดอร์สเก็ตช์อื่นได้ (ใช้ทำ negative control)
DRV=../host-preview/driver.cpp
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
cp "$SK/Host-TFT28.ino" "$work/host.cpp"; for h in "$SK"/*.h; do cp "$h" "$work/"; done
n=$(grep -n '#include "host.cpp"' "$DRV" | head -1 | cut -d: -f1)
m=$(grep -n '^int main' "$DRV" | head -1 | cut -d: -f1)
python3 - "$work" "$n" "$m" "$DRV" <<'PY'
import sys
w,n,m,drv=sys.argv[1],int(sys.argv[2]),int(sys.argv[3]),sys.argv[4]
L=open(drv,encoding='utf-8').read().split('\n')
before='\n'.join(L[:n-1])
before=before.replace('void pinMode(int,int){} void digitalWrite(int,int){} int digitalRead(int){return 1;}','void pinMode(int,int){}')
before=before.replace('void tone(int,unsigned int,unsigned long){} void noTone(int){}','')
before=before.replace('void esp_deep_sleep_start(){}','')
after='\n'.join(L[n:m-1])
cases=open('cases.h',encoding='utf-8').read()
hooks='''
// ---- ตัวจับปุ่ม (ขา 4) และบัซเซอร์ (ขา 7) ----
extern int g_btn; extern unsigned long g_pressStart; extern int g_sleepCalls;
void bzTone(int hz, unsigned long ms); void bzStopEv(); void bzLevel(bool on);
int digitalRead(int pin) {
  if (pin == 4) {
    if (g_btn == 0 && g_millis - g_pressStart > 2600) return 1;   // ผู้ใช้ปล่อยปุ่มหลังเสียงปิดเครื่องจบ (กันวนรอไม่รู้จบ)
    return g_btn;
  }
  return 1;
}
void digitalWrite(int pin, int v) { if (pin == 7) bzLevel(v == 1); }
void tone(int pin, unsigned int hz, unsigned long ms) { if (pin == 7) bzTone((int)hz, ms); }
void noTone(int pin) { if (pin == 7) bzStopEv(); }
void esp_deep_sleep_start() { g_sleepCalls++; }
'''
out=before+'\n'+hooks+'\n#include "host.cpp"\n'+after+'\n'+cases+'\nint main(){ int r = bzRun(); printf("\\n%s\\n", r ? "มีข้อไม่ผ่าน" : "ผ่านทุกข้อ"); return r; }\n'
open(w+'/drv.cpp','w',encoding='utf-8').write(out)
PY
rc=0
for A in 0 1; do
  echo "================ Host-TFT28  BUZZER_ACTIVE=$A ================"
  g++ -std=gnu++17 -DBUZZER_ACTIVE=$A -I../screen-preview -I"$work" -DESP_ARDUINO_VERSION_MAJOR=3 -w -o "$work/t" "$work/drv.cpp"
  if [ -n "${BZ_VERBOSE:-}" ]; then "$work/t" || true; else "$work/t" | grep -v "^ผ่าน:" || true; fi
  "$work/t" >/dev/null || rc=1
done
exit $rc
