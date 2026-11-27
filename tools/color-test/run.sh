#!/usr/bin/env bash
# ตรวจ CYD-ColorTest: คอมไพล์กับตัวแทนไลบรารี รันทุกคำสั่ง ทั้งสองชิป ทุกชุดค่า ข้อความไม่ล้นจอ และเรนเดอร์ภาพหน้าทดสอบ
set -euo pipefail
cd "$(dirname "$0")"
SK=../../firmware/CYD-ColorTest
OUT="${1:-../../docs/screens-colortest}"
python3 ../check-ino-types.py "$SK/CYD-ColorTest.ino"
cp "$SK/CYD-ColorTest.ino" sketch.cpp
g++ -std=gnu++17 -I../screen-preview -Wall -Wextra -Wno-unused-parameter -o /tmp/cyd_colortest test.cpp
rm -f sketch.cpp
timeout 30 /tmp/cyd_colortest /tmp/cyd_colortest_ops.txt
python3 ../screen-preview/render.py /tmp/cyd_colortest_ops.txt "$OUT" 320 240 2
