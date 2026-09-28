#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ประกอบภาพหน้าจอหลายภาพเป็นแผ่นเดียว (แผ่นเปรียบเทียบ / ภาพรวม)

ใช้: make_sheet.py ผลลัพธ์.png จำนวนคอลัมน์ ภาพ1 คำบรรยาย1 [ภาพ2 คำบรรยาย2 ...]
     คำบรรยายว่าง ("") = ไม่มีแถบคำบรรยายของภาพนั้น

เดิมแผ่นเหล่านี้ประกอบด้วยมือนอกเครื่องมือ พอภาพต้นทางเปลี่ยนก็ไม่มีใครสร้างใหม่
และคำบรรยายภาษาไทยกลายเป็นกล่องสี่เหลี่ยม เพราะฟอนต์ที่ใช้ตอนนั้นไม่มีอักษรไทย
ตอนนี้ run.sh ของแต่ละเครื่องมือเรียกสคริปต์นี้ทุกครั้ง และคำบรรยายเขียนด้วย
ฟอนต์ 5x7 ตัวจริงของ Adafruit_GFX (ASCII ล้วน) แบบเดียวกับตัวอักษรบนจอ
"""
import sys
from PIL import Image, ImageDraw
from bitmapfont import glcd_font

BG = (26, 27, 34)
CAP = (170, 174, 186)
PAD = 18
CAP_H = 30
CAP_SIZE = 2          # ขนาดตัวอักษรของคำบรรยาย (ช่องละ 12x16 พิกเซล)


def draw_caption(img, x, y, text):
    d = ImageDraw.Draw(img)
    font = glcd_font()

    def fillrect(rx, ry, rw, rh, c):
        d.rectangle([rx, ry, rx + rw - 1, ry + rh - 1], fill=c)

    for i, ch in enumerate(text.encode("ascii", "replace")):
        font.draw_char(fillrect, x + i * 6 * CAP_SIZE, y, ch, CAP, CAP, CAP_SIZE)


def main(out, cols, pairs):
    tiles = [(Image.open(p).convert("RGB"), cap) for p, cap in pairs]
    has_cap = any(cap for _, cap in tiles)
    rows = [tiles[i:i + cols] for i in range(0, len(tiles), cols)]
    col_w = [max(r[c][0].width for r in rows if c < len(r)) for c in range(cols)]
    row_h = [max(t.height for t, _ in r) + (CAP_H if has_cap else 0) for r in rows]
    W = PAD + sum(w + PAD for w in col_w)
    H = PAD + sum(h + PAD for h in row_h)
    sheet = Image.new("RGB", (W, H), BG)
    y = PAD
    for r, row in enumerate(rows):
        x = PAD
        for c, (img, cap) in enumerate(row):
            ty = y
            if has_cap:
                if cap:
                    draw_caption(sheet, x + 2, y + 4, cap)
                ty += CAP_H
            sheet.paste(img, (x, ty))
            x += col_w[c] + PAD
        y += row_h[r] + PAD
    sheet.save(out)
    print("%s %s" % (out, sheet.size))


if __name__ == "__main__":
    if len(sys.argv) < 5 or (len(sys.argv) - 3) % 2:
        sys.exit(__doc__)
    args = sys.argv[3:]
    main(sys.argv[1], int(sys.argv[2]), list(zip(args[0::2], args[1::2])))
