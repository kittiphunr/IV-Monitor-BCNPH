#!/usr/bin/env python3
"""
@file      extract_dashboard.py
@brief     ดึงหน้าเว็บ Dashboard จาก firmware/Host-OLED/web_dashboard.h มาเป็น static/dashboard.html
@version   1.0.0-PC
@date      2026-11-19

หน้าเว็บมีต้นฉบับที่เดียวคือ web_dashboard.h (เฟิร์มแวร์ Host-OLED) ไฟล์ static/dashboard.html
เป็นสำเนาที่สร้างจากต้นฉบับ ห้ามแก้ด้วยมือ

    python3 extract_dashboard.py          เขียนทับ static/dashboard.html
    python3 extract_dashboard.py --check  ตรวจว่าตรงกับต้นฉบับ (ออกด้วยรหัส 1 ถ้าไม่ตรง)
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "..", "firmware", "Host-OLED", "web_dashboard.h")
DST = os.path.join(HERE, "static", "dashboard.html")
OPEN = 'const char PAGE_INDEX[] PROGMEM = R"rawliteral('
CLOSE = ')rawliteral";'


def extract():
    with open(SRC, encoding="utf-8") as f:
        t = f.read()
    a = t.index(OPEN) + len(OPEN)
    b = t.index(CLOSE, a)
    return t[a:b]


if __name__ == "__main__":
    html = extract()
    if "--check" in sys.argv:
        try:
            cur = open(DST, encoding="utf-8").read()
        except FileNotFoundError:
            cur = None
        if cur != html:
            print("static/dashboard.html ไม่ตรงกับ web_dashboard.h — รัน python3 extract_dashboard.py")
            sys.exit(1)
        print("ตรง: static/dashboard.html = web_dashboard.h")
    else:
        os.makedirs(os.path.dirname(DST), exist_ok=True)
        with open(DST, "w", encoding="utf-8", newline="") as f:
            f.write(html)
        print("เขียน %s (%d ไบต์)" % (DST, len(html.encode("utf-8"))))
