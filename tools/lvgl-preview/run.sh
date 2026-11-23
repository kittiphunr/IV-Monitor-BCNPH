#!/usr/bin/env bash
# คอมไพล์ Host-CYD-LVGL กับตัวแทน LVGL บน PC แล้วเรนเดอร์ภาพหน้าจอทุกหน้า (320x240)
# ตัวแทน LVGL ไม่ใช่ของจริง (ดู tools/screen-preview/lvgl.h) ภาพนี้ใช้ตรวจผัง ตำแหน่ง ซ้อนทับ และสี
# ไม่ได้ตรวจหน้าตาฟอนต์ Montserrat และไม่พิสูจน์ว่า API ของ LVGL จริงเรียกถูก
set -euo pipefail
cd "$(dirname "$0")"
OUT="${1:-../../docs/screens-lvgl}"
SKETCH_DIR="${2:-../../firmware/Host-CYD-LVGL}"
python3 ../check-ino-types.py "$SKETCH_DIR/$(basename "$SKETCH_DIR").ino"
cp "$SKETCH_DIR/$(basename "$SKETCH_DIR").ino" host.cpp
for h in "$SKETCH_DIR"/*.h; do [ -e "$h" ] && cp "$h" "$(basename "$h")"; done
g++ -std=gnu++17 -I../screen-preview -DESP_ARDUINO_VERSION_MAJOR=3 -Wall -Wno-unused-variable -Wno-format-truncation -o lvglpreview driver.cpp
./lvglpreview ops.txt
python3 ../screen-preview/render.py ops.txt "$OUT" 320 240 3
rm -f host.cpp web_dashboard.h HostScreen.h lvglpreview ops.txt
python3 make_overview.py "$OUT"
echo "เสร็จแล้ว: ภาพอยู่ใน $OUT"
