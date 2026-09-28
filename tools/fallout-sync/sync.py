#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""รักษา Station-Fallout.ino ให้ตรงกับ Station.ino ทุกบรรทัด ยกเว้นส่วนที่เป็นธีม

สาย Fallout ต่างจากสายหลักแค่ "หน้าตา" ตรรกะการนับหยด การแจ้งเตือน และ ESP-NOW
ต้องเหมือนกันทุกบรรทัด ไม่อย่างนั้นการแก้บั๊กด้านความปลอดภัยในสายหลักจะไม่ไหลมาถึง
Arduino IDE ไม่ยอมให้สเก็ตช์ #include ไฟล์จากโฟลเดอร์อื่น จึงต้องเก็บเป็นสำเนา
แล้วใช้สคริปต์นี้คุมให้สำเนาไม่เพี้ยน

สิ่งที่ยอมให้ต่างได้มีแค่สามอย่าง
  1) บล็อกหัวไฟล์ Doxygen (ชื่อไฟล์ เวอร์ชัน คำอธิบาย และแถวประวัติของสาย Fallout)
  2) บรรทัด #define APP_VERSION
  3) ค่าสีใน #define THEME_* / COLOR_* / UI_* (ชุดสีจอเขียวแบบ Pip-Boy อยู่ในตาราง THEME ข้างล่าง)
ส่วน StationScreen.h ของสาย Fallout เป็นของสายนี้เอง (หน้า 1 วาดคนเดินแทนถุงน้ำเกลือ)
และ drop_detector.h ต้องเหมือนของสายหลักทุกไบต์ เช่นเดียวกับสายบอร์ดเล็ก

ใช้:
  python3 sync.py            ตรวจเฉย ๆ (คืนค่า 1 ถ้าไม่ตรง)
  python3 sync.py --write    สร้าง Station-Fallout.ino ใหม่จาก Station.ino ปัจจุบัน
"""
import io, os, re, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(ROOT, "firmware", "Station", "Station.ino")
DST = os.path.join(ROOT, "firmware", "Station-Fallout", "Station-Fallout.ino")
DET_SRC = os.path.join(ROOT, "firmware", "Station", "drop_detector.h")
DET_DST = os.path.join(ROOT, "firmware", "Station-Fallout", "drop_detector.h")

# ชุดสีจอเขียวแบบ Pip-Boy — ค่าสีมาจากจานสี 16 ระดับของภาพ statpip0.bmp
# ในโครงการ PipBoy_2040 ของ Adafruit (ดู tools/fallout-sprite/SOURCE.md) แปลงเป็น RGB565
# พื้นจอจึงเป็นสีเดียวกับพื้นของภาพตัวละครพอดี วาดภาพทับได้โดยไม่เห็นขอบ
# สีเตือนวิกฤต (แดง ส้ม เหลือง) คงไว้ตามสายหลัก เพราะพยาบาลแยกความเร่งด่วนด้วยสี
# ธีมต้องไม่ลบความหมายนั้นทิ้ง
THEME = {
    "THEME_BG":      "0x08C0",   # พื้นจอ #091B05
    "THEME_SKYBLUE": "0x778A",   # เขียวสว่าง #77F350 แทนฟ้า (ค่าเป้าหมาย SET ฯลฯ)
    "THEME_CYAN":    "0x778A",   # แถบความคืบหน้า
    "THEME_PINK":    "0x778A",   # หัวหน้าคาลิเบรต
    "THEME_NAVY":    "0x11A1",   # พื้นการ์ด #16370E
    "THEME_DARKBOX": "0x11A1",
    "THEME_MAROON":  "0x6000",
    "COLOR_WHITE":   "0x778A",   # ตัวอักษรหลักเป็นเขียวสว่างแทนขาว
    "COLOR_GREEN":   "0x778A",
    "COLOR_RED":     "0xF800",   # คงเดิม — เหตุวิกฤต
    "COLOR_ORANGE":  "0xFD20",   # คงเดิม — ใกล้หมด
    "COLOR_YELLOW":  "0xFFE0",   # คงเดิม — หยุดชั่วคราว
    "UI_CARD":       "0x11A1",
    "UI_BAR":        "0x1161",   # แถบบนสุด #112D0A
    "UI_LINE":       "0x3364",   # เส้นคั่น #316F20
    "UI_DIM":        "0x5E07",   # ตัวอักษรรอง #5BC23D
    "UI_GREEN":      "0x778A",
}

DEFINE = re.compile(r"^(#define\s+)(THEME_\w+|COLOR_\w+|UI_\w+)(\s+)(0x[0-9A-Fa-f]+)(.*)$")
APPV = re.compile(r'^#define\s+APP_VERSION\s+"[^"]*"\s*$')


def split_header(text):
    end = text.index(" */\n") + 4
    return text[:end], text[end:]


def normalize(text):
    """ตัดส่วนที่ยอมให้ต่างทิ้ง แล้วคืนเนื้อที่ต้องเหมือนกันทุกบรรทัด"""
    _, body = split_header(text)
    out = []
    for line in body.split("\n"):
        if APPV.match(line):
            out.append("#define APP_VERSION <<ต่างได้>>")
            continue
        m = DEFINE.match(line)
        if m and m.group(2) in THEME:
            out.append(m.group(1) + m.group(2) + m.group(3) + "<<สี>>" + m.group(5))
            continue
        out.append(line)
    return out


def fallout_rows(header):
    return [l for l in header.split("\n") if re.match(r"^ \* \|\s*\d+\.\d+\.\d+-FO\s*\|", l)]


def build(src_text, dst_text):
    """สร้างไฟล์ Fallout ใหม่: หัวไฟล์ของ Station ที่แปลงเป็น Fallout + เนื้อของ Station + ธีม"""
    s_head, s_body = split_header(src_text)
    d_head, _ = split_header(dst_text)
    fo_version = re.search(r'#define\s+APP_VERSION\s+"([^"]+)"', dst_text).group(1)
    rows = fallout_rows(d_head)
    date = re.search(r"^ \* @date\s+(\S+)", d_head, re.M).group(1)
    brief = re.search(r"^ \* @brief\s+(.+)$", d_head, re.M).group(1)
    fo_desc = re.search(r"( \* @par Fallout Edition\n(?: \*.*\n)*?) \*\n \* @par Revision History",
                        d_head).group(1)

    h = s_head
    h = re.sub(r"^( \* @file\s+).*$", r"\g<1>Station-Fallout.ino", h, count=1, flags=re.M)
    h = re.sub(r"^( \* @brief\s+).*$", lambda m: m.group(1) + brief, h, count=1, flags=re.M)
    h = re.sub(r"^( \* @version\s+).*$", lambda m: m.group(1) + fo_version, h, count=1, flags=re.M)
    h = re.sub(r"^( \* @date\s+).*$", lambda m: m.group(1) + date, h, count=1, flags=re.M)
    h = h.replace(" * @par Revision History\n", fo_desc + " *\n * @par Revision History\n", 1)
    mark = " * |---|---|---|\n"
    h = h.replace(mark, mark + "".join(r + "\n" for r in rows), 1)

    body_lines = []
    for line in s_body.split("\n"):
        if APPV.match(line):
            body_lines.append('#define APP_VERSION         "%s"' % fo_version)
            continue
        m = DEFINE.match(line)
        if m and m.group(2) in THEME:
            line = m.group(1) + m.group(2) + m.group(3) + THEME[m.group(2)] + m.group(5)
        body_lines.append(line)
    return h + "\n".join(body_lines)


def main():
    src = io.open(SRC, encoding="utf-8").read()
    dst = io.open(DST, encoding="utf-8").read()
    if "--write" in sys.argv:
        new = build(src, dst)
        io.open(DST, "w", encoding="utf-8").write(new)
        dst = new
        print("เขียน %s ใหม่จาก Station.ino แล้ว" % os.path.relpath(DST, ROOT))
        io.open(DET_DST, "wb").write(io.open(DET_SRC, "rb").read())

    if io.open(DET_SRC, "rb").read() != io.open(DET_DST, "rb").read():
        print("*** firmware/Station-Fallout/drop_detector.h ไม่ตรงกับของสายหลักทุกไบต์ ***")
        print("แก้โดยรัน: python3 tools/fallout-sync/sync.py --write")
        return 1

    a, b = normalize(src), normalize(dst)
    if a != b:
        import difflib
        print("*** Station-Fallout.ino ไม่ตรงกับ Station.ino นอกส่วนธีม ***")
        for l in list(difflib.unified_diff(a, b, "Station.ino", "Station-Fallout.ino", n=1, lineterm=""))[:40]:
            print("   " + l)
        print("แก้โดยรัน: python3 tools/fallout-sync/sync.py --write")
        return 1
    # ทุกสีในตารางต้องถูกใช้จริง (กันพิมพ์ชื่อผิดแล้วธีมไม่เปลี่ยนเงียบ ๆ)
    for name, val in THEME.items():
        if not re.search(r"^#define\s+%s\s+%s\b" % (name, re.escape(val)), dst, re.M):
            print("*** ไม่พบ #define %s %s ใน Station-Fallout.ino ***" % (name, val))
            return 1
    print("Station-Fallout.ino ตรงกับ Station.ino ทุกบรรทัดนอกส่วนธีม (%d บรรทัด)" % len(a))
    print("drop_detector.h ตรงกับของสายหลักทุกไบต์")
    return 0


if __name__ == "__main__":
    sys.exit(main())
