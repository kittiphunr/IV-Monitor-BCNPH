#!/usr/bin/env bash
# เรนเดอร์แบบร่าง UI แบบสมาร์ตโฟนสำหรับจอสัมผัส 2.4" (240x320)
set -euo pipefail
cd "$(dirname "$0")"
OUT="${1:-../../docs/ui-concepts-touch}"
mkdir -p "$OUT"
g++ -std=gnu++17 -I../screen-preview -Wall -Wno-unused-variable -o mockup touch_ui_concepts.cpp mockup_runtime.cpp
./mockup ops.txt
python3 ../screen-preview/render.py ops.txt "$OUT" 240 320 3
rm -f mockup ops.txt
# แผ่นภาพรวมลำดับการใช้งาน สร้างใหม่ทุกครั้งจากภาพที่เพิ่งเรนเดอร์
python3 ../screen-preview/make_sheet.py "$OUT/overview_flow.png" 3 \
  "$OUT/t1_home.png" "1 HOME" "$OUT/t3_bed.png" "2 BED DETAIL" \
  "$OUT/t4_bed_settings.png" "3 BED SETTINGS (-/+)" "$OUT/t5_numpad.png" "4 NUMPAD" \
  "$OUT/t6_system.png" "5 SYSTEM SETTINGS" "$OUT/t7_alarm.png" "6 ALARM"
echo "เสร็จแล้ว: ภาพอยู่ใน $OUT"
