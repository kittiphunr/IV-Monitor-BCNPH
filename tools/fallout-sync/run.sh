#!/usr/bin/env bash
# ตรวจว่าสาย Fallout ยังใช้ตรรกะเดียวกับสายหลักทุกบรรทัด ต่างกันแค่ธีมและหน้าจอ
# แก้สายหลักแล้วตรวจไม่ผ่าน ให้รัน: python3 tools/fallout-sync/sync.py --write
set -euo pipefail
cd "$(dirname "$0")"
python3 sync.py
