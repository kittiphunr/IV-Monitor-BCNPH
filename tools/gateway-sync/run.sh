#!/usr/bin/env bash
# ตรวจว่าตรรกะเตือน/รับแพ็กเก็ตของ Host-Gateway ยังตรงกับ Host-OLED
set -euo pipefail
cd "$(dirname "$0")"
python3 -B check.py
