#!/usr/bin/env python3
"""ตรวจว่าโครงสร้างแพ็กเก็ต ESP-NOW ของทุกเฟิร์มแวร์ตรงกันจริง (Protocol v3)

Host กับ Station ส่งโครงสร้างนี้หากันตรง ๆ ถ้าลำดับหรือชนิดของฟิลด์ต่างกันแม้ไบต์เดียว
ข้อมูลจะเพี้ยนทั้งระบบโดยไม่มีอะไรฟ้อง สคริปต์นี้ดึงโครงสร้างจากไฟล์ .ino ทุกตัว
แล้วเทียบกันทีละฟิลด์ ใช้เป็นด่านกันพลาดก่อน merge

ใช้: python3 check_protocol.py     (คืนค่า 1 เมื่อพบความต่าง)
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
STRUCTS = ("struct_message", "struct_host_sync")
SIZES = {"uint8_t": 1, "int8_t": 1, "uint16_t": 2, "int16_t": 2,
         "uint32_t": 4, "int32_t": 4, "float": 4}


def extract(path: Path, name: str):
    """ดึงรายชื่อ (ชนิด, ฟิลด์) ของโครงสร้างจากไฟล์ .ino"""
    text = path.read_text(encoding="utf-8")
    m = re.search(r"typedef struct __attribute__\(\(packed\)\) " + name +
                  r"\s*\{(.*?)\}\s*" + name + r"\s*;", text, re.S)
    if not m:
        return None
    fields = []
    for line in m.group(1).splitlines():
        line = line.split("//")[0].strip()
        if not line or not line.endswith(";"):
            continue
        parts = line[:-1].split()
        if len(parts) >= 2:
            fields.append((parts[0], parts[1]))
    return fields


def size_of(fields):
    return sum(SIZES.get(t, -9999) for t, _ in fields)


def check_no_signal_capability() -> int:
    """ตรวจว่า Station ทุกสายประกาศความสามารถ NO DROP ตรงกับที่ Host ใช้อ่าน (บิต 0x02 ของ struct_message.flags) และรหัสเตือน 6 ตรงกัน

    Host ส่งรหัส 6 ให้เฉพาะเตียงที่ประกาศบิตนี้ ถ้า Station สายใดลืมใส่บิต หรือเลขบิตไม่ตรง
    เครื่องนั้นจะไม่ได้รับ NO DROP โดยไม่มีอะไรฟ้อง จึงตรวจจากซอร์สตรง ๆ
    """
    print("\n=== ความสามารถ NO DROP (รหัส 6)")
    problems = 0
    host = (ROOT / "firmware/Host-CYD28/protocol.h").read_text(encoding="utf-8")
    hf = re.search(r"MSG_FLAG_NO_SIGNAL_OK\s*=\s*(0x[0-9A-Fa-f]+)", host)
    hc = re.search(r"ALERT_NO_SIGNAL\s*=\s*(\d+)", host)
    if not hf or not hc:
        print("  Host-CYD28 ไม่ประกาศ MSG_FLAG_NO_SIGNAL_OK หรือ ALERT_NO_SIGNAL")
        return 1
    flag, code = int(hf.group(1), 16), int(hc.group(1))
    print(f"  {'Host-CYD28':34s} บิต 0x{flag:02X}  รหัส {code}  (ตัวอ้างอิง)")
    for ino in sorted(ROOT.glob("firmware/Station*/*.ino")):
        text = ino.read_text(encoding="utf-8")
        label = ino.parent.name
        mf = re.search(r"#define\s+MSG_FLAG_NO_SIGNAL_OK\s+(0x[0-9A-Fa-f]+)", text)
        mc = re.search(r"#define\s+ALERT_NO_SIGNAL\s+(\d+)", text)
        sets = re.search(r"myData\.flags\s*=[^;]*\|\s*MSG_FLAG_NO_SIGNAL_OK", text)
        ok = bool(mf and mc and sets and int(mf.group(1), 16) == flag and int(mc.group(1)) == code)
        print(f"  {label:34s} {'ตรงกัน และประกาศบิตทุกแพ็กเก็ต' if ok else '** ไม่ตรง/ไม่ประกาศ **'}")
        if not ok:
            problems += 1
    return problems


def main() -> int:
    inos = sorted(ROOT.glob("firmware/*/*.ino")) + sorted(ROOT.glob("firmware/*/protocol.h"))   # Host-CYD28 รุ่นเขียนใหม่เก็บโครงสร้างใน protocol.h
    problems = 0

    for name in STRUCTS:
        print(f"\n=== {name}")
        reference = None
        ref_file = None
        for ino in inos:
            fields = extract(ino, name)
            if fields is None:
                continue
            label = ino.parent.name
            if reference is None:
                reference, ref_file = fields, label
                print(f"  {label:34s} {size_of(fields):3d} ไบต์  (ใช้เป็นตัวอ้างอิง)")
                continue
            if fields == reference:
                print(f"  {label:34s} {size_of(fields):3d} ไบต์  ตรงกัน")
            else:
                problems += 1
                print(f"  {label:34s} {size_of(fields):3d} ไบต์  ** ไม่ตรงกับ {ref_file} **")
                for i in range(max(len(fields), len(reference))):
                    a = reference[i] if i < len(reference) else ("-", "-")
                    b = fields[i] if i < len(fields) else ("-", "-")
                    if a != b:
                        print(f"      ฟิลด์ที่ {i}: อ้างอิง {a[0]} {a[1]}  <->  พบ {b[0]} {b[1]}")

    problems += check_no_signal_capability()
    print()
    if problems:
        print(f"พบความต่าง {problems} จุด — Host กับ Station จะสื่อสารกันเพี้ยน")
        return 1
    print("โครงสร้างแพ็กเก็ตตรงกันทุกเฟิร์มแวร์")
    return 0


if __name__ == "__main__":
    sys.exit(main())
