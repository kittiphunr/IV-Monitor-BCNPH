#!/usr/bin/env bash
# ทดสอบ Station-Thin บนเครื่อง PC: พฤติกรรมของเครื่อง (ส่ง/รับ Protocol v4) และเรนเดอร์ภาพหน้าจอ
# ใช้: run_station.sh [โฟลเดอร์ภาพผลลัพธ์]   (ปริยาย docs/screens-station-thin)
set -euo pipefail
cd "$(dirname "$0")"
OUT="${1:-../../docs/screens-station-thin}"
SKETCH=../../firmware/Station-Thin
python3 ../check-ino-types.py "$SKETCH/Station-Thin.ino" >/dev/null
cp "$SKETCH/Station-Thin.ino" station_thin.cpp
for h in "$SKETCH"/*.h; do cp "$h" "$(basename "$h")"; done
trap 'rm -f station_thin.cpp StationScreen.h drop_detector.h protocol_v4.h battery_check.h stthin ops.txt' EXIT
g++ -std=gnu++17 -I../screen-preview -I. -DESP_ARDUINO_VERSION_MAJOR=3 -Wall -Wno-unused-variable \
    -Wno-format-truncation -o stthin station_driver.cpp
./stthin ops.txt
python3 ../screen-preview/render.py ops.txt "$OUT" >/dev/null
echo "ภาพหน้าจอ: $OUT"
