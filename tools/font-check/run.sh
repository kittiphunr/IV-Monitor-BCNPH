#!/usr/bin/env bash
# ตรวจฟอนต์ตัวจริงที่ใช้เรนเดอร์ภาพจำลอง และค่าคงที่ความกว้างตัวอักษรในเฟิร์มแวร์
set -euo pipefail
cd "$(dirname "$0")"
python3 check.py
