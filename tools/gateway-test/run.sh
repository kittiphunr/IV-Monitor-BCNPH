#!/usr/bin/env bash
# ทดสอบเฟิร์มแวร์ Host-Gateway บนเครื่อง PC: ชุดทดสอบภายใน และทดสอบคู่กับเซิร์ฟเวอร์พีซีตัวจริง
set -euo pipefail
cd "$(dirname "$0")"
SRC="${1:-../../firmware/Host-Gateway}"
NAME="$(basename "$SRC")"
cp "$SRC/$NAME.ino" gateway.cpp
for h in "$SRC"/*.h; do [ -e "$h" ] && cp "$h" "$(basename "$h")"; done
trap 'rm -f gateway.cpp HostScreen.h gw' EXIT
g++ -std=gnu++17 -I../screen-preview -I. -DESP_ARDUINO_VERSION_MAJOR=3 -Wall -Wno-unused-variable \
    -Wno-format-truncation -o gw driver.cpp
./gw unit
echo
echo "== เฟิร์มแวร์จริง กับ เซิร์ฟเวอร์พีซีจริง =="
python3 -I e2e.py ./gw 2>/dev/null || python3 e2e.py ./gw
