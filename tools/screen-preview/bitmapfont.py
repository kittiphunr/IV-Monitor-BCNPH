# -*- coding: utf-8 -*-
"""ฟอนต์บิตแมปตัวจริงของไลบรารีที่เฟิร์มแวร์ใช้ สำหรับเรนเดอร์ภาพหน้าจอจำลอง

เดิมตัวเรนเดอร์ใช้ฟอนต์ DejaVu (TrueType) วาดแทน ตำแหน่งกับขนาดใกล้เคียง
แต่รูปตัวอักษรไม่ใช่ของจริง ภาพอ้างอิงจึงหน้าตาไม่เหมือนจอที่ประกอบเสร็จ
ไฟล์นี้อ่านข้อมูลฟอนต์ต้นฉบับของไลบรารีโดยตรง แล้วถอดรหัสด้วยวิธีเดียวกับไลบรารี

  U8g2Font  ฟอนต์ของ U8g2 (จอ OLED) อ่านจากไฟล์ fonts/u8g2_font_<ชื่อ>.c
            ถอดรหัสตาม csrc/u8g2_font.c ของ U8g2 ทีละขั้น
            (หัวฟอนต์ 23 ไบต์ บิตสตรีมเรียงจากบิตต่ำ ภาพตัวอักษรแบบ run-length)
  GlcdFont  ฟอนต์ 5x7 "classic" ของ Adafruit_GFX (จอ TFT) อ่านจาก fonts/glcdfont.c
            วาดตาม Adafruit_GFX::drawChar() ทีละขั้น

ที่มาและสัญญาอนุญาตของไฟล์ฟอนต์อยู่ที่ fonts/README.md
"""
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
FONT_DIR = os.path.join(HERE, "fonts")

_ESC = {"n": 10, "t": 9, "r": 13, "\\": 92, '"': 34, "'": 39, "?": 63,
        "a": 7, "b": 8, "f": 12, "v": 11}


def _c_string_bytes(src):
    """รวมสตริงลิเทอรัลของภาษา C หลังเครื่องหมาย = จนถึง ; ออกมาเป็นไบต์ดิบ

    ต้องถอด escape แบบภาษา C เอง (octal ยาวไม่เกินสามหลัก) เพราะ U8g2
    เก็บข้อมูลฟอนต์เป็นสตริงที่เต็มไปด้วย \\NNN
    """
    body = src.split("=", 1)[1].rsplit(";", 1)[0]
    out = bytearray()
    for lit in re.findall(r'"((?:[^"\\]|\\.)*)"', body, re.S):
        i = 0
        while i < len(lit):
            c = lit[i]
            if c != "\\":
                out.append(ord(c))
                i += 1
                continue
            n = lit[i + 1]
            if n in "01234567":
                j, v = i + 1, 0
                while j < len(lit) and j < i + 4 and lit[j] in "01234567":
                    v = v * 8 + int(lit[j])
                    j += 1
                out.append(v & 0xFF)
                i = j
            elif n == "x":
                j = i + 2
                while j < len(lit) and lit[j] in "0123456789abcdefABCDEF":
                    j += 1
                out.append(int(lit[i + 2:j], 16) & 0xFF)
                i = j
            else:
                out.append(_ESC[n])
                i += 2
    return bytes(out)


class _BitReader:
    """u8g2_font_decode_get_unsigned_bits() / _signed_bits() — อ่านจากบิตต่ำขึ้นไป"""

    def __init__(self, data, pos):
        self.d, self.p, self.bit = data, pos, 0

    def u(self, cnt):
        val = self.d[self.p] >> self.bit
        end = self.bit + cnt
        if end >= 8:
            self.p += 1
            val |= self.d[self.p] << (8 - self.bit)
            end -= 8
        self.bit = end
        return val & ((1 << cnt) - 1)

    def s(self, cnt):
        return self.u(cnt) - (1 << (cnt - 1))


class U8g2Font:
    """ฟอนต์ U8g2 หนึ่งตัว — ชื่อตามที่ตัวจำลองบันทึกไว้ เช่น "5x8", "logisoso32" """

    def __init__(self, name):
        path = os.path.join(FONT_DIR, "u8g2_font_%s_tf.c" % name)
        src = open(path, encoding="latin-1").read()
        self.name = name
        self.data = _c_string_bytes(src)
        declared = int(re.search(r"\[(\d+)\]", src).group(1))
        # ความยาวที่ประกาศในไฟล์ = ข้อมูล + ศูนย์ปิดท้ายของสตริงภาษา C
        if len(self.data) + 1 != declared:
            raise ValueError("%s: ถอดข้อมูลได้ %d ไบต์ แต่ไฟล์ประกาศไว้ %d"
                             % (name, len(self.data) + 1, declared))
        d = self.data
        self.bits_0, self.bits_1 = d[2], d[3]
        self.bits_w, self.bits_h, self.bits_x, self.bits_y, self.bits_dx = d[4:9]
        self.ascent_A = d[13] if d[13] < 128 else d[13] - 256
        self.start_A = (d[17] << 8) | d[18]
        self.start_a = (d[19] << 8) | d[20]
        self._cache = {}

    def glyph(self, enc):
        """คืน (กว้าง, สูง, x, y, ระยะเลื่อน, แถวพิกเซล) หรือ None ถ้าไม่มีตัวนี้ในฟอนต์"""
        if enc in self._cache:
            return self._cache[enc]
        d = self.data
        p = 23                                   # U8G2_FONT_DATA_STRUCT_SIZE
        if enc >= ord("a"):
            p += self.start_a
        elif enc >= ord("A"):
            p += self.start_A
        while True:                              # u8g2_font_get_glyph_data()
            if d[p + 1] == 0:
                self._cache[enc] = None
                return None
            if d[p] == enc:
                break
            p += d[p + 1]

        br = _BitReader(d, p + 2)                # u8g2_font_setup_decode()
        w = br.u(self.bits_w)
        h = br.u(self.bits_h)
        x = br.s(self.bits_x)                    # u8g2_font_decode_glyph()
        y = br.s(self.bits_y)
        dx = br.s(self.bits_dx)
        rows = [[0] * w for _ in range(h)]
        if w > 0:
            pos = [0, 0]

            def run(length, fg):                 # u8g2_font_decode_len()
                cnt = length
                lx, ly = pos
                while True:
                    rem = w - lx
                    cur = cnt if cnt < rem else rem
                    if ly < h:
                        for k in range(cur):
                            rows[ly][lx + k] = fg
                    if cnt < rem:
                        break
                    cnt -= rem
                    lx = 0
                    ly += 1
                pos[0], pos[1] = lx + cnt, ly

            while True:
                a = br.u(self.bits_0)
                b = br.u(self.bits_1)
                while True:
                    run(a, 0)
                    run(b, 1)
                    if br.u(1) == 0:
                        break
                if pos[1] >= h:
                    break
        g = (w, h, x, y, dx, rows)
        self._cache[enc] = g
        return g

    def str_width(self, text):
        """ระยะเลื่อนรวมของสตริง แบบเดียวกับที่ u8g2_DrawStr() ขยับ x"""
        return sum((self.glyph(ord(c)) or (0, 0, 0, 0, 0, None))[4] for c in text)

    def draw_str(self, setpx, x, y, text, color, transparent=False):
        """u8g2_DrawStr(): y คือเส้นฐาน

        ค่าเริ่มต้นของ U8g2 คือโหมดทึบ (is_transparent = 0 ใน u8g2_setup.c)
        พื้นหลังภายในกรอบของตัวอักษรแต่ละตัวจึงถูกระบายด้วยสีตรงข้ามด้วย
        ตัวอักษรที่วางทับเส้นหรือกล่องจึงลบสิ่งที่อยู่ใต้กรอบตัวอักษรออก เหมือนจอจริง
        คืนค่า x ถัดจากตัวสุดท้าย
        """
        bg = 1 if color == 0 else 0
        for ch in text:
            g = self.glyph(ord(ch))
            if g is None:
                continue
            w, h, gx, gy, dx, rows = g
            if w > 0:
                tx, ty = x + gx, y - (h + gy)
                for r in range(h):
                    row = rows[r]
                    for c in range(w):
                        if row[c]:
                            setpx(tx + c, ty + r, color)
                        elif not transparent:
                            setpx(tx + c, ty + r, bg)
            x += dx
        return x


class GlcdFont:
    """ฟอนต์ 5x7 ในตัวของ Adafruit_GFX (glcdfont.c) — 5 ไบต์ต่อตัว หนึ่งไบต์ = หนึ่งคอลัมน์"""

    def __init__(self):
        src = open(os.path.join(FONT_DIR, "glcdfont.c"), encoding="latin-1").read()
        body = src.split("font[] PROGMEM = {", 1)[1].split("};", 1)[0]
        body = re.sub(r"//[^\n]*", "", body)
        body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
        self.data = bytes(int(v, 16) for v in re.findall(r"0x[0-9A-Fa-f]{2}", body))
        if len(self.data) != 256 * 5:
            raise ValueError("glcdfont.c: ได้ %d ไบต์ แต่ต้องได้ 1280" % len(self.data))

    def draw_char(self, fillrect, x, y, c, color, bg, size):
        """Adafruit_GFX::drawChar() ของฟอนต์ classic

        พื้นหลังระบายเฉพาะเมื่อ bg != color (setTextColor สองค่า)
        ช่องละ 6x8 พิกเซลต่อขนาด 1 คอลัมน์ที่หกคือช่องว่างระหว่างตัวอักษร
        """
        if c >= 176:                             # _cp437 == false (ค่าเริ่มต้น)
            c += 1
        c &= 0xFF
        for i in range(5):
            line = self.data[c * 5 + i]
            for j in range(8):
                if line & 1:
                    fillrect(x + i * size, y + j * size, size, size, color)
                elif bg != color:
                    fillrect(x + i * size, y + j * size, size, size, bg)
                line >>= 1
        if bg != color:
            fillrect(x + 5 * size, y, size, 8 * size, bg)


_u8g2 = {}


def u8g2_font(name):
    if name not in _u8g2:
        _u8g2[name] = U8g2Font(name)
    return _u8g2[name]


_glcd = []


def glcd_font():
    if not _glcd:
        _glcd.append(GlcdFont())
    return _glcd[0]
