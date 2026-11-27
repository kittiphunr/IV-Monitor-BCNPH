#!/usr/bin/env bash
# ทดสอบหน้าจอและเสียงของ Host-CYD28 รุ่นเขียนใหม่ (ด่านที่ 3) แล้วเรนเดอร์ภาพทุกหน้าไป docs/screens-cyd2/
# ตรวจ: โซนแตะตรงปุ่มที่วาด ข้อความไม่ล้นจอ เส้นทางการแตะทุกหน้า จังหวะบัซเซอร์ทุกรหัสเตือน
set -euo pipefail
cd "$(dirname "$0")"
SK=../../firmware/Host-CYD28
OUT="${1:-../../docs/screens-cyd2}"
python3 ../check-ino-types.py "$SK/Host-CYD28.ino"
cp "$SK/Host-CYD28.ino" sketch.cpp
g++ -std=gnu++17 -I../screen-preview -I../cyd-bed-test -I"$SK" -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-format-truncation -o /tmp/cyd_ui_test test.cpp
rm -f sketch.cpp
timeout 60 /tmp/cyd_ui_test /tmp/cyd_ui_ops.txt
python3 ../screen-preview/render.py /tmp/cyd_ui_ops.txt "$OUT" 320 240 2
