#!/usr/bin/env bash
# ตรวจสูตรเปอร์เซ็นต์แบตเตอรี่ของ Host ทั้งสามรุ่นจอ (calculateBatteryPct ตาราง OCV)
#   1) ตัดตารางและฟังก์ชันออกมาจากไฟล์ .ino ตัวจริง ไม่ได้ลอกมาเขียนใหม่
#   2) ทั้งสามรุ่นต้องเหมือนกันทุกบรรทัด
#   3) คอมไพล์ด้วย g++ แล้วเทียบค่าที่จุดสำคัญ และตรวจว่าไม่ลดลงเมื่อแรงดันเพิ่ม
set -euo pipefail
cd "$(dirname "$0")"
ROOT=../..
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT

extract() {  # ตั้งแต่ BATT_OCV_V จนจบฟังก์ชัน calculateBatteryPct
  awk '/^static const float   BATT_OCV_V\[\]/{on=1} on{print} on&&/^}/{exit}' "$1"
}
for h in Host-OLED Host-TFT28 Host-Touch24; do
  extract "$ROOT/firmware/$h/$h.ino" > "$TMP/$h.cpp"
  [ -s "$TMP/$h.cpp" ] || { echo "ไม่พบตาราง OCV ใน $h.ino"; exit 1; }
done
for h in Host-TFT28 Host-Touch24; do
  if ! diff -u "$TMP/Host-OLED.cpp" "$TMP/$h.cpp"; then
    echo "FAIL: calculateBatteryPct ของ $h ไม่ตรงกับ Host-OLED"; exit 1
  fi
done
echo "ตาราง OCV และ calculateBatteryPct ตรงกันทั้งสามรุ่น"

{ echo '#include <cstdint>'; echo '#include <cstdio>'; cat "$TMP/Host-OLED.cpp"; cat battery_pct_test.cpp; } > "$TMP/t.cpp"
g++ -O2 -std=gnu++17 -Wall -Wextra -o "$TMP/t" "$TMP/t.cpp"
"$TMP/t"
