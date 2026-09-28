#!/usr/bin/env bash
# เรนเดอร์ภาพหน้าจอของเฟิร์มแวร์ Station ทุกหน้า (ใช้ Arduino API จำลองบน PC)
# ใช้: run.sh [โฟลเดอร์ผลลัพธ์] [ไฟล์ .ino]
set -euo pipefail
cd "$(dirname "$0")"
OUT="${1:-../../docs/screens-station}"
SKETCH="${2:-../../firmware/Station/Station.ino}"

# ตรวจลำดับการประกาศชนิดข้อมูล (Arduino แทรก prototype ไว้ก่อนฟังก์ชันแรกของไฟล์)
python3 ../check-ino-types.py "$SKETCH"

cp "$SKETCH" station.cpp
# คัดลอกไฟล์ .h ทุกตัวของสเก็ตช์ เพราะโค้ดวาดจอถูกแยกไปอยู่ StationScreen.h แล้ว
for h in "$(dirname "$SKETCH")"/*.h; do [ -e "$h" ] && cp "$h" "$(basename "$h")"; done
# สายเสถียรยังมีหน้าคาลิเบรตสามขั้นแบบเดิม ตัวขับจึงเก็บภาพทีละขั้นให้
EXTRA=""
[ "$(basename "$SKETCH")" = "Station-Stable.ino" ] && EXTRA="-DSTATION_STABLE_PREVIEW"
g++ -std=gnu++17 -I../screen-preview -DESP_ARDUINO_VERSION_MAJOR=3 $EXTRA -Wall -Wno-unused-variable -o stpreview driver.cpp
./stpreview ops.txt
python3 ../screen-preview/render.py ops.txt "$OUT"
rm -f station.cpp drop_detector.h StationScreen.h stpreview ops.txt
echo "เสร็จแล้ว: ภาพอยู่ใน $OUT"

# แผ่นรวมมีหน้าตรวจเซนเซอร์ของสายหลัก สายเสถียรเรนเดอร์ไว้เอาเฉพาะหน้าคาลิเบรตจึงข้าม
[ -z "$EXTRA" ] && python3 make_overview.py "$OUT"

# หน้าคาลิเบรตสามขั้นมีเฉพาะสายเสถียร เรนเดอร์สายนั้นแยกไว้ แล้วคัดมาเฉพาะภาพคาลิเบรต
# (ทำเฉพาะตอนเรนเดอร์สายหลัก เพื่อไม่ให้เรียกตัวเองวนซ้ำ)
if [ "$(basename "$SKETCH")" = "Station.ino" ]; then
  STABLE_TMP="$(mktemp -d)"
  bash ./run.sh "$STABLE_TMP" ../../firmware/Station-Stable/Station-Stable.ino >/dev/null
  for f in cal_1_noise cal_2_learn cal_3_result cal_4_verify; do cp "$STABLE_TMP/$f.png" "$OUT/"; done
  rm -rf "$STABLE_TMP"
  python3 ../screen-preview/make_sheet.py "$OUT/overview_calib.png" 4 \
    "$OUT/cal_1_noise.png" "1 NOISE" "$OUT/cal_2_learn.png" "2 LEARN" \
    "$OUT/cal_3_result.png" "3 RESULT" "$OUT/cal_4_verify.png" "4 VERIFY"
fi
