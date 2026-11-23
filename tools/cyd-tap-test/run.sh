#!/usr/bin/env bash
# ทดสอบว่าแตะตรงปุ่มจริงทุกหน้าของ Host บอร์ด CYD ทั้งแนวนอน (TFT_ROTATION=1) และแนวตั้ง (0)
# ภาพเรนเดอร์พิสูจน์ได้แค่ว่า "วาด" ถูก ไม่ได้พิสูจน์ว่าโซนแตะตรงกับที่วาด จึงต้องมีตัวนี้
set -euo pipefail
cd "$(dirname "$0")"
SK=../../firmware/Host-CYD28
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
cp "$SK/Host-CYD28.ino" "$work/host.cpp"; cp "$SK"/*.h "$work/"
sed '/^int main(/,$d' ../cyd-preview/driver.cpp > "$work/drv.cpp"
# cases.h เดิมมี main() ของตัวเอง ให้เปลี่ยนเป็น runCases() แล้วต่อชุดทดสอบคาลิเบรตและ main ใหม่
python3 - "$work" <<'PY'
import sys
w=sys.argv[1]
src=open('cases.h',encoding='utf-8').read()
i=src.index('int main(){')
open(w+'/cases_all.h','w',encoding='utf-8').write(src[:i]+src[i:].replace('int main(){','int runCases(){',1))
PY
cat "$work/cases_all.h" calib_cases.h >> "$work/drv.cpp"
echo 'int main(){ int a = runCases(); int b = calibRun(); printf("\n%s\n", (a || b) ? "มีข้อไม่ผ่าน" : "ผ่านทุกข้อ"); return (a || b) ? 1 : 0; }' >> "$work/drv.cpp"
rc=0
for R in 1 0; do
  echo "================ TFT_ROTATION=$R ================"
  g++ -std=gnu++17 -DTFT_ROTATION=$R -I../screen-preview -I"$work" -DESP_ARDUINO_VERSION_MAJOR=3 -w -o "$work/t" "$work/drv.cpp"
  "$work/t" | grep -v "^ผ่าน:" || true
  "$work/t" >/dev/null || rc=1
done
exit $rc
