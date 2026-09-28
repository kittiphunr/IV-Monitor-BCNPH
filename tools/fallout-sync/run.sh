#!/usr/bin/env bash
# ตรวจว่าสาย Fallout ยังใช้ตรรกะเดียวกับสายหลักทุกบรรทัด ต่างกันแค่ธีมและหน้าจอ
# และภาพ Vault Boy ใน pipboy_sprite.h ตรงกับภาพต้นทางใน tools/fallout-sprite
# แก้สายหลักแล้วตรวจไม่ผ่าน ให้รัน: python3 tools/fallout-sync/sync.py --write
set -euo pipefail
cd "$(dirname "$0")"
python3 sync.py
python3 ../fallout-sprite/make_sprite.py --check
