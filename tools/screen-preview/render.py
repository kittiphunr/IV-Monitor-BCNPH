#!/usr/bin/env python3
"""เรนเดอร์คำสั่งวาดจากเฟิร์มแวร์ (ops.txt) ออกเป็นภาพพรีวิวหน้าจอ TFT

ใช้: render.py ops.txt outdir [กว้าง สูง [อัตราขยาย]]
ค่าเริ่มต้นคือจอของเครื่องประจำเตียง 172x320 ขยาย 3 เท่า

ตัวอักษรวาดด้วยฟอนต์ 5x7 "classic" ตัวจริงของ Adafruit_GFX (fonts/glcdfont.c)
ตามขั้นตอนของ Adafruit_GFX::drawChar() ทีละพิกเซล จึงเป็นรูปเดียวกับจอจริง
เดิมใช้ฟอนต์ DejaVu (TrueType) วาดแทน ขนาดใกล้เคียงแต่รูปตัวอักษรไม่ใช่ของจริง
"""
import sys, os
from PIL import Image, ImageDraw
from bitmapfont import glcd_font

W, H, S = 172, 320, 3

def rgb(c):
    c = int(c)
    r = (c >> 11) & 0x1F; g = (c >> 5) & 0x3F; b = c & 0x1F
    return ((r * 255) // 31, (g * 255) // 63, (b * 255) // 31)

def main(ops_path, outdir):
    os.makedirs(outdir, exist_ok=True)
    img = Image.new("RGB", (W * S, H * S), (0, 0, 0))
    d = ImageDraw.Draw(img)
    saved = []
    warnings = []
    for line in open(ops_path, encoding="utf-8"):
        p = line.rstrip("\n").split(" ")
        op = p[0]
        if op == "FILLSCREEN":
            d.rectangle([0, 0, W * S, H * S], fill=rgb(p[1]))
        elif op == "RECT":
            x, y, w, h, c, fill = int(p[1]), int(p[2]), int(p[3]), int(p[4]), p[5], p[6] == "1"
            if w <= 0 or h <= 0:
                continue          # Adafruit_GFX ไม่วาดอะไรเลยเมื่อกว้าง/สูงเป็น 0 หรือติดลบ
            box = [x * S, y * S, (x + w) * S - 1, (y + h) * S - 1]
            if fill: d.rectangle(box, fill=rgb(c))
            else:    d.rectangle(box, outline=rgb(c), width=S)
        elif op == "RRECT":
            x, y, w, h, r, c, fill = int(p[1]), int(p[2]), int(p[3]), int(p[4]), int(p[5]), p[6], p[7] == "1"
            if w <= 0 or h <= 0:
                continue
            box = [x * S, y * S, (x + w) * S - 1, (y + h) * S - 1]
            if fill: d.rounded_rectangle(box, radius=r * S, fill=rgb(c))
            else:    d.rounded_rectangle(box, radius=r * S, outline=rgb(c), width=S)
        elif op == "LINE":
            d.line([int(p[1]) * S, int(p[2]) * S, int(p[3]) * S, int(p[4]) * S], fill=rgb(p[5]), width=S)
        elif op == "CIRC":
            x, y, r = int(p[1]), int(p[2]), int(p[3])
            d.ellipse([(x - r) * S, (y - r) * S, (x + r) * S, (y + r) * S], fill=rgb(p[4]))
        elif op == "CIRCO":
            x, y, r = int(p[1]), int(p[2]), int(p[3])
            d.ellipse([(x - r) * S, (y - r) * S, (x + r) * S, (y + r) * S], outline=rgb(p[4]), width=S)
        elif op == "TEXT":
            # Adafruit_GFX::write(): วาดทีละตัวที่เคอร์เซอร์ แล้วเลื่อนไป 6 x ขนาด
            # (เฟิร์มแวร์ทุกสายสั่ง setTextWrap(false) จึงไม่ต้องจำลองการตัดบรรทัด)
            x, y, size, col, bg = int(p[1]), int(p[2]), int(p[3]), int(p[4]), int(p[5])
            text = " ".join(p[6:]) if len(p) > 6 else ""
            font = glcd_font()

            def fillrect(rx, ry, rw, rh, c):
                d.rectangle([rx * S, ry * S, (rx + rw) * S - 1, (ry + rh) * S - 1], fill=rgb(c))

            # Adafruit_GFX::print() ส่งทีละไบต์ ข้อความ UTF-8 (เช่นอักษรไทย) จึงกลายเป็น
            # สัญลักษณ์มั่วตัวละไบต์บนจอจริง วาดตามนั้นเป๊ะ แล้วเตือนไว้ให้แก้ต้นทาง
            raw = text.encode("utf-8")
            if any(b >= 0x80 for b in raw):
                warnings.append(f"ข้อความไม่ใช่ ASCII จะขึ้นเป็นสัญลักษณ์มั่วบนจอจริง ที่ ({x},{y}): {text!r}")
            for i, chpr in enumerate(raw):
                font.draw_char(fillrect, x + i * 6 * size, y, chpr, col, bg, size)
        elif op == "MARK":
            name = p[1]
            out = os.path.join(outdir, f"{name}.png")
            img.save(out)
            saved.append(out)
    print("\n".join(saved))
    if warnings:
        print("\nคำเตือน:")
        for w in sorted(set(warnings)):
            print("  " + w)

if __name__ == "__main__":
    if len(sys.argv) >= 5:
        W, H = int(sys.argv[3]), int(sys.argv[4])
    if len(sys.argv) >= 6:
        S = int(sys.argv[5])
    main(sys.argv[1], sys.argv[2])
