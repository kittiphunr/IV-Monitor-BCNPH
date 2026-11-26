#!/usr/bin/env bash
# ทดสอบจังหวะเสียงเตือนของ Station สายหลักและสายเสถียร (Station-Stable) ทั้งโหมด Passive และ Active
#   Passive (ค่าปริยาย) ต้องให้โทนและจังหวะเท่ารุ่นก่อนทุกค่า · Active (BUZZER_ACTIVE=1) ต้องแยกสัญญาณทั้งสี่ได้ด้วยจังหวะ
# จับการเปิด/ปิดขาบัซเซอร์ในตัวจำลอง จึงตรวจจังหวะได้ ไม่ได้ตรวจความดังหรือชนิดบัซเซอร์จริง
set -euo pipefail
cd "$(dirname "$0")"
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
DRV=../station-preview/driver.cpp
n=$(grep -n '#include "station.cpp"' "$DRV" | head -1 | cut -d: -f1)    # ตัวจำลองส่วนก่อน include
m=$(grep -n '^int main' "$DRV" | head -1 | cut -d: -f1)                  # ตัวจำลองส่วนหลัง include ถึงก่อน main()

build() {   # $1 = ชื่อสายใน firmware/  $2 = BUZZER_ACTIVE (0/1)
  local sk="../../firmware/$1"
  rm -rf "$work"/*
  cp "$sk/$1.ino" "$work/station.cpp"; for h in "$sk"/*.h; do cp "$h" "$work/"; done
  python3 - "$work" "$n" "$m" "$DRV" <<'PY'
import sys
w,n,m,drv=sys.argv[1],int(sys.argv[2]),int(sys.argv[3]),sys.argv[4]
L=open(drv,encoding='utf-8').read().split('\n')
before='\n'.join(L[:n-1]).replace('void pinMode(int,int){} void digitalWrite(int,int){}','void pinMode(int,int){}')
before=before.replace('void tone(int,unsigned int,unsigned long){} void noTone(int){}','')
after='\n'.join(L[n:m-1])
cases=open('cases.h',encoding='utf-8').read()
head,rest=cases.split('static int bzBad = 0;',1)
types=head.replace('static std::vector<BzEv> g_bz;','std::vector<BzEv> g_bz;')
out=before+'\n'+types+'''
// ตัวจับเหตุการณ์ขาบัซเซอร์ (ขา 3) แทนตัวจำลองว่าง ๆ ของตัวเรนเดอร์
void bzPush(bool on, int hz) { g_bz.push_back({g_millis, on, hz}); }
void digitalWrite(int pin, int v) { if (pin == 3) bzPush(v == 1, 0); }
void tone(int pin, unsigned int hz, unsigned long) { if (pin == 3) bzPush(true, (int)hz); }
void noTone(int pin) { if (pin == 3) bzPush(false, 0); }
#include "station.cpp"
'''+after+'\nstatic int bzBad = 0;'+rest+'\nint main(){ int r = bzRun(); printf("\\n%s\\n", r ? "มีข้อไม่ผ่าน" : "ผ่านทุกข้อ"); return r; }\n'
open(w+'/drv.cpp','w',encoding='utf-8').write(out)
PY
  local extra=""; [ "$1" = "Station-Stable" ] && extra="-DHAS_CALBEEP=1"      # สายเสถียรมี calBeep() เฉพาะตัว
  g++ -std=gnu++17 -DBUZZER_ACTIVE="$2" $extra -I../screen-preview -I"$work" -DESP_ARDUINO_VERSION_MAJOR=3 -w -o "$work/t" "$work/drv.cpp"
}

rc=0
for LINE in Station Station-Stable; do
  for A in 0 1; do
    echo "================ $LINE  BUZZER_ACTIVE=$A ================"
    build "$LINE" "$A"
    if [ -n "${BZ_VERBOSE:-}" ]; then "$work/t" || true; else "$work/t" | grep -v "^ผ่าน:" || true; fi
    "$work/t" >/dev/null || rc=1
  done
done
exit $rc
