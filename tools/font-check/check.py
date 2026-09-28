#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ตรวจว่าฟอนต์ตัวจริงที่ใช้เรนเดอร์ภาพจำลอง ถอดรหัสได้ถูกต้องและตรงกับสมมติฐานในเฟิร์มแวร์

  1) ทุกฟอนต์ถอดตัวอักษร ASCII 32-126 ได้ครบ และทุกตัวถอดจบพอดีความสูง
  2) ฟอนต์ความกว้างคงที่ เลื่อนตัวละเท่าตัวเลขในชื่อ (5x8 = 5 พิกเซล)
  3) ค่าคงที่ ADV_* ในเฟิร์มแวร์ ตรงกับระยะเลื่อนจริงของฟอนต์
  4) u8g2_widths.h ตรงกับฟอนต์ปัจจุบัน (ไม่มีใครลืมรันสคริปต์สร้างใหม่)
  5) ฟอนต์ 5x7 ของ Adafruit_GFX มีครบ 256 ตัว
"""
import os, re, subprocess, sys, tempfile, shutil

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SP = os.path.join(ROOT, "tools", "screen-preview")
sys.path.insert(0, SP)
from bitmapfont import u8g2_font, glcd_font  # noqa: E402

FONTS = ["4x6", "5x8", "6x10", "7x13", "7x14B", "helvB10", "helvB12", "logisoso16", "logisoso32"]
FIXED = {"4x6": 4, "5x8": 5, "6x10": 6, "7x13": 7, "7x14B": 7}
fail = 0


def check(ok, msg):
    global fail
    print("  %-62s %s" % (msg, "ผ่าน" if ok else "*** ไม่ผ่าน ***"))
    if not ok:
        fail += 1


print("1-2) ถอดรหัสฟอนต์ U8g2")
for n in FONTS:
    f = u8g2_font(n)
    missing = [chr(c) for c in range(32, 127) if f.glyph(c) is None]
    check(not missing, "%-11s ครบทุกตัว ASCII 32-126" % n)
    if n in FIXED:
        advs = {f.glyph(c)[4] for c in range(32, 127)}
        check(advs == {FIXED[n]}, "%-11s เลื่อนตัวละ %d พิกเซลทุกตัว" % (n, FIXED[n]))

print("\n3) ค่าคงที่ในเฟิร์มแวร์ตรงกับฟอนต์จริง")
FONT_OF = {"4X6": "4x6", "5X8": "5x8", "6X10": "6x10", "7X13": "7x13", "LOGI": "logisoso16"}
for root, _, files in os.walk(os.path.join(ROOT, "firmware")):
    for fn in files:
        if not fn.endswith((".ino", ".h")):
            continue
        src = open(os.path.join(root, fn), encoding="utf-8", errors="replace").read()
        for key, val in re.findall(r"#define\s+ADV_(\w+)\s+(\d+)", src):
            if key not in FONT_OF:
                continue
            f = u8g2_font(FONT_OF[key])
            real = f.glyph(ord("0"))[4]
            rel = os.path.relpath(os.path.join(root, fn), ROOT)
            check(int(val) == real, "%s ADV_%s = %s (จริง %d)" % (rel, key, val, real))

print("\n4) u8g2_widths.h ตรงกับฟอนต์ปัจจุบัน")
tmp = tempfile.mkdtemp()
try:
    for f in os.listdir(SP):
        if f.endswith(".py"):
            shutil.copy(os.path.join(SP, f), tmp)
    shutil.copytree(os.path.join(SP, "fonts"), os.path.join(tmp, "fonts"))
    subprocess.run([sys.executable, "gen_u8g2_widths.py"], cwd=tmp, check=True, capture_output=True)
    same = open(os.path.join(tmp, "u8g2_widths.h")).read() == open(os.path.join(SP, "u8g2_widths.h")).read()
    check(same, "สร้างใหม่แล้วได้ไฟล์เดิมทุกไบต์ (ถ้าไม่ผ่าน ให้รัน gen_u8g2_widths.py)")
finally:
    shutil.rmtree(tmp)

print("\n5) ฟอนต์ Adafruit_GFX")
check(len(glcd_font().data) == 1280, "glcdfont.c มี 256 ตัว ตัวละ 5 ไบต์")

print("\n%s (%d ข้อที่ไม่ผ่าน)" % ("ผ่านทั้งหมด" if not fail else "มีข้อที่ไม่ผ่าน", fail))
sys.exit(1 if fail else 0)
