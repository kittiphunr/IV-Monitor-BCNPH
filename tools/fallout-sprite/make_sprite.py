#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""สร้าง firmware/Station-Fallout/pipboy_sprite.h จากภาพ Vault Boy ใน statpip0.bmp

ภาพต้นทางมาจากโครงการ PipBoy_2040 ของ Adafruit (ที่มาและสัญญาอนุญาตอยู่ที่ SOURCE.md)
ภาพนั้นมี Vault Boy ท่าเดินแค่ท่าเดียว คือท่าก้าวยาว ขาหน้าขาหลังกางออก
สคริปต์นี้ตัดเฉพาะตัวละครออกมา แล้วสร้างอีกสองเฟรมจากท่าเดิม ด้วยการหุบขาเข้าหากัน
ตั้งแต่สะโพกลงไปถึงเท้า (หุบมากขึ้นเรื่อย ๆ ตามความลึกจากสะโพก) และยกตัวขึ้นทีละพิกเซล

  เฟรม 0  ท่าเดิม ก้าวยาว               (ยืนรอหยดถัดไปในท่านี้)
  เฟรม 1  หุบขาครึ่งทาง ตัวสูงขึ้น 1 px
  เฟรม 2  ขาชิดกัน (ท่าขาสวนกัน) ตัวสูงขึ้น 2 px

หนึ่งหยด = เล่นเฟรม 0 1 2 1 0 หนึ่งรอบ = หนึ่งก้าว

เก็บเป็นพิกเซลละ 4 บิต ชี้เข้าจานสี 16 สีของภาพต้นทาง (แปลงเป็น RGB565 แล้ว)
สีพื้นของภาพตรงกับ THEME_BG ของสาย Fallout พอดี (tools/fallout-sync/sync.py)
จึงวาดทั้งกรอบสี่เหลี่ยมทับของเดิมได้เลย ไม่ต้องล้างพื้นก่อน จอจึงไม่กะพริบ

ใช้:
  python3 make_sprite.py            สร้างไฟล์ .h ใหม่
  python3 make_sprite.py --check    ตรวจว่าไฟล์ .h ตรงกับที่สคริปต์นี้สร้าง (คืนค่า 1 ถ้าไม่ตรง)
  python3 make_sprite.py --png ไฟล์  เรนเดอร์ทั้งสามเฟรมเป็นภาพ PNG ไว้ดูด้วยตา
"""
import io
import os
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
SRC = os.path.join(HERE, "statpip0.bmp")
DST = os.path.join(ROOT, "firmware", "Station-Fallout", "pipboy_sprite.h")
sys.path.insert(0, os.path.join(ROOT, "tools", "fallout-sync"))
from sync import THEME  # noqa: E402

BG_INDEX = 2                        # สีพื้นของภาพต้นทาง (9, 27, 5)
X0, Y0, X1, Y1 = 112, 53, 160, 146  # กรอบตัวละครในภาพ 280x240 รวมขอบว่างรอบตัว
HIP, FOOT, CX = 123, 145, 137       # แถวสะโพก แถวปลายเท้า และแกนกลางลำตัว
LIFT_MAX = 2                        # เผื่อแถวด้านบนไว้ให้ยกตัวขึ้นได้สองพิกเซล
POSES = [(1.00, 0), (0.62, 1), (0.30, 2)]   # (สัดส่วนความกว้างของขาที่ปลายเท้า, ยกตัวกี่ px)

W = X1 - X0 + 1
H = Y1 - Y0 + 1 + LIFT_MAX
ROW = (W + 1) // 2


def rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def load():
    im = Image.open(SRC)
    if im.mode != "P" or im.size != (280, 240):
        sys.exit("statpip0.bmp ต้องเป็นภาพสีแบบจานสีขนาด 280x240")
    pal = im.getpalette()[:48]
    return im.load(), [tuple(pal[i * 3:i * 3 + 3]) for i in range(16)]


def frame(px, spread, lift):
    """หนึ่งเฟรม: ลำตัวคัดลอกตรง ๆ ส่วนขาหุบเข้าหาแกนกลางตามความลึกจากสะโพก"""
    out = [[BG_INDEX] * W for _ in range(H)]
    for y in range(Y0, Y1 + 1):
        dy = y - Y0 + LIFT_MAX - lift
        for x in range(X0, X1 + 1):
            if y < HIP:
                v = px[x, y]
            else:
                s = 1.0 - (1.0 - spread) * min(1.0, (y - HIP) / float(FOOT - HIP))
                sx = int(round(CX + (x - CX) / s))
                v = px[sx, y] if X0 <= sx <= X1 else BG_INDEX
            out[dy][x - X0] = v
    return out


def frames():
    px, pal = load()
    fr = [frame(px, s, l) for s, l in POSES]
    for f in fr:
        for row in f:
            if max(row) > 15:
                sys.exit("ภาพใช้สีเกิน 16 สี เก็บแบบ 4 บิตไม่ได้")
    return fr, pal


def header_text():
    fr, pal = frames()
    pal565 = [rgb565(*c) for c in pal]
    bg = "0x%04X" % pal565[BG_INDEX]
    if bg != THEME["THEME_BG"]:
        sys.exit("สีพื้นของภาพ (%s) ไม่ตรงกับ THEME_BG ของสาย Fallout (%s)" % (bg, THEME["THEME_BG"]))

    L = []
    L.append("/**")
    L.append(" * @file      pipboy_sprite.h")
    L.append(" * @brief     ภาพ Vault Boy สามเฟรมสำหรับหน้า 1 ของสาย Fallout (ไฟล์สร้างอัตโนมัติ ห้ามแก้ด้วยมือ)")
    L.append(" * @version   1.1.0-FO")
    L.append(" * @date      2026-09-28")
    L.append(" * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>")
    L.append(" *")
    L.append(" * @par Description")
    L.append(" * สร้างด้วย `python3 tools/fallout-sprite/make_sprite.py` จากภาพ `statpip0.bmp`")
    L.append(" * ของโครงการ PipBoy_2040 ของ Adafruit (สัญญาอนุญาต MIT ดู tools/fallout-sprite/SOURCE.md)")
    L.append(" * ภาพต้นทางมีท่าเดินท่าเดียว เฟรม 1 และ 2 สร้างจากท่าเดิมด้วยการหุบขาเข้าหากัน")
    L.append(" * เก็บพิกเซลละ 4 บิต (บิตสูงคือพิกเซลซ้าย) ชี้เข้าจานสี `FO_SPR_PAL`")
    L.append(" * สีพื้น (ดัชนี %d) เท่ากับ THEME_BG จึงวาดทับของเดิมได้โดยไม่ต้องล้างพื้น" % BG_INDEX)
    L.append(" *")
    L.append(" * @note      Vault Boy และชื่อ Fallout เป็นเครื่องหมายการค้าของ Bethesda Softworks")
    L.append(" *            ใช้ในโครงการนี้เพื่อการศึกษาโดยไม่แสวงหากำไร")
    L.append(" *")
    L.append(" * @par Revision History")
    L.append(" * | Version | Date | Change |")
    L.append(" * |---|---|---|")
    L.append(" * | 1.1.0-FO | 2026-09-28 | สร้างครั้งแรก |")
    L.append(" */")
    L.append("")
    L.append("#pragma once")
    L.append("")
    L.append("#define FO_SPR_W       %d" % W)
    L.append("#define FO_SPR_H       %d" % H)
    L.append("#define FO_SPR_ROW     %d      // ไบต์ต่อแถว" % ROW)
    L.append("#define FO_SPR_FRAMES  %d" % len(fr))
    L.append("")
    L.append("static const uint16_t FO_SPR_PAL[16] = {")
    L.append("  " + ", ".join("0x%04X" % v for v in pal565[:8]) + ",")
    L.append("  " + ", ".join("0x%04X" % v for v in pal565[8:]))
    L.append("};")
    L.append("")
    L.append("static const uint8_t FO_SPR_DATA[FO_SPR_FRAMES][FO_SPR_H * FO_SPR_ROW] PROGMEM = {")
    for n, f in enumerate(fr):
        L.append("  {  // เฟรม %d" % n)
        for row in f:
            r = row + [BG_INDEX] * (ROW * 2 - W)
            b = [(r[i] << 4) | r[i + 1] for i in range(0, len(r), 2)]
            L.append("    " + ",".join("0x%02X" % v for v in b) + ",")
        L.append("  }%s" % ("," if n < len(fr) - 1 else ""))
    L.append("};")
    L.append("")
    return "\n".join(L)


def main():
    text = header_text()
    if "--png" in sys.argv:
        out = sys.argv[sys.argv.index("--png") + 1]
        fr, pal = frames()
        sheet = Image.new("RGB", (len(fr) * (W + 8), H + 8), pal[BG_INDEX])
        for n, f in enumerate(fr):
            for y, row in enumerate(f):
                for x, v in enumerate(row):
                    sheet.putpixel((4 + n * (W + 8) + x, 4 + y), pal[v])
        sheet.resize((sheet.width * 4, sheet.height * 4), Image.NEAREST).save(out)
        print(out)
        return 0
    if "--check" in sys.argv:
        cur = io.open(DST, encoding="utf-8").read() if os.path.exists(DST) else ""
        if cur != text:
            print("*** pipboy_sprite.h ไม่ตรงกับที่ make_sprite.py สร้าง ***")
            print("แก้โดยรัน: python3 tools/fallout-sprite/make_sprite.py")
            return 1
        print("pipboy_sprite.h ตรงกับภาพต้นทาง (%dx%d px, %d เฟรม)" % (W, H, len(POSES)))
        return 0
    io.open(DST, "w", encoding="utf-8").write(text)
    print("เขียน %s แล้ว (%dx%d px, %d เฟรม)" % (os.path.relpath(DST, ROOT), W, H, len(POSES)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
