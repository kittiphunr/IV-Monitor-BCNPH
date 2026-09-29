#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""สร้าง firmware/Station-Fallout/pipboy_sprite.h จากภาพ Vault Boy ท่าเดิน

มีต้นทางได้สามแบบ เลือกให้อัตโนมัติตามลำดับนี้
  0) walk/NN.jpg  เฟรมภาพเคลื่อนไหวต้นฉบับทีละไฟล์ (ตอนนี้ใช้แบบนี้ 21 เฟรม) — ใช้ทุกเฟรม
     เรียงตามเลขในชื่อไฟล์ เฟรมละ WALK_MS ท่ายืนคือไฟล์ที่ WALK_STAND_FILE
     เฟรมที่ได้ผลลัพธ์เหมือนกันทุกพิกเซลหลังแปลงเก็บครั้งเดียว ลำดับการเล่นยังครบทุกไฟล์
  1) vaultboy_walk.gif  ภาพเคลื่อนไหวต้นฉบับ (เช่น 21 เฟรม) — ถ้ามีไฟล์นี้ ใช้ทุกเฟรมตามลำดับ
     และตามเวลาของแต่ละเฟรมในไฟล์ ไม่สร้างท่าเอง ท่ายืน (หยุดชั่วคราว / ไม่ไหล) คือเฟรม
     GIF_STAND ทุกเฟรมผ่านขั้นลบเส้นสแกนเดิม ย่อ แปลงจานสี และใส่เส้นสแกนใหม่แบบเดียวกัน
  2) vaultboy_walk.webp ภาพนิ่งท่าเดียว — ใช้เมื่อยังไม่มีไฟล์ GIF สร้างท่าเดินเองตามวิธีข้างล่าง

ต้นทางแบบที่ 2 คือ vaultboy_walk.webp (358x498 เส้นสแกนแบบจอ CRT พื้นดำ) ที่ผู้ดูแลโครงการส่งมาเป็นแบบ
ภาพนั้นมีท่าเดียว คือท่าก้าวยาว สคริปต์นี้ขยับขาเป็นท่าเดินต่อจากท่านั้น แล้วย่อลงขนาดจอ

วิธีขยับขา (ทำที่ความละเอียดต้นทาง แล้วค่อยย่อ ภาพจึงไม่แตก)
  - ลำตัวตั้งแต่แถว CUT ขึ้นไปคงเดิม ขาแยกซ้ายขวาด้วยเส้น SPLIT_X
  - หน้าแข้ง "เฉือน" ในแนวนอน ระยะเลื่อนแปรตามความลึกจากแถว CUT
    ขอบบนของขาจึงติดลำตัวเสมอ ไม่มีรอยต่อ และแถวพิกเซลยังเป็นแนวนอน
  - รองเท้าเลื่อนทั้งชิ้นตามข้อเท้า รูปรองเท้าไม่บิดเบี้ยว
  - ท่าขาสวนกัน: ขาทั้งสองตั้งตรงใต้สะโพก เท้าหลังยกพ้นพื้น ลำตัวสูงขึ้น (ช่องว่างระหว่าง
    ลำตัวกับขาเติมด้วยแถวบนสุดของขา เหมือนขายืดออก)
  ท่านี้กลับซ้ายขวาให้อีกขานำไม่ได้ เพราะขาหลังในภาพต้นทางสั้นกว่า (ยกส้นเท้า)
  เท้าสองข้างจะชนกัน หนึ่งก้าวจึงเป็น ก้าวยาว -> ขาสวนกัน -> ก้าวยาว

  เฟรม 0  ท่าก้าวยาว (ท่าต้นฉบับ)          ยืนรอหยดถัดไปในท่านี้
  เฟรม 1  หนึ่งในสามของทาง
  เฟรม 2  สองในสามของทาง
  เฟรม 3  ท่าขาสวนกัน เท้าหลังยก ตัวสูงสุด
  เฟรม 4  ยืนขาตรง เท้าติดพื้น (หยุดชั่วคราว / ไม่ไหล)

ลำดับการเล่นวนซ้ำ 0 1 2 3 2 1 เฟรมละ SYNTH_MS

หลังย่อ แปลงความสว่างเป็นจานสี 16 ระดับ จากสีพื้นจอ (THEME_BG) ถึงเขียวสว่าง
แล้วใส่เส้นสแกนใหม่แถวเว้นแถว (เส้นสแกนเดิมของต้นทางถูกลบก่อนขยับขา
เพราะถ้าเลื่อนรองเท้าขึ้นลง เส้นเดิมจะเหลื่อมกับของลำตัว) เส้นสแกนผูกกับแถวของจอ
ไม่ขยับตามตัวละคร เหมือนจอ CRT จริง

ใช้:
  python3 make_sprite.py            สร้างไฟล์ .h ใหม่
  python3 make_sprite.py --check    ตรวจว่าไฟล์ .h ตรงกับที่สคริปต์นี้สร้าง (คืนค่า 1 ถ้าไม่ตรง)
  python3 make_sprite.py --png ไฟล์  เรนเดอร์ทุกเฟรมเป็นภาพ PNG ไว้ดูด้วยตา
"""
import io
import os
import re
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
SRC = os.path.join(HERE, "vaultboy_walk.webp")
GIF = os.path.join(HERE, "vaultboy_walk.gif")
GIF_STAND = 0                 # เฟรมของ GIF ที่ใช้เป็นท่ายืน
WALK_DIR = os.path.join(HERE, "walk")
WALK_MS = 90                  # 11 เฟรมต่อหนึ่งรอบก้าว (สองก้าว) = ราว 1 วินาที ใกล้จังหวะเดินจริง
WALK_STAND_FILE = 1           # ท่ายืน = walk/01.jpg ท่าเดียวกับภาพนิ่งอ้างอิงเดิม
SYNTH_SEQ = [0, 1, 2, 3, 2, 1]
SYNTH_MS = 90
DST = os.path.join(ROOT, "firmware", "Station-Fallout", "pipboy_sprite.h")
sys.path.insert(0, os.path.join(ROOT, "tools", "fallout-sync"))
from sync import THEME  # noqa: E402

# ---- ตำแหน่งในภาพต้นทาง 358x498 ----
TH = 22                       # ค่าเขียวที่ถือว่าเป็นตัวละคร (พื้นดำสนิท)
CUT = 366                     # แถวแรกที่เป็นขา เหนือขึ้นไปคือลำตัว
SPLIT_X = 180                 # ซ้ายของเส้นนี้คือขาหลัง ขวาคือขาหน้า
REAR_ANKLE_Y, FRONT_ANKLE_Y = 415, 440
SHEAR_REAR, SHEAR_FRONT = 0.61, -0.31   # เฉือนเท่านี้ ข้อเท้าจะมาอยู่ใต้สะโพกพอดี
LIFT = 12                     # ท่าขาสวนกัน เท้าหลังยกกี่พิกเซล (ต้นทาง)
BOB = 6                       # ท่าขาสวนกัน ลำตัวสูงขึ้นกี่พิกเซล (ต้นทาง)
PAD_TOP = BOB + 2

# ---- ขนาดบนจอ ----
OUT_H = 128                   # ความสูงของภาพบนจอ (กรอบตัวละครสูง 180)
VMAX = 140.0                  # ความสว่างของเนื้อสีในภาพต้นทาง = เขียวสว่างสุด
GAMMA = 1.8                   # มากกว่า 1 = เส้นขอบดำเข้มขึ้น เนื้อสียังสว่างเกือบเต็ม
SCAN = 0.62                   # แถวเส้นสแกนมืดลงเหลือเท่านี้
BRIGHT = (119, 243, 80)       # เขียวสว่างของจานสี Pip-Boy (ตรงกับ COLOR_GREEN)
BG = (9, 27, 5)               # พื้นจอ (ตรงกับ THEME_BG)

#          เฉือน  ยกเท้าหลัง  ลำตัวสูงขึ้น
POSES = [(0.0,   0.0,  0.0),
         (1 / 3., 1 / 3., 1 / 3.),
         (2 / 3., 2 / 3., 2 / 3.),
         (1.0,   1.0,  1.0),
         (1.0,   0.0,  0.0)]


def rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def descan(im):
    """ช่องเขียวของภาพ หลังลบเส้นสแกนเดิม"""
    w, h = im.size
    g0 = im.convert("RGB").getchannel("G").tobytes()
    # ลบเส้นสแกนเดิม: ค่าสูงสุดในหน้าต่างแนวตั้ง 4 แถว (แถบมืดของเส้นสแกนหนาไม่เกิน 3 แถว)
    g = bytearray(w * h)
    for y in range(h):
        rows = [min(h - 1, max(0, yy)) * w for yy in range(y - 1, y + 3)]
        for x in range(w):
            g[y * w + x] = max(g0[r + x] for r in rows)
    return g


def load():
    im = Image.open(SRC).convert("RGB")
    if im.size != (358, 498):
        sys.exit("vaultboy_walk.webp ต้องมีขนาด 358x498")
    w, h = im.size
    return w, h, descan(im)


def compose(w, h, g, shear, lift, bob):
    """หนึ่งท่าที่ความละเอียดต้นทาง คืนภาพขาวดำ (ความสว่าง = ช่องเขียว)"""
    H = h + PAD_TOP
    out = bytearray(w * H)
    b = int(round(BOB * bob))
    fl = LIFT * lift

    def src(x, y):
        return g[y * w + x] if 0 <= x < w and 0 <= y < h else 0

    # ขา: หาพิกเซลต้นทางย้อนจากปลายทาง (ไม่มีรู) ขาหลังก่อน ขาหน้าทับ
    for rear in (True, False):
        c = (SHEAR_REAR if rear else SHEAR_FRONT) * shear
        ay = REAR_ANKLE_Y if rear else FRONT_ANKLE_Y
        foot_dx = c * (ay - CUT)
        for y in range(CUT - b, h):
            for x in range(w):
                v = 0
                # รองเท้า (ยกเฉพาะขาหลัง ซึ่งเป็นขาที่กำลังเหวี่ยง)
                sx, sy = int(round(x - foot_dx)), int(round(y + (fl if rear else 0)))
                if CUT <= sy < h and ((sx < SPLIT_X) == rear) and is_foot(rear, sx, sy):
                    v = src(sx, sy)
                if v <= TH:
                    yy = max(y, CUT)                 # แถวเหนือ CUT = ขายืดขึ้นไปหาลำตัว
                    sx = int(round(x - c * (yy - CUT)))
                    if ((sx < SPLIT_X) == rear) and not is_foot(rear, sx, yy):
                        v = src(sx, yy)
                if v > TH:
                    out[(y + PAD_TOP) * w + x] = v
    # ลำตัวทับบนสุด
    for y in range(CUT):
        dy = y - b + PAD_TOP
        for x in range(w):
            v = g[y * w + x]
            if v > TH:
                out[dy * w + x] = v
    return Image.frombytes("L", (w, H), bytes(out))


def is_foot(rear, x, y):
    return (y > 408 and x < 150) if rear else (y > 438)


def gif_frames():
    """ทุกเฟรมของ GIF ต้นฉบับ (Pillow รวมเฟรมตามวิธีทิ้งเฟรมของ GIF ให้แล้ว) กับเวลาของแต่ละเฟรม"""
    im = Image.open(GIF)
    big, ms = [], []
    for i in range(getattr(im, "n_frames", 1)):
        im.seek(i)
        fr = im.convert("RGB")
        big.append(Image.frombytes("L", fr.size, bytes(descan(fr))))
        ms.append(int(im.info.get("duration", 100)) or 100)
    return big, ms


def dir_files():
    if not os.path.isdir(WALK_DIR):
        return []
    fs = [f for f in os.listdir(WALK_DIR) if re.fullmatch(r"\d+\.(jpg|jpeg|png|webp)", f, re.I)]
    return sorted(fs, key=lambda f: int(f.split(".")[0]))


def dir_frames(files):
    big = []
    for f in files:
        fr = Image.open(os.path.join(WALK_DIR, f)).convert("RGB")
        big.append(Image.frombytes("L", fr.size, bytes(descan(fr))))
    return big


def frames():
    """คืน (เฟรมที่ย่อแล้ว, จานสี, ความกว้าง, ลำดับการเล่น, เวลาต่อช่อง, เฟรมท่ายืน, ที่มา, ชื่อต้นทางของแต่ละเฟรม)"""
    files = dir_files()
    if files:
        big = dir_frames(files)
        seq, ms, origin = list(range(len(big))), [WALK_MS] * len(big), "dir"
        stand = [int(f.split(".")[0]) for f in files].index(WALK_STAND_FILE)
        names = files
    elif os.path.exists(GIF):
        big, ms = gif_frames()
        seq, stand, origin = list(range(len(big))), GIF_STAND, "gif"
        names = ["GIF %d" % i for i in range(len(big))]
    else:
        w, h, g = load()
        big = [compose(w, h, g, *p) for p in POSES]
        seq, stand, origin = SYNTH_SEQ, 4, "webp"
        ms = [SYNTH_MS] * len(seq)
        names = ["pose %d" % i for i in range(len(big))]
    box = None
    for im in big:
        bb = im.point(lambda v: 255 if v > TH else 0).getbbox()
        box = bb if box is None else (min(box[0], bb[0]), min(box[1], bb[1]),
                                      max(box[2], bb[2]), max(box[3], bb[3]))
    bw, bh = box[2] - box[0], box[3] - box[1]
    ow = int(round(bw * OUT_H / float(bh)))
    out = []
    for im in big:
        small = im.crop(box).resize((ow, OUT_H), Image.BOX)
        px = small.tobytes()
        idx = []
        for y in range(OUT_H):
            row = []
            for x in range(ow):
                lv = min(1.0, px[y * ow + x] / VMAX) ** GAMMA
                if y % 2:
                    lv *= SCAN
                row.append(int(round(15 * lv)))
            idx.append(row)
        out.append(idx)
    # เก็บเฟรมที่เหมือนกันทุกพิกเซลครั้งเดียว ลำดับการเล่นชี้ไปที่เฟรมที่เก็บ
    uniq, remap, src = [], [], []
    for n, f in enumerate(out):
        if f in uniq:
            k = uniq.index(f)
            src[k].append(names[n])
        else:
            k = len(uniq)
            uniq.append(f)
            src.append([names[n]])
        remap.append(k)
    seq = [remap[i] for i in seq]
    stand = remap[stand]
    pal = [tuple(int(round(BG[k] + (BRIGHT[k] - BG[k]) * i / 15.0)) for k in range(3)) for i in range(16)]
    return uniq, pal, ow, seq, ms, stand, origin, src


def header_text():
    fr, pal, W, seq, ms, stand, origin, src = frames()
    H = OUT_H
    ROW = (W + 1) // 2
    pal565 = [rgb565(*c) for c in pal]
    if "0x%04X" % pal565[0] != THEME["THEME_BG"] or "0x%04X" % pal565[15] != THEME["COLOR_GREEN"]:
        sys.exit("จานสีของภาพไม่ตรงกับ THEME_BG / COLOR_GREEN ของสาย Fallout")

    L = []
    L.append("/**")
    L.append(" * @file      pipboy_sprite.h")
    L.append(" * @brief     ภาพ Vault Boy ท่าเดิน %d เฟรมสำหรับหน้า 1 ของสาย Fallout (ไฟล์สร้างอัตโนมัติ ห้ามแก้ด้วยมือ)" % len(fr))
    L.append(" * @version   1.4.0-FO")
    L.append(" * @date      2026-09-29")
    L.append(" * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>")
    L.append(" *")
    L.append(" * @par Description")
    if origin == "dir":
        L.append(" * สร้างด้วย `python3 tools/fallout-sprite/make_sprite.py` จากเฟรมต้นฉบับ `walk/NN.jpg` ทั้ง %d ไฟล์" % len(seq))
        L.append(" * (ที่มาอยู่ที่ tools/fallout-sprite/SOURCE.md) เล่นทุกไฟล์ตามลำดับ เฟรมละ %d ms ย่อเหลือ %dx%d" % (WALK_MS, W, H))
        L.append(" * ไฟล์ที่ได้ผลเหมือนกันทุกพิกเซลหลังแปลงเก็บครั้งเดียว (%d เฟรมที่เก็บ)" % len(fr))
    elif origin == "gif":
        L.append(" * สร้างด้วย `python3 tools/fallout-sprite/make_sprite.py` จากทุกเฟรมของ `vaultboy_walk.gif`")
        L.append(" * (ที่มาอยู่ที่ tools/fallout-sprite/SOURCE.md) ตามลำดับและเวลาของเฟรมในไฟล์ ย่อเหลือ %dx%d" % (W, H))
    else:
        L.append(" * สร้างด้วย `python3 tools/fallout-sprite/make_sprite.py` จากภาพนิ่ง `vaultboy_walk.webp`")
        L.append(" * (ที่มาอยู่ที่ tools/fallout-sprite/SOURCE.md) ขยับขาจากท่าต้นฉบับเป็นท่าเดิน แล้วย่อเหลือ %dx%d" % (W, H))
        L.append(" * เฟรม 0 ก้าวยาว · 1 และ 2 ระหว่างทาง · 3 ขาสวนกัน เท้าหลังยก · 4 ยืนขาตรง")
    L.append(" * เล่นวนตาม `FO_WALK_SEQ` / `FO_WALK_MS` เก็บพิกเซลละ 4 บิต (บิตสูงคือพิกเซลซ้าย)")
    L.append(" * ชี้เข้าจานสี `FO_SPR_PAL` ดัชนี 0 เท่ากับ THEME_BG จึงวาดทับของเดิมได้โดยไม่ต้องล้างพื้น")
    L.append(" * แถวคี่มืดลงเป็นเส้นสแกนแบบจอ CRT")
    L.append(" *")
    L.append(" * @note      Vault Boy และชื่อ Fallout เป็นเครื่องหมายการค้าของ Bethesda Softworks")
    L.append(" *            ใช้ในโครงการนี้เพื่อการศึกษาโดยไม่แสวงหากำไร")
    L.append(" *")
    L.append(" * @par Revision History")
    L.append(" * | Version | Date | Change |")
    L.append(" * |---|---|---|")
    L.append(" * | 1.4.0-FO | 2026-09-29 | ใช้เฟรมภาพเคลื่อนไหวต้นฉบับ 21 ไฟล์ครบทุกเฟรม แทนท่าที่สร้างจากภาพนิ่ง |")
    L.append(" * | 1.3.0-FO | 2026-09-28 | เพิ่มลำดับและเวลาการเล่นวน (FO_WALK_SEQ / FO_WALK_MS) อ่านทุกเฟรมจาก GIF ต้นฉบับได้ |")
    L.append(" * | 1.2.0-FO | 2026-09-28 | ภาพต้นทางคมขึ้นพร้อมเส้นสแกน ขาเฉือนแทนการหุบ เท้าหลังยก ลำตัวขึ้นลง และท่ายืน |")
    L.append(" * | 1.1.0-FO | 2026-09-28 | สร้างครั้งแรก จาก statpip0.bmp ของ Adafruit PipBoy_2040 สามเฟรม |")
    L.append(" */")
    L.append("")
    L.append("#pragma once")
    L.append("")
    L.append("#define FO_SPR_W       %d" % W)
    L.append("#define FO_SPR_H       %d" % H)
    L.append("#define FO_SPR_ROW     %d      // ไบต์ต่อแถว" % ROW)
    L.append("#define FO_SPR_FRAMES  %d" % len(fr))
    L.append("#define FO_SPR_STAND   %d       // ท่ายืน (หยุดชั่วคราว / ไม่ไหล)" % stand)
    L.append("#define FO_WALK_LEN    %d      // จำนวนช่องในลำดับการเล่นวน" % len(seq))
    L.append("")
    L.append("static const uint8_t FO_WALK_SEQ[FO_WALK_LEN] = {" + ", ".join(str(v) for v in seq) + "};")
    L.append("static const uint16_t FO_WALK_MS[FO_WALK_LEN] = {" + ", ".join(str(v) for v in ms) + "};")
    L.append("")
    L.append("static const uint16_t FO_SPR_PAL[16] = {")
    L.append("  " + ", ".join("0x%04X" % v for v in pal565[:8]) + ",")
    L.append("  " + ", ".join("0x%04X" % v for v in pal565[8:]))
    L.append("};")
    L.append("")
    L.append("static const uint8_t FO_SPR_DATA[FO_SPR_FRAMES][FO_SPR_H * FO_SPR_ROW] PROGMEM = {")
    for n, f in enumerate(fr):
        L.append("  {  // เฟรม %d จาก %s" % (n, ", ".join(src[n])))
        for row in f:
            r = row + [0] * (ROW * 2 - W)
            b = [(r[i] << 4) | r[i + 1] for i in range(0, len(r), 2)]
            L.append("    " + ",".join("0x%02X" % v for v in b) + ",")
        L.append("  }%s" % ("," if n < len(fr) - 1 else ""))
    L.append("};")
    L.append("")
    return "\n".join(L), (W, H, len(fr))


def main():
    if "--png" in sys.argv:
        out = sys.argv[sys.argv.index("--png") + 1]
        fr, pal, W = frames()[:3]
        sheet = Image.new("RGB", (len(fr) * (W + 8), OUT_H + 8), pal[0])
        for n, f in enumerate(fr):
            for y, row in enumerate(f):
                for x, v in enumerate(row):
                    sheet.putpixel((4 + n * (W + 8) + x, 4 + y), pal[v])
        sheet.resize((sheet.width * 4, sheet.height * 4), Image.NEAREST).save(out)
        print(out)
        return 0
    text, (W, H, n) = header_text()
    if "--check" in sys.argv:
        cur = io.open(DST, encoding="utf-8").read() if os.path.exists(DST) else ""
        if cur != text:
            print("*** pipboy_sprite.h ไม่ตรงกับที่ make_sprite.py สร้าง ***")
            print("แก้โดยรัน: python3 tools/fallout-sprite/make_sprite.py")
            return 1
        print("pipboy_sprite.h ตรงกับภาพต้นทาง (%dx%d px, %d เฟรม)" % (W, H, n))
        return 0
    io.open(DST, "w", encoding="utf-8").write(text)
    print("เขียน %s แล้ว (%dx%d px, %d เฟรม)" % (os.path.relpath(DST, ROOT), W, H, n))
    return 0


if __name__ == "__main__":
    sys.exit(main())
