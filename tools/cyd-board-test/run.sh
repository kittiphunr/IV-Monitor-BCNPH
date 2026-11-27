#!/usr/bin/env bash
# ทดสอบฐานบอร์ด Host-CYD28 รุ่นเขียนใหม่ (ด่านที่ 1): คาลิเบรต affine, ไดรเวอร์ทัช, ตัวติดตามนิ้ว
# และรันสเก็ตช์ทั้งไฟล์กับแบบจำลองชิปทัชที่แตะเป้าหมายคาลิเบรตเอง ทั้งจอ ILI9341 และ ST7789
set -euo pipefail
cd "$(dirname "$0")"
SK=../../firmware/Host-CYD28
python3 ../check-ino-types.py "$SK/Host-CYD28.ino"
cp "$SK/Host-CYD28.ino" sketch.cpp
for P in 9341 7789; do
  g++ -std=gnu++17 -DCYD_PANEL=$P -I../screen-preview -I"$SK" -Wall -Wextra -Wno-unused-parameter -Wno-unused-function \
      -o /tmp/cyd_board_test_$P test.cpp
  timeout 20 /tmp/cyd_board_test_$P
done
rm -f sketch.cpp
