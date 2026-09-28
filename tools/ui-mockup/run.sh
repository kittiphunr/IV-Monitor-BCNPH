#!/usr/bin/env bash
# เรนเดอร์แบบร่าง UI ทั้ง 3 แนวทางเป็นภาพ PNG
set -euo pipefail
cd "$(dirname "$0")"
OUT="${1:-../../docs/ui-concepts}"
g++ -std=gnu++17 -I../screen-preview -DESP_ARDUINO_VERSION_MAJOR=3 -Wall -Wno-unused-variable \
    -o mockup station_ui_concepts.cpp mockup_runtime.cpp
./mockup ops.txt
python3 ../screen-preview/render.py ops.txt "$OUT"
rm -f mockup ops.txt
# แผ่นเปรียบเทียบสามแนวทาง สร้างใหม่ทุกครั้งจากภาพที่เพิ่งเรนเดอร์
for s in 1:normal 2:alert 3:nearend; do
  n="${s%%:*}"; k="${s##*:}"
  python3 ../screen-preview/make_sheet.py "$OUT/compare_$k.png" 3 \
    "$OUT/A${n}_$k.png" "A BIG NUMBER" "$OUT/B${n}_$k.png" "B STATUS CARD" "$OUT/C${n}_$k.png" "C IV BAG"
done
echo "เสร็จแล้ว: ภาพอยู่ใน $OUT"
