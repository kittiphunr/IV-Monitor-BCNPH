#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ประกอบภาพหน้าจอทีละเฟรมเป็นภาพเคลื่อนไหว GIF

ใช้: make_gif.py ผลลัพธ์.gif มิลลิวินาทีต่อเฟรม เฟรม1.png [เฟรม2.png ...]

ภาพจากตัวเรนเดอร์ขยาย 3 เท่า ที่นี่ย่อเหลือ 2 เท่าของจอจริงแบบไม่เกลี่ยสี
(ทุกพิกเซลของจอเป็นบล็อก 3x3 สีเดียว การสุ่มแบบ NEAREST จึงไม่ทำให้เพี้ยน)
เฟรมที่เหมือนเฟรมก่อนหน้าทุกพิกเซลจะถูกรวมเป็นเฟรมเดียวที่ค้างนานขึ้น
จังหวะเวลาจึงเท่าของจริง แต่ไฟล์เล็กลงมาก เพราะตัวละครยืนนิ่งระหว่างหยดเป็นส่วนใหญ่
"""
import sys
from PIL import Image


def main(out, ms, paths):
    frames, durs = [], []
    for p in paths:
        im = Image.open(p).convert("RGB")
        im = im.resize((im.width * 2 // 3, im.height * 2 // 3), Image.NEAREST)
        if frames and im.tobytes() == frames[-1].tobytes():
            durs[-1] += ms
        else:
            frames.append(im)
            durs.append(ms)
    pal = [f.quantize(colors=64, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE) for f in frames]
    pal[0].save(out, save_all=True, append_images=pal[1:], duration=durs, loop=0, optimize=False, disposal=1)
    print("%s %d เฟรม (จากทั้งหมด %d) %.1f วินาที" % (out, len(frames), len(paths), sum(durs) / 1000.0))


if __name__ == "__main__":
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    main(sys.argv[1], int(sys.argv[2]), sys.argv[3:])
