#!/usr/bin/env bash
# ทดสอบการแตะของ Host-CYD-LVGL: (1) ชุดเดิมของสาย Host-CYD28 (handle*Tap ต้องไม่เปลี่ยนพฤติกรรม)
# (2) เส้นทางใหม่ LVGL -> dispatchTap ผ่านตัวแทน LVGL (ไม่ใช่ LVGL จริง)
set -euo pipefail
cd "$(dirname "$0")"
SK=../../firmware/Host-CYD-LVGL
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
cp "$SK/Host-CYD-LVGL.ino" "$work/host.cpp"; cp "$SK"/*.h "$work/"
n=$(grep -n '#include "host.cpp"' ../lvgl-preview/driver.cpp | cut -d: -f1)
head -$((n-1)) ../lvgl-preview/driver.cpp > "$work/drv.cpp"
echo '#include "host.cpp"' >> "$work/drv.cpp"
python3 - "$work" <<'PY'
import sys,re
w=sys.argv[1]
# cases.h เดิมจบด้วย main() ให้ตัดออกแล้วห่อเป็นฟังก์ชัน runCyd()
src=open('../cyd-tap-test/cases.h',encoding='utf-8').read()
i=src.index('int main(){')
body=src[:i]
main=src[i:]
main=main.replace('int main(){','int runCyd(){',1).replace('g_ops=fopen("/dev/null","w"); setup();','g_ops=fopen("/dev/null","w"); setup();')
open(w+'/cases_all.h','w',encoding='utf-8').write(body+main)
PY
cat "$work/cases_all.h" >> "$work/drv.cpp"
cat extra.h >> "$work/drv.cpp"
echo 'int main(){ int a = runCyd(); int b = lvRun(); printf("\n%s\n", (a || b) ? "มีข้อไม่ผ่าน" : "ผ่านทุกข้อ"); return (a || b) ? 1 : 0; }' >> "$work/drv.cpp"
g++ -std=gnu++17 -I../screen-preview -I"$work" -DESP_ARDUINO_VERSION_MAJOR=3 -w -o "$work/t" "$work/drv.cpp"
"$work/t" | grep -v "^ผ่าน:" || true
"$work/t" >/dev/null
