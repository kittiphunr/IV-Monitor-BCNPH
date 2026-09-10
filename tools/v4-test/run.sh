#!/usr/bin/env bash
# ชุดทดสอบ Protocol v4 ทั้งหมด: ตัวคำนวณอัตราไหล Station-Thin Host-TFT-Gateway และวงจรปิดคู่กับเซิร์ฟเวอร์พีซี
set -euo pipefail
cd "$(dirname "$0")"
ROOT=../../firmware

echo "== protocol_v4.h เหมือนกันทุกไบต์ใน Station-Thin และ Host-TFT-Gateway"
cmp "$ROOT/Station-Thin/protocol_v4.h" "$ROOT/Host-TFT-Gateway/protocol_v4.h"
echo "ตรงกัน"

echo; echo "== ตัวคำนวณอัตราไหล (rate_estimator.h) เทียบกับวิธีเดิมของ Station"
g++ -std=gnu++17 -I"$ROOT/Host-TFT-Gateway" -Wall -O1 -o est_test estimator_test.cpp
./est_test | grep -v "^  .*ผ่าน$" || true
./est_test >/dev/null

echo; echo "== ตัวตรวจระดับแบตเตอรี่ (battery_check.h)"
cmp "$ROOT/Station-Thin/battery_check.h" "$ROOT/Host-TFT-Gateway/battery_check.h"
g++ -std=gnu++17 -I"$ROOT/Host-TFT-Gateway" -Wall -O1 -o bat_test battery_test.cpp
./bat_test | grep -v "^  .*ผ่าน$" || true
./bat_test >/dev/null

echo; echo "== Station-Thin"
bash run_station.sh /tmp/v4_station_shots
echo; echo "== Host-TFT-Gateway"
bash run_host.sh

echo; echo "== วงจรปิด: เฟิร์มแวร์จริงสองฝั่ง + เซิร์ฟเวอร์พีซีจริง"
cp "$ROOT/Station-Thin/Station-Thin.ino" station_thin.cpp
cp "$ROOT/Host-TFT-Gateway/Host-TFT-Gateway.ino" host_tg.cpp
mkdir -p /tmp/v4_sim_st /tmp/v4_sim_host
cp "$ROOT"/Station-Thin/*.h /tmp/v4_sim_st/; cp "$ROOT"/Host-TFT-Gateway/*.h /tmp/v4_sim_host/
trap 'rm -f est_test bat_test station_thin.cpp host_tg.cpp stthin_sim hosttg_sim' EXIT
g++ -std=gnu++17 -I../screen-preview -I/tmp/v4_sim_st -I. -DESP_ARDUINO_VERSION_MAJOR=3 -w -o stthin_sim station_driver.cpp
g++ -std=gnu++17 -I../screen-preview -I/tmp/v4_sim_host -I. -DESP_ARDUINO_VERSION_MAJOR=3 -w -o hosttg_sim host_driver.cpp
python3 -B closed_loop.py ./stthin_sim ./hosttg_sim
