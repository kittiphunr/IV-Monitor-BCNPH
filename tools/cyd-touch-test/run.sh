#!/usr/bin/env bash
# ทดสอบไดรเวอร์ทัช XPT2046 แบบ bit-bang ของ Host บอร์ด CYD กับแบบจำลองชิปบนเครื่อง PC
set -euo pipefail
cd "$(dirname "$0")"
g++ -std=gnu++17 -I../screen-preview -I../../firmware/Host-CYD28 -Wall -o /tmp/cyd_touch_test test.cpp
/tmp/cyd_touch_test
