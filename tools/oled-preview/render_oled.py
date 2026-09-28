#!/usr/bin/env python3
"""เรนเดอร์คำสั่งวาดของ U8g2 เป็นภาพจอ OLED ขาวดำ

ใช้: render_oled.py ops.txt outdir [scale]

วาดลงบัฟเฟอร์ขนาดเท่าจอจริงทีละพิกเซลก่อน แล้วค่อยขยายแบบไม่เกลี่ยสี
ภาพที่ได้จึงเป็นพิกเซลเดียวกับที่จอจริงจะแสดง

  ตัวอักษร  ใช้ฟอนต์บิตแมปตัวจริงของ U8g2 (ดู tools/screen-preview/bitmapfont.py)
            รวมถึงโหมดทึบซึ่งเป็นค่าเริ่มต้นของ U8g2: พื้นหลังในกรอบตัวอักษรถูกระบาย
            ด้วยสีตรงข้าม ตัวอักษรที่วางทับเส้นหรือกล่องจึงเห็นผลเหมือนจอจริง
  รูปทรง    กล่องมน กรอบมน และเส้นตรง แปลงมาจาก u8g2_box.c u8g2_circle.c
            และ u8g2_line.c ทีละบรรทัด มุมมนจึงตรงพิกเซลกับของจริง
"""
import os
import sys
from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "screen-preview"))
from bitmapfont import u8g2_font  # noqa: E402

# ขนาดจอปรับได้ผ่านตัวแปรสภาพแวดล้อม เพราะโครงการนี้มีจอ OLED สองขนาด
# 128x64 (SH1106 ของเครื่องส่วนกลาง) และ 72x40 (SSD1306 ของเครื่องประจำเตียงรุ่นบอร์ดเล็ก)
W = int(os.environ.get("OLED_W", "128"))
H = int(os.environ.get("OLED_H", "64"))

UPPER_RIGHT, UPPER_LEFT, LOWER_LEFT, LOWER_RIGHT = 0x01, 0x02, 0x04, 0x08


class Canvas:
    """บัฟเฟอร์ 1 บิตขนาดเท่าจอ พร้อมคำสั่งวาดแบบเดียวกับ U8g2"""

    def __init__(self):
        self.px = bytearray(W * H)

    def pixel(self, x, y, c):
        if 0 <= x < W and 0 <= y < H:
            self.px[y * W + x] = 1 if c else 0

    def hline(self, x, y, w, c):
        for i in range(w):
            self.pixel(x + i, y, c)

    def vline(self, x, y, h, c):
        for i in range(h):
            self.pixel(x, y + i, c)

    def box(self, x, y, w, h, c):
        for j in range(h):
            self.hline(x, y + j, w, c)

    def frame(self, x, y, w, h, c):                      # u8g2_DrawFrame()
        xtmp = x
        self.hline(x, y, w, c)
        if h >= 2:
            h -= 2
            y += 1
            if h > 0:
                self.vline(x, y, h, c)
                self.vline(x + w - 1, y, h, c)
                y += h
            self.hline(xtmp, y, w, c)

    def _circle_section(self, x, y, x0, y0, opt, c):
        if opt & UPPER_RIGHT:
            self.pixel(x0 + x, y0 - y, c); self.pixel(x0 + y, y0 - x, c)
        if opt & UPPER_LEFT:
            self.pixel(x0 - x, y0 - y, c); self.pixel(x0 - y, y0 - x, c)
        if opt & LOWER_RIGHT:
            self.pixel(x0 + x, y0 + y, c); self.pixel(x0 + y, y0 + x, c)
        if opt & LOWER_LEFT:
            self.pixel(x0 - x, y0 + y, c); self.pixel(x0 - y, y0 + x, c)

    def _disc_section(self, x, y, x0, y0, opt, c):
        if opt & UPPER_RIGHT:
            self.vline(x0 + x, y0 - y, y + 1, c); self.vline(x0 + y, y0 - x, x + 1, c)
        if opt & UPPER_LEFT:
            self.vline(x0 - x, y0 - y, y + 1, c); self.vline(x0 - y, y0 - x, x + 1, c)
        if opt & LOWER_RIGHT:
            self.vline(x0 + x, y0, y + 1, c); self.vline(x0 + y, y0, x + 1, c)
        if opt & LOWER_LEFT:
            self.vline(x0 - x, y0, y + 1, c); self.vline(x0 - y, y0, x + 1, c)

    def _midpoint(self, x0, y0, rad, opt, c, section):   # u8g2_draw_circle / _disc
        f = 1 - rad
        ddF_x = 1
        ddF_y = -2 * rad
        x, y = 0, rad
        section(x, y, x0, y0, opt, c)
        while x < y:
            if f >= 0:
                y -= 1
                ddF_y += 2
                f += ddF_y
            x += 1
            ddF_x += 2
            f += ddF_x
            section(x, y, x0, y0, opt, c)

    def rbox(self, x, y, w, h, r, c):                    # u8g2_DrawRBox()
        xl, yu = x + r, y + r
        xr, yl = x + w - r - 1, y + h - r - 1
        for cx, cy, o in ((xl, yu, UPPER_LEFT), (xr, yu, UPPER_RIGHT),
                          (xl, yl, LOWER_LEFT), (xr, yl, LOWER_RIGHT)):
            self._midpoint(cx, cy, r, o, c, self._disc_section)
        ww = w - 2 * r
        xl += 1
        yu += 1
        if ww >= 3:
            ww -= 2
            self.box(xl, y, ww, r + 1, c)
            self.box(xl, yl, ww, r + 1, c)
        hh = h - 2 * r
        if hh >= 3:
            hh -= 2
            self.box(x, yu, w, hh, c)

    def rframe(self, x, y, w, h, r, c):                  # u8g2_DrawRFrame()
        xl, yu = x + r, y + r
        xr, yl = x + w - r - 1, y + h - r - 1
        for cx, cy, o in ((xl, yu, UPPER_LEFT), (xr, yu, UPPER_RIGHT),
                          (xl, yl, LOWER_LEFT), (xr, yl, LOWER_RIGHT)):
            self._midpoint(cx, cy, r, o, c, self._circle_section)
        ww = w - 2 * r
        hh = h - 2 * r
        xl += 1
        yu += 1
        if ww >= 3:
            ww -= 2
            self.hline(xl, y, ww, c)
            self.hline(xl, y + h - 1, ww, c)
        if hh >= 3:
            hh -= 2
            self.vline(x, yu, hh, c)
            self.vline(x + w - 1, yu, hh, c)

    def line(self, x1, y1, x2, y2, c):                   # u8g2_DrawLine()
        dx, dy = abs(x2 - x1), abs(y2 - y1)
        swap = dy > dx
        if swap:
            dx, dy = dy, dx
            x1, y1, x2, y2 = y1, x1, y2, x2
        if x1 > x2:
            x1, x2, y1, y2 = x2, x1, y2, y1
        err = dx >> 1
        ystep = 1 if y2 > y1 else -1
        y = y1
        for x in range(x1, x2 + 1):
            if swap:
                self.pixel(y, x, c)
            else:
                self.pixel(x, y, c)
            err -= dy
            if err < 0:
                y += ystep
                err += dx

    def image(self, scale):
        img = Image.frombytes("L", (W, H), bytes(v * 255 for v in self.px))
        return img.resize((W * scale, H * scale), Image.NEAREST).convert("RGB")


def main(ops_path, outdir, scale=6):
    os.makedirs(outdir, exist_ok=True)
    cv = Canvas()
    warnings = []
    saved = []

    for line in open(ops_path, encoding="utf-8"):
        p = line.rstrip("\n").split(" ")
        op = p[0]
        if op == "CLEAR":
            cv = Canvas()
        elif op == "MARK":
            out = os.path.join(outdir, p[1] + ".png")
            cv.image(scale).save(out)
            saved.append(out)
        elif op == "TEXT":
            x, y, font, color = int(p[1]), int(p[2]), p[3], int(p[4])
            text = " ".join(p[5:])
            f = u8g2_font(font)
            end = f.draw_str(cv.pixel, x, y, text, color)
            if end > W:
                warnings.append(f"ข้อความล้นขอบขวา ({end} > {W} px) ที่ ({x},{y}) ฟอนต์ {font}: {text!r}")
        elif op == "LINE":
            x0, y0, x1, y1, color = (int(v) for v in p[1:6])
            cv.line(x0, y0, x1, y1, color)
        else:
            x, y, w, h, r, color = (int(v) for v in p[1:7])
            if op in ("BOX", "HLINE", "VLINE"):
                cv.box(x, y, w, h, color)
            elif op == "FRAME":
                cv.frame(x, y, w, h, color)
            elif op == "RBOX":
                cv.rbox(x, y, w, h, r, color)
            elif op == "RFRAME":
                cv.rframe(x, y, w, h, r, color)

    for s in saved:
        print(s)
    if warnings:
        print("\nคำเตือน:")
        for w in sorted(set(warnings)):
            print("  " + w)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 6))
