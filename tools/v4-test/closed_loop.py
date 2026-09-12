#!/usr/bin/env python3
"""
@file      closed_loop.py
@brief     ทดสอบวงจรปิด: Station-Thin จริง + Host-TFT-Gateway จริง + เซิร์ฟเวอร์พีซีจริง วิ่งคู่กันบนนาฬิกาจำลอง
@version   1.0.0-V4
@date      2026-11-19

ไม่มีสตับฝั่งโปรโตคอลเลย ทุกไบต์ที่ข้ามระหว่างเครื่องคือแพ็กเก็ตที่เฟิร์มแวร์สร้างจริง
ตัวจำลองใส่ให้แค่สามอย่าง: หยดตามเวลา แพ็กเก็ตหาย และเหตุผิดปกติ (หยุดไหล พักการนับ Host หาย คำสั่งจากพีซี)
"""
import json, os, random, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "server", "PC-Dashboard"))
import server  # noqa: E402

ST_BIN, HOST_BIN = sys.argv[1], sys.argv[2]
SOFF, HOFF = 200000, 50000          # นาฬิกาสองเครื่องเริ่มไม่เท่ากัน (มีผลเฉพาะถ้าโค้ดพึ่งเวลาร่วมกัน ซึ่งต้องไม่พึ่ง)
LOSS = 0.20                          # แพ็กเก็ตหายทั้งสองทิศ
fails = 0


def check(ok, what):
    global fails
    print("  %-76s %s" % (what, "ผ่าน" if ok else "ไม่ผ่าน"))
    if not ok:
        fails += 1


class Proc:
    def __init__(self, path):
        self.p = subprocess.Popen([path, "sim"], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  text=True, bufsize=1)

    def send(self, line):
        self.p.stdin.write(line + "\n")
        self.p.stdin.flush()

    def ask(self, line, stop):
        self.send(line)
        out = []
        while True:
            l = self.p.stdout.readline().strip()
            if not l:
                raise RuntimeError("process ended")
            if l.startswith(stop):
                return out, l
            out.append(l)

    def tick(self, t):
        out, _ = self.ask("T %d" % t, "OK")
        return [l[2:] for l in out if l.startswith("S ")]

    def state(self):
        _, l = self.ask("Q", "STATE")
        return dict(re.findall(r"(\w+)=(.*?)(?=\s+\w+=|$)", l[len("STATE "):]))


class Clock:
    t = 1_800_000_005.0
    def __call__(self): return Clock.t


random.seed(7)
st, host = Proc(ST_BIN), Proc(HOST_BIN)
gw = server.Gateway(server.History(os.path.join(tempfile.mkdtemp(), "cl.sqlite")), clock=Clock())

v = 0                    # เวลาจำลอง (ms)
rate_ml_h = 100.0
drop_on = True
next_drop = 1800.0
drops_sent = 0
down_blocked = False
lastPC = 0


def run(seconds):
    global v, next_drop, drops_sent, lastPC
    end = v + seconds * 1000
    while v < end:
        v += 10
        Clock.t = 1_800_000_005.0 + v / 1000.0
        if drop_on and rate_ml_h > 0:
            iv = 3600000.0 / (rate_ml_h * 20)
            while next_drop <= v:
                next_drop += iv * (1 + random.gauss(0, 0.05))
                st.tick(v + SOFF)
                st.send("D")
        else:
            next_drop = v + 3600000.0 / (max(rate_ml_h, 1) * 20)
        for pkt in st.tick(v + SOFF):                        # Station -> Host
            if random.random() >= LOSS:
                host.tick(v + HOFF + random.randint(0, 6))
                host.send("U " + pkt)
        for pkt in host.tick(v + HOFF):                      # Host -> Station
            if not down_blocked and random.random() >= LOSS:
                st.send("P " + pkt)
        if v - lastPC >= 1000:                               # Host -> พีซี -> Host ทุกวินาที
            lastPC = v
            _, l = host.ask("J", "J ")
            reply = gw.ingest(json.loads(l[2:]))
            for line in reply.strip().split("\n"):
                host.send("R " + line)
            gw.tick()


print("1) เดินปกติ 100 mL/h เป้า 100 แพ็กเก็ตหาย 20%% ทั้งสองทิศ (90 วินาที)")
run(90)
s, h = st.state(), host.state()
d = gw.api_data("%s" % json.loads(host.ask("J", "J ")[1][2:])["host"])
check(s["host"] == "1", "Station ได้ยิน Host")
check(abs(float(s["rate"]) - 100) / 100 < 0.06, "อัตราที่ Station แสดง ≈ 100 (%s)" % s["rate"])
check(abs(float(s["vol"]) - int(h["drops"]) / 20.0) < 1.0, "ปริมาตรที่ Station แสดงตรงกับหยดสะสม (ต่างไม่เกิน 1 mL จากความหน่วง)")
check(s["alert"] == "0" and s["status"] == "NORMAL", "ไม่เตือน สถานะ NORMAL")
check(int(s["minutes"]) > 0 and int(s["pct"]) >= 0, "นาทีที่เหลือและ %% มีค่า (%s นาที %s%%)" % (s["minutes"], s["pct"]))
check(h["rateValid"] == "1", "Host รายงานอัตราใช้ได้")
check(abs(d["stations"][0]["flowRateHr"] - 100) / 100 < 0.06 and d["hostOnline"], "พีซีเห็นอัตราเดียวกันและ Host ออนไลน์")

print("\n2) อัตราเร็วเกินเป้า (180 mL/h) 40 วินาที")
rate_ml_h = 180.0; next_drop = v + 100
run(45)
s = st.state()
check(s["alert"] == "1" and s["status"] == "TOO FAST", "Station ขึ้น TOO FAST (ตัดสินที่ Host ส่งมา)")

print("\n3) กลับมาตรงเป้า")
rate_ml_h = 100.0; next_drop = v + 100
run(60)
s = st.state()
check(s["alert"] == "0", "เตือนหายเมื่ออัตรากลับมาปกติ")

print("\n4) หยดหยุด (สายพับ)")
drop_on = False
run(25)
s = st.state()
check(s["alert"] == "5" and s["status"] == "NO FLOW", "Station ขึ้น NO FLOW จากรหัสที่ Host ส่งมา (ไม่ได้ตัดสินเอง)")
check(float(s["rate"]) == 0.0, "อัตราที่แสดงเป็น 0")
drop_on = True; next_drop = v + 500
run(40)
s = st.state()
check(s["alert"] == "0" and float(s["rate"]) > 80, "หยดกลับมา: เตือนหายและอัตรากลับมาคำนวณ")

print("\n5) พักการนับที่เตียง (กดปุ่มหยุดชั่วคราว)")
st.send("G")
drop_on = False
run(15)
s, h = st.state(), host.state()
check(s["running"] == "0" and s["status"] == "PAUSED", "Station ขึ้น PAUSED")
check(h["alert"] == "0", "Host ไม่เตือนระหว่างหยุดชั่วคราว (ไม่เห็นเป็นสายพับ)")
st.send("G"); drop_on = True; next_drop = v + 800
run(45)
s, h = st.state(), host.state()
check(s["running"] == "1" and h["alert"] == "0", "กลับมานับแล้วไม่มีเตือนผิด (เวลาจากหยดล่าสุดไม่รวมช่วงที่หยุด)")
check(float(s["rate"]) > 80, "อัตรากลับมาคำนวณใหม่ได้")

print("\n6) พีซีสั่งเปลี่ยนค่าเตียง")
gw.enqueue(json.loads(host.ask("J", "J ")[1][2:])["host"], "C", 1, {"rate": "150", "vol": "500"})
run(6)
s = st.state()
check(abs(float(s["target"]) - 150) < 0.1, "เป้าหมายใหม่ 150 ไปถึงจอ Station (พีซี → Host → Station)")
check(int(host.state()["ack"]) >= 1, "Host ack คำสั่งกลับพีซี")

print("\n7) พีซีสั่งเริ่มถุงใหม่")
before = int(st.state()["drops"])
gw.enqueue(json.loads(host.ask("J", "J ")[1][2:])["host"], "R", 1)
run(8)
s = st.state()
check(before > 100 and int(s["drops"]) < 40, "ยอดหยดที่ Station ถูกล้าง แล้วเริ่มนับใหม่ (%d -> %s)" % (before, s["drops"]))

print("\n8) Host หาย 12 วินาที แล้วกลับมา")
down_blocked = True
run(12)
s = st.state()
check(s["host"] == "0" and s["status"] == "NO HOST", "Station ขึ้น NO HOST ไม่ขึ้น NORMAL")
check(float(s["rate"]) == 0 and s["minutes"] == "-1", "ไม่โชว์อัตรา/นาทีที่เหลือที่ไม่มีใครยืนยัน")
down_blocked = False
run(6)
s = st.state()
check(s["host"] == "1" and s["status"] != "NO HOST", "Host กลับมา จอกลับเป็นปกติ")

print("\n9) ประวัติที่พีซี")
dd = json.loads(host.ask("J", "J ")[1][2:])
rows = gw.history.db.execute("SELECT COUNT(*), SUM(drops) FROM logs WHERE host=?", (dd["host"],)).fetchone()
check(rows[0] >= 5 and rows[1] > 100, "พีซีบันทึกรายนาทีได้ต่อเนื่องทุกถุง (%d แถว รวม %s หยด)" % (rows[0], rows[1]))

st.send("X"); host.send("X")
print("\n%s" % ("ผ่านทุกข้อ" if not fails else "มีข้อที่ไม่ผ่าน %d ข้อ" % fails))
sys.exit(1 if fails else 0)
