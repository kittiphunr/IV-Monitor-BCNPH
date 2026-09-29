#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ประกอบภาพหน้าจอทีละเฟรมของหนึ่งฉากเป็นแผ่นเรียงตามเวลา (filmstrip)

ใช้: make_strip.py ผลลัพธ์.png มิลลิวินาทีต่อเฟรม เลือกทุกกี่มิลลิวินาที จำนวนคอลัมน์ เฟรม1.png [...]

เลือกเฟรมทุก ๆ ช่วงเวลาที่กำหนด ย่อเหลือขนาดจอจริง (ภาพจากตัวเรนเดอร์ขยาย 3 เท่า)
แล้วเขียนเวลาใต้ภาพด้วยฟอนต์ 5x7 ตัวจริงของ Adafruit_GFX แบบเดียวกับ make_sheet.py
"""
import os
import sys
from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "screen-preview"))
from make_sheet import draw_caption, BG  # noqa: E402

PAD, CAP_H = 12, 30


def main(out, ms, every, cols, paths):
    step = max(1, every // ms)
    pick = [(i * ms, paths[i]) for i in range(0, len(paths), step)]
    tiles = []
    for t, p in pick:
        im = Image.open(p).convert("RGB")
        tiles.append((t, im.resize((im.width // 3, im.height // 3), Image.NEAREST)))
    tw, th = tiles[0][1].size
    rows = (len(tiles) + cols - 1) // cols
    sheet = Image.new("RGB", (PAD + cols * (tw + PAD), PAD + rows * (th + CAP_H + PAD)), BG)
    for n, (t, im) in enumerate(tiles):
        x = PAD + (n % cols) * (tw + PAD)
        y = PAD + (n // cols) * (th + CAP_H + PAD)
        draw_caption(sheet, x + 2, y + 6, "t=%.1fs" % (t / 1000.0))
        sheet.paste(im, (x, y + CAP_H))
    sheet.save(out)
    print("%s %s (%d ภาพ)" % (out, sheet.size, len(tiles)))


if __name__ == "__main__":
    if len(sys.argv) < 6:
        sys.exit(__doc__)
    main(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), sys.argv[5:])
