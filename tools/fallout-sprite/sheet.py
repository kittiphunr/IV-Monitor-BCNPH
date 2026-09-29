#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""วาดทุกเฟรมของ Vault Boy จากข้อมูลที่เฟิร์มแวร์เก็บจริงใน pipboy_sprite.h

ใช้: sheet.py ผลลัพธ์.png [ที่อยู่ pipboy_sprite.h]

อ่านจานสี FO_SPR_PAL ข้อมูลพิกเซล FO_SPR_DATA ลำดับการเล่น FO_WALK_SEQ และเวลา FO_WALK_MS
จากไฟล์ .h โดยตรง แล้วถอดพิกเซลละ 4 บิตแบบเดียวกับ foDrawSprite() ในเฟิร์มแวร์
ภาพจึงเป็นสิ่งที่จอจะแสดงจริง ไม่ใช่ภาพจากต้นทางก่อนแปลง

แถวบน   ทุกเฟรมที่เก็บไว้ ขยาย 3 เท่า พร้อมเลขเฟรม (และบอกว่าเฟรมไหนคือท่ายืน)
แถวล่าง ลำดับการเล่นวนหนึ่งรอบตามจริง ขนาดจอจริง พร้อมเวลาของแต่ละช่อง
"""
import os
import re
import sys
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "screen-preview"))
from make_sheet import draw_caption, BG  # noqa: E402


def c565(v):
    r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
    return (r * 255 // 31, g * 255 // 63, b * 255 // 31)


def parse(path):
    src = open(path, encoding="utf-8").read()
    # ตัดคอมเมนต์ทิ้งก่อน เพราะคอมเมนต์ "// เฟรม 1" ในอาร์เรย์มีตัวเลขปนอยู่
    src = re.sub(r"/\*.*?\*/", "", src, flags=re.S)
    src = re.sub(r"//[^\n]*", "", src)
    d = {k: int(v) for k, v in re.findall(r"#define\s+(FO_\w+)\s+(\d+)", src)}
    arr = lambda name: [int(x, 0) for x in re.findall(r"0x[0-9A-Fa-f]+|\d+",
                        re.search(name + r"[^=]*=\s*\{(.*?)\};", src, re.S).group(1))]
    pal = [c565(v) for v in arr(r"FO_SPR_PAL\[16\]")]
    data = arr(r"FO_SPR_DATA\[FO_SPR_FRAMES\]\[FO_SPR_H \* FO_SPR_ROW\]")
    if len(data) != d["FO_SPR_FRAMES"] * d["FO_SPR_H"] * d["FO_SPR_ROW"]:
        sys.exit("FO_SPR_DATA มี %d ไบต์ ไม่ตรงกับขนาดที่ประกาศ" % len(data))
    seq = arr(r"FO_WALK_SEQ\[FO_WALK_LEN\]")
    ms = arr(r"FO_WALK_MS\[FO_WALK_LEN\]")
    return d, pal, data, seq, ms


def frame_img(d, pal, data, f):
    W, H, ROW = d["FO_SPR_W"], d["FO_SPR_H"], d["FO_SPR_ROW"]
    base = f * H * ROW
    im = Image.new("RGB", (W, H))
    for r in range(H):
        for c in range(W):
            b = data[base + r * ROW + c // 2]
            im.putpixel((c, r), pal[(b & 0x0F) if c & 1 else (b >> 4)])
    return im


def main(out, hpath):
    d, pal, data, seq, ms = parse(hpath)
    W, H, N = d["FO_SPR_W"], d["FO_SPR_H"], d["FO_SPR_FRAMES"]
    frames = [frame_img(d, pal, data, f) for f in range(N)]
    S, PAD, CAP = 3, 16, 30
    big_w, small_w = W * S + PAD, W + 18
    width = max(PAD + N * big_w, PAD + len(seq) * small_w) + PAD
    height = PAD + CAP + H * S + PAD + CAP + CAP + H + PAD
    sheet = Image.new("RGB", (width, height), BG)
    y = PAD
    for f, im in enumerate(frames):
        x = PAD + f * big_w
        tag = "F%d%s" % (f, " STAND" if f == d["FO_SPR_STAND"] else "")
        draw_caption(sheet, x, y, tag)
        sheet.paste(im.resize((W * S, H * S), Image.NEAREST), (x, y + CAP))
    y += CAP + H * S + PAD
    draw_caption(sheet, PAD, y, "LOOP %d SLOTS, %d ms" % (len(seq), sum(ms)))
    y += CAP
    for n, (f, t) in enumerate(zip(seq, ms)):
        x = PAD + n * small_w
        draw_caption(sheet, x, y, "F%d" % f)
        sheet.paste(frames[f], (x, y + CAP))
    sheet.save(out)
    print("%s %s (%d เฟรม, วน %d ช่อง)" % (out, sheet.size, N, len(seq)))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else
         os.path.join(ROOT, "firmware", "Station-Fallout", "pipboy_sprite.h"))
