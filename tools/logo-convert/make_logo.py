#!/usr/bin/env python3
"""แปลงภาพโลโก้ (PNG/JPG) เป็นไฟล์ logo.h สำหรับหน้าเปิดเครื่อง (RGB565 เก็บใน flash)

ใช้:
  python3 tools/logo-convert/make_logo.py ภาพ.png --size 320x240 --out firmware/Host-CYD28/logo.h
  python3 tools/logo-convert/make_logo.py ภาพ.png --size 172x320 --out firmware/Station-Gen2/logo.h --bg 000000

ภาพถูกย่อให้พอดีกรอบโดยคงสัดส่วน (วางกึ่งกลาง เติมพื้นหลังสีเดียว) ภาพโปร่งใสวางบนสีพื้นหลัง
ขนาดไฟล์ = กว้าง x สูง x 2 ไบต์ (320x240 = 153,600 ไบต์) ตรวจก่อนว่า Partition Scheme ของบอร์ดพอ
ตัวเฟิร์มแวร์ใช้ LOGO_PRESENT เป็นตัวบอกว่ามีภาพจริง ถ้าเป็น 0 จะแสดงหน้าเปิดเครื่องแบบตัวอักษรแทน
"""
import argparse, hashlib, os, sys
from PIL import Image

def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("image")
    ap.add_argument("--size", required=True, help="กว้างxสูง เช่น 320x240")
    ap.add_argument("--out", required=True)
    ap.add_argument("--bg", default="000000", help="สีพื้นหลังฐานสิบหก RRGGBB (ปริยาย ดำ)")
    ap.add_argument("--name", default="")
    a = ap.parse_args()
    w, h = (int(v) for v in a.size.lower().split("x"))
    if not (8 <= w <= 480 and 8 <= h <= 480):
        sys.exit("ขนาดต้องอยู่ระหว่าง 8 ถึง 480 พิกเซล")
    bg = tuple(int(a.bg[i:i + 2], 16) for i in (0, 2, 4))
    src = Image.open(a.image).convert("RGBA")
    scale = min(w / src.width, h / src.height)
    nw, nh = max(1, round(src.width * scale)), max(1, round(src.height * scale))
    src = src.resize((nw, nh), Image.LANCZOS)
    canvas = Image.new("RGB", (w, h), bg)
    canvas.paste(src, ((w - nw) // 2, (h - nh) // 2), src)
    px = [rgb565(*canvas.getpixel((x, y))) for y in range(h) for x in range(w)]
    digest = hashlib.sha256(b"".join(v.to_bytes(2, "big") for v in px)).hexdigest()[:12]
    lines = []
    for y in range(h):
        row = px[y * w:(y + 1) * w]
        lines.append("  " + ",".join(f"0x{v:04X}" for v in row) + ("," if y < h - 1 else ""))
    name = a.name or os.path.basename(a.image)
    text = f'''/**
 * @file      {os.path.basename(a.out)}
 * @brief     โลโก้หน้าเปิดเครื่อง {w}x{h} RGB565 (สร้างอัตโนมัติ ห้ามแก้ด้วยมือ)
 * @version   1.0.0
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * สร้างจากภาพ {name} ด้วย tools/logo-convert/make_logo.py (ขนาด {w}x{h} พื้นหลัง #{a.bg} sha256 {digest})
 * เปลี่ยนโลโก้ให้รันสคริปต์ใหม่กับภาพใหม่ ห้ามแก้อาร์เรย์ด้วยมือ
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 1.0.0 | 2026-11-27 | สร้างจากภาพ {name} |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <Arduino.h>

#define LOGO_PRESENT 1
#define LOGO_W {w}
#define LOGO_H {h}

static const uint16_t LOGO_DATA[LOGO_W * LOGO_H] PROGMEM = {{
{chr(10).join(lines)}
}};
'''
    with open(a.out, "w", encoding="utf-8") as f:
        f.write(text)
    print(f"เขียน {a.out}: {w}x{h} ({w * h * 2:,} ไบต์ใน flash) sha256 {digest}")

if __name__ == "__main__":
    main()
