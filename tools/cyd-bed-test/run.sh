#!/usr/bin/env bash
# ทดสอบแบบจำลองเตียงของ Host-CYD28 รุ่นเขียนใหม่ (ด่านที่ 2): ออนไลน์ รหัสเตือน ขาดการติดต่อ Sync ตัวจัดช่องเวลา
set -euo pipefail
cd "$(dirname "$0")"
g++ -std=gnu++17 -I../../firmware/Host-CYD28 -Wall -Wextra -o /tmp/cyd_bed_test test.cpp
timeout 30 /tmp/cyd_bed_test
