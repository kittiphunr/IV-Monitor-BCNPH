#!/usr/bin/env bash
# ทดสอบไฟสถานะการเชื่อมต่อแบบสองทางของเครื่องประจำเตียงทุกสาย บนเครื่อง PC
#
# ที่มา: ผู้ใช้ถ่ายภาพเครื่องจริง เตียงทั้งสามขึ้น ONLINE แต่ Host ขึ้นแค่ 2/3
# เพราะเตียงรู้แค่ว่าตัวเองได้ยิน Host ไม่รู้ว่า Host ได้ยินตัวเองหรือไม่
#
# ยืมส่วนตัวแทน Arduino จากตัวขับเรนเดอร์ของแต่ละรุ่น (ตัดตั้งแต่ main() ทิ้ง)
# ตัวแทนจึงตรงกับที่ใช้เรนเดอร์หน้าจอเสมอ ไม่ต้องดูแลสองชุด
set -euo pipefail
cd "$(dirname "$0")"
rc=0
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

run_station() {   # $1 = โฟลเดอร์สเก็ตช์
  local dir="../../firmware/$1" name="$1"
  echo "================ $name ================"
  cp "$dir/$name.ino" "$work/station.cpp"
  for h in "$dir"/*.h; do cp "$h" "$work/"; done
  sed '/^int main(/,$d' ../station-preview/driver.cpp > "$work/drv.cpp"
  cat >> "$work/drv.cpp" <<'CPP'
#define MY_ID          currentStationId
#define SYNC_CB        OnTimeSyncRecv
#define ADVANCE(ms)    (g_millis += (ms))
#define TICK()         loop()
#define RESET_LINK()   do { isHostOnline = false; lastHostRecvTime = 0; hostOnlineSince = 0; \
                            lastOwnSyncTime = 0; lastHeardReportTime = 0; hostReportsUplink = false; } while (0)
#include "cases.h"
int main() { g_ops = fopen("/dev/null", "w"); setup(); return runCases(); }
CPP
  cp cases.h "$work/"
  g++ -std=gnu++17 -I../screen-preview -I"$work" -DESP_ARDUINO_VERSION_MAJOR=3 -w -o "$work/t" "$work/drv.cpp"
  "$work/t" || rc=1
}

run_c3() {
  local dir="../../firmware/Station-C3-OLED"
  echo "================ Station-C3-OLED ================"
  cp "$dir/Station-C3-OLED.ino" "$work/station_c3.cpp"
  for h in "$dir"/*.h; do cp "$h" "$work/"; done
  sed '/^int main(/,$d' ../c3-preview/driver.cpp > "$work/drv.cpp"
  cat >> "$work/drv.cpp" <<'CPP'
#define MY_ID          stationId
#define SYNC_CB        OnHostSyncRecv
#define ADVANCE(ms)    (g_micros += (unsigned long)(ms) * 1000UL)
#define TICK()         ((void)0)
#define RESET_LINK()   do { lastHostSyncTime = 0; hostOnlineSince = 0; lastOwnSyncTime = 0; \
                            lastHeardReportTime = 0; hostReportsUplink = false; } while (0)
#include "cases.h"
int main() { g_ops = fopen("/dev/null", "w"); g_micros = 3000UL * 1000UL; return runCases(); }
CPP
  cp cases.h "$work/"
  g++ -std=gnu++17 -I../screen-preview -I"$work" -DESP_ARDUINO_VERSION_MAJOR=3 -w -o "$work/t" "$work/drv.cpp"
  "$work/t" || rc=1
}

run_station Station
run_station Station-Stable
run_station Station-Fallout
run_c3
exit $rc
