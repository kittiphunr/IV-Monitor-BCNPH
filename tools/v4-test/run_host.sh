#!/usr/bin/env bash
# ทดสอบ Host-TFT-Gateway บนเครื่อง PC: รับ Protocol v4 คำนวณอัตราไหล ตัดสินเตือน ส่งผลกลับให้ Station และรับคำสั่งจากพีซี
set -euo pipefail
cd "$(dirname "$0")"
SKETCH=../../firmware/Host-TFT-Gateway
python3 ../check-ino-types.py "$SKETCH/Host-TFT-Gateway.ino" >/dev/null
cp "$SKETCH/Host-TFT-Gateway.ino" host_tg.cpp
for h in "$SKETCH"/*.h; do cp "$h" "$(basename "$h")"; done
trap 'rm -f host_tg.cpp HostScreen.h protocol_v4.h rate_estimator.h battery_check.h hosttg' EXIT
g++ -std=gnu++17 -I../screen-preview -I. -DESP_ARDUINO_VERSION_MAJOR=3 -Wall -Wno-unused-variable \
    -Wno-format-truncation -o hosttg host_driver.cpp
./hosttg
