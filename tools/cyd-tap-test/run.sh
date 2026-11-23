#!/usr/bin/env bash
# ทดสอบว่าแตะตรงปุ่มจริงทุกหน้าของ Host บอร์ด CYD ทั้งแนวนอน (TFT_ROTATION=1) และแนวตั้ง (0)
# ภาพเรนเดอร์พิสูจน์ได้แค่ว่า "วาด" ถูก ไม่ได้พิสูจน์ว่าโซนแตะตรงกับที่วาด จึงต้องมีตัวนี้
set -euo pipefail
cd "$(dirname "$0")"
SK=../../firmware/Host-CYD28
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
cp "$SK/Host-CYD28.ino" "$work/host.cpp"; cp "$SK"/*.h "$work/"
sed '/^int main(/,$d' ../cyd-preview/driver.cpp > "$work/drv.cpp"
cat cases.h >> "$work/drv.cpp"
rc=0
for R in 1 0; do
  echo "================ TFT_ROTATION=$R ================"
  g++ -std=gnu++17 -DTFT_ROTATION=$R -I../screen-preview -I"$work" -DESP_ARDUINO_VERSION_MAJOR=3 -w -o "$work/t" "$work/drv.cpp"
  "$work/t" | grep -v "^ผ่าน:" || true
  "$work/t" >/dev/null || rc=1
done
exit $rc
