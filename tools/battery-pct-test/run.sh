#!/usr/bin/env bash
# ตรวจสูตรเปอร์เซ็นต์แบตเตอรี่ (calculateBatteryPct ตาราง OCV) และสถานะเตือนแบตต่ำ
# (batteryAlertState) ของ Host ทั้งสามรุ่นจอ
# และเครื่องประจำเตียงบอร์ดเล็ก Station-C3-OLED
#   1) ตัดตาราง เกณฑ์เตือนแบตต่ำ และฟังก์ชันออกมาจากไฟล์ .ino ตัวจริง ไม่ได้ลอกมาเขียนใหม่
#   2) ทั้งสี่สเก็ตช์ต้องเหมือนกันทุกบรรทัด
#   3) คอมไพล์ด้วย g++ แล้วเทียบค่าที่จุดสำคัญ และตรวจว่าไม่ลดลงเมื่อแรงดันเพิ่ม
set -euo pipefail
cd "$(dirname "$0")"
ROOT=../..
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT

extract() {  # ค่าเกณฑ์เตือนแบต + ตั้งแต่ BATT_OCV_V จนจบ calculateBatteryPct และ batteryAlertState
  grep -E '^#define BATT_(STATE_OK|STATE_LOW|STATE_CRIT|LOW_V|CRIT_V|HYST_V) ' "$1"
  awk '/^static const float   BATT_OCV_V\[\]/{on=1} on&&/^\/\//{next} on{print}
       on&&/^}/{n++} n==2{exit}' "$1"
}
for h in Host-OLED Host-TFT28 Host-Touch24 Station-C3-OLED; do
  extract "$ROOT/firmware/$h/$h.ino" > "$TMP/$h.cpp"
  [ -s "$TMP/$h.cpp" ] || { echo "ไม่พบตาราง OCV ใน $h.ino"; exit 1; }
done
for h in Host-TFT28 Host-Touch24 Station-C3-OLED; do
  if ! diff -u "$TMP/Host-OLED.cpp" "$TMP/$h.cpp"; then
    echo "FAIL: calculateBatteryPct ของ $h ไม่ตรงกับ Host-OLED"; exit 1
  fi
done
echo "ตาราง OCV เกณฑ์เตือนแบต และฟังก์ชันทั้งสอง ตรงกันทั้ง Host สามรุ่นและ Station-C3-OLED"

{ echo '#include <cstdint>'; echo '#include <cstdio>'; cat "$TMP/Host-OLED.cpp"; cat battery_pct_test.cpp; } > "$TMP/t.cpp"
g++ -O2 -std=gnu++17 -Wall -Wextra -o "$TMP/t" "$TMP/t.cpp"
"$TMP/t"
