#!/usr/bin/env python3
"""
ทดสอบ "เฟิร์มแวร์จริง กับ เซิร์ฟเวอร์พีซีจริง" ว่าตกลงกันตรงกัน (ไม่มีสตับฝั่งโปรโตคอล)
  1) ./gw emit    -> JSON ที่ Host จะส่งจริง
  2) ส่งเข้า server.Gateway.ingest() ตัวจริง แล้วตรวจว่าหน้าเว็บอ่านข้อมูลได้ครบ
  3) สั่งงานผ่านเซิร์ฟเวอร์ แล้วเอาคำตอบที่เซิร์ฟเวอร์ส่งกลับไปป้อน ./gw apply
  4) ตรวจว่าเฟิร์มแวร์ทำตามคำสั่งและรายงาน ack ตรงกับที่เซิร์ฟเวอร์รอ
"""
import json, os, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "server", "PC-Dashboard"))
import server  # noqa: E402

gw_bin = sys.argv[1]
tmp = tempfile.mkdtemp()
fails = 0


def check(ok, what):
    global fails
    print("  %-66s %s" % (what, "ผ่าน" if ok else "ไม่ผ่าน"))
    if not ok:
        fails += 1


class Clock:
    t = 1_800_000_005.0
    def __call__(self): return Clock.t


gw = server.Gateway(server.History(os.path.join(tmp, "e2e.sqlite")), clock=Clock())

up = os.path.join(tmp, "uplink.json")
subprocess.run([gw_bin, "emit", up], check=True)
payload = json.load(open(up, encoding="utf-8"))          # ต้องเป็น JSON ที่ถูกต้อง
print("1) JSON จากเฟิร์มแวร์")
check(payload["host"] == "AABBCCDDEEFF" and len(payload["stations"]) == 3, "เป็น JSON ถูกต้อง มี 3 เตียง")

reply0 = gw.ingest(payload)
check(reply0.startswith("T "), "เซิร์ฟเวอร์รับและตอบเวลากลับ")
d = gw.api_data("AABBCCDDEEFF")
for k in ("version", "pollMs", "netMode", "activeCount", "currentTime", "hostBatPct", "stations", "channel"):
    check(k in d, "หน้าเว็บอ่านฟิลด์ %s ได้" % k)
st = d["stations"][0]
need = ("id online running rssi link idConflict mac rxTotal battery totalDrops volumeMl flowRateHr "
        "msSinceLastDrop targetRate planVolume dropFactor patientName alertCode nearEndPct nearEndAck "
        "caseActive caseStart").split()
check(all(k in st for k in need), "ทุกฟิลด์ของการ์ดเตียงที่หน้าเว็บเดิมใช้มีครบ")
check(st["patientName"] == "ผู้ป่วย A" and st["targetRate"] == 60 and st["planVolume"] == 500,
      "ค่าตั้งเตียงสะท้อนกลับมาที่พีซี (พีซีเรียนรู้จาก Host ได้)")
check(d["channel"] == 6 and d["activeCount"] == 3, "ช่องสัญญาณ/จำนวนเตียงถูกส่งมา")

print("\n2) สั่งงานผ่านเซิร์ฟเวอร์ แล้วให้เฟิร์มแวร์ทำ")
gw.enqueue("AABBCCDDEEFF", "C", 1, {"rate": "75", "vol": "800", "df": 15, "near": 90, "name": "สมชาย ใจดี"})
gw.enqueue("AABBCCDDEEFF", "R", 2)
gw.enqueue("AABBCCDDEEFF", "A", 3)
gw.enqueue("AABBCCDDEEFF", "N", 0, {"count": 3})
reply = gw.ingest(payload)
rp = os.path.join(tmp, "reply.txt")
open(rp, "w", encoding="utf-8").write(reply)
out = subprocess.run([gw_bin, "apply", rp], check=True, capture_output=True, text=True).stdout
kv = dict(l.split("=", 1) for l in out.strip().splitlines())
check(kv["rate1"] == "75.0" and kv["vol1"] == "800", "อัตรา/ปริมาตรตามที่สั่ง")
check(kv["df1"] == "15" and kv["near1"] == "90", "drop factor / near-end ตามที่สั่ง")
check(kv["name1"] == "สมชาย ใจดี", "ชื่อภาษาไทยผ่านการเข้ารหัสไป-กลับครบ")
check(kv["reset2"] == "1", "เริ่มถุงใหม่ (resetSeq เพิ่ม)")
check(kv["nearAck3"] == "1", "รับทราบเตือน")
check(kv["synced"] == "1", "นาฬิกาถูกตั้งจากเวลาในคำตอบ")
check(kv["ack"] == "4", "เฟิร์มแวร์ ack เลขล่าสุดที่เซิร์ฟเวอร์ส่งมา")

print("\n3) ack กลับไปแล้วเซิร์ฟเวอร์เลิกส่งซ้ำ")
payload["ack"] = int(kv["ack"])
reply2 = gw.ingest(payload)
check(reply2.strip().count("\n") == 0, "ไม่เหลือคำสั่งค้าง")

print("\n%s" % ("ผ่านทุกข้อ" if not fails else "มีข้อที่ไม่ผ่าน %d ข้อ" % fails))
sys.exit(1 if fails else 0)
