#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""สร้าง u8g2_widths.h — ตารางขนาดตัวอักษรของฟอนต์ U8g2 ให้ตัวจำลองบน PC

ตัวจำลอง (U8g2lib.h) ต้องตอบ u8g2.getStrWidth() ให้ตรงกับจอจริง เฟิร์มแวร์
จึงจัดกึ่งกลางข้อความด้วยฟอนต์ความกว้างไม่คงที่ (logisoso, helvB) ได้ถูกต้อง
ตารางนี้สร้างจากไฟล์ฟอนต์ตัวจริงใน fonts/ ด้วยตัวถอดรหัสเดียวกับที่ใช้เรนเดอร์ภาพ
ห้ามแก้ไฟล์ .h ด้วยมือ ให้รันสคริปต์นี้ใหม่แทน
"""
import os
from bitmapfont import u8g2_font

FONTS = ["4x6", "5x8", "6x10", "7x13", "7x14B", "helvB10", "helvB12", "logisoso16", "logisoso32"]
HERE = os.path.dirname(os.path.abspath(__file__))

out = ["// สร้างอัตโนมัติด้วย gen_u8g2_widths.py จากฟอนต์ตัวจริงใน fonts/ — ห้ามแก้ด้วยมือ",
       "// ต่อหนึ่งตัวอักษร (ASCII 32-126): {ระยะเลื่อน dx, ความกว้างภาพ w, ระยะเยื้อง x}",
       "#pragma once", "#include <cstdint>", "#include <cstring>", "",
       "struct U8g2GlyphMetric { int8_t dx, w, x; };", ""]
for n in FONTS:
    f = u8g2_font(n)
    cells = []
    for c in range(32, 127):
        g = f.glyph(c)
        cells.append("{%d,%d,%d}" % ((g[4], g[0], g[2]) if g else (0, 0, 0)))
    ident = n.replace("x", "x")
    out.append("static const U8g2GlyphMetric u8g2_metric_%s[95] = {" % ident)
    for i in range(0, 95, 12):
        out.append("  " + ",".join(cells[i:i + 12]) + ",")
    out.append("};")
out.append("")
out.append("inline const U8g2GlyphMetric* u8g2_metrics_for(const char* name) {")
for n in FONTS:
    out.append('  if (strcmp(name, "%s") == 0) return u8g2_metric_%s;' % (n, n))
out.append("  return u8g2_metric_5x8;")
out.append("}")
open(os.path.join(HERE, "u8g2_widths.h"), "w", encoding="utf-8").write("\n".join(out) + "\n")
print("เขียน u8g2_widths.h แล้ว (%d ฟอนต์)" % len(FONTS))
