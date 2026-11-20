#!/usr/bin/env python3
"""
@file      server.py
@brief     เซิร์ฟเวอร์ Dashboard ฝั่งพีซีของ Smart IV Alert (รับข้อมูลจาก Host-Gateway)
@version   1.0.0-PC
@date      2026-11-19

Revision History
| Version | Date       | Change |
|---------|------------|--------|
| 1.0.0-PC | 2026-11-19 | รุ่นแรก รับข้อมูลจาก Host-Gateway เก็บประวัติลง SQLite เสิร์ฟหน้าเว็บเดิมของ Host แทนตัวเครื่อง |

ทำไมต้องมีโปรแกรมนี้
    Host-OLED เดิมต้องทำเว็บเซิร์ฟเวอร์ เก็บล็อก สร้าง CSV และดูแลมือถือหลายเครื่อง
    ทั้งหมดบนชิป ESP32 ตัวเดียวกับที่ต้องรับ ESP-NOW จากทุกเตียง งานฝั่งผู้ใช้จึงแย่งเวลา
    งานเฝ้าเตียง โปรแกรมนี้ย้ายงานเหล่านั้นมาไว้บนพีซี Host-Gateway เหลือแค่รับ ESP-NOW
    ตัดสินเตือน แจ้งเตือนที่หน้างาน แล้วส่งสรุปมาที่นี่วินาทีละหนึ่งครั้ง

    พีซีล่มหรือเราเตอร์ล่ม ไม่กระทบการเตือนหน้างาน เพราะ Host-Gateway ตัดสินเตือนเองเสมอ

ใช้เฉพาะไลบรารีมาตรฐานของ Python 3.8 ขึ้นไป ไม่ต้อง pip install อะไร

โปรโตคอลกับ Host-Gateway (ดู docs/GATEWAY-PROTOCOL.md)
    POST /api/ingest   Host ส่งสถานะทุกเตียงเป็น JSON หนึ่งก้อน
                       ตอบกลับเป็นข้อความหลายบรรทัด: เวลา (T) และคำสั่งที่ยังไม่ได้รับรู้ (C/R/A/N)
"""

import argparse
import base64
import csv
import io
import json
import os
import re
import socket
import sqlite3
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, quote, urlparse

SERVER_VERSION = "1.0.0-PC"
HERE = os.path.dirname(os.path.abspath(__file__))
DASHBOARD_PATH = os.path.join(HERE, "static", "dashboard.html")

# ---- ค่าเดียวกับเฟิร์มแวร์ (ต้องตรงกัน) ----
MAX_STATIONS = 8
ALLOWED_DF = (10, 15, 20, 60)
NEAR_MIN, NEAR_MAX = 50, 95
RATE_MAX = 1000.0
VOLUME_MAX = 10000.0
HOST_STALE_S = 5.0          # เงียบเกินนี้ถือว่า Host หลุดจากพีซี
CMD_BATCH = 8               # ส่งคำสั่งค้างกลับไปไม่เกินเท่านี้ต่อหนึ่งคำตอบ
CMD_TTL_S = 600             # คำสั่งที่ Host ไม่รับรู้เกิน 10 นาที ทิ้ง (กันคำสั่งเก่าค้างข้ามคืน)

ALERT_TEXT_EN = {
    0: "NORMAL", 1: "TOO FAST", 2: "TOO SLOW", 3: "NEXT BAG", 4: "BAG EMPTY",
    5: "OCCLUSION", 6: "NO SIGNAL", 7: "BED LOST",
}

CSV_HEADER = ("Record_Index,Date_Time,Drops_Per_Min,Drop_Factor,Total_Volume_mL,"
              "Flow_Rate_mL_hr,Target_Rate_mL_hr,Plan_Volume_mL,Near_End_Alert_Pct,"
              "Alert_Code,Alert_Text,RSSI_dBm,Battery_Volts")


def safe_df(df):
    return df if df in ALLOWED_DF else 20


def safe_near(p):
    return p if NEAR_MIN <= p <= NEAR_MAX else 80


def local_stamp(ts):
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(ts))


# ============================================================================
# ฐานข้อมูลประวัติ
# ============================================================================
class History:
    """เก็บบันทึกรายนาทีของทุกเตียง (เทียบเท่า LogEntry ที่เคยอยู่ใน RAM ของ Host)"""

    def __init__(self, path):
        self.lock = threading.Lock()
        self.db = sqlite3.connect(path, check_same_thread=False)
        self.db.execute("""CREATE TABLE IF NOT EXISTS logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            host TEXT NOT NULL, station INTEGER NOT NULL, bag INTEGER NOT NULL,
            idx INTEGER NOT NULL, ts REAL NOT NULL,
            drops INTEGER, volume REAL, rate REAL, target REAL,
            alert INTEGER, rssi INTEGER, battery REAL,
            df INTEGER, plan REAL, near INTEGER)""")
        self.db.execute("CREATE INDEX IF NOT EXISTS ix_logs ON logs(host, station, bag, idx)")
        self.db.execute("""CREATE TABLE IF NOT EXISTS bags (
            host TEXT NOT NULL, station INTEGER NOT NULL, bag INTEGER NOT NULL,
            PRIMARY KEY (host, station))""")
        self.db.commit()

    def current_bag(self, host, station):
        with self.lock:
            r = self.db.execute("SELECT bag FROM bags WHERE host=? AND station=?",
                                (host, station)).fetchone()
            return r[0] if r else 1

    def new_bag(self, host, station):
        with self.lock:
            r = self.db.execute("SELECT bag FROM bags WHERE host=? AND station=?",
                                (host, station)).fetchone()
            n = (r[0] if r else 1) + 1
            self.db.execute("INSERT OR REPLACE INTO bags VALUES (?,?,?)", (host, station, n))
            self.db.commit()
            return n

    def add(self, host, station, ts, row):
        bag = self.current_bag(host, station)
        with self.lock:
            r = self.db.execute(
                "SELECT COALESCE(MAX(idx),0) FROM logs WHERE host=? AND station=? AND bag=?",
                (host, station, bag)).fetchone()
            idx = r[0] + 1
            self.db.execute(
                "INSERT INTO logs (host,station,bag,idx,ts,drops,volume,rate,target,alert,"
                "rssi,battery,df,plan,near) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                (host, station, bag, idx, ts, row["drops"], row["volume"], row["rate"],
                 row["target"], row["alert"], row["rssi"], row["battery"], row["df"],
                 row["plan"], row["near"]))
            self.db.commit()

    def rows(self, host, station, bag=None, limit=None):
        if bag is None:
            bag = self.current_bag(host, station)
        q = ("SELECT idx,ts,drops,volume,rate,target,alert,rssi,battery,df,plan,near FROM logs "
             "WHERE host=? AND station=? AND bag=? ORDER BY idx")
        args = [host, station, bag]
        if limit:                      # เอาเฉพาะ N แถวล่าสุด แต่คืนเรียงเก่า -> ใหม่
            q = ("SELECT * FROM (SELECT idx,ts,drops,volume,rate,target,alert,rssi,battery,df,"
                 "plan,near FROM logs WHERE host=? AND station=? AND bag=? "
                 "ORDER BY idx DESC LIMIT ?) ORDER BY idx")
            args.append(limit)
        with self.lock:
            return self.db.execute(q, args).fetchall()


# ============================================================================
# สถานะของ Host แต่ละตัว
# ============================================================================
class HostState:
    def __init__(self, host_id):
        self.id = host_id
        self.snap = {}             # JSON ล่าสุดจาก Host
        self.last_seen = 0.0
        self.next_seq = 1
        self.cmds = []             # [{"seq","line","at"}] ที่ Host ยังไม่รับรู้
        # ตัวสะสมรายเตียงสำหรับสร้างบันทึกรายนาที
        self.acc = {}              # station -> {"last":int,"base":bool,"drops":int}

    def online(self, now=None):
        return (now or time.time()) - self.last_seen <= HOST_STALE_S


class Gateway:
    def __init__(self, history, token="", clock=time.time):
        self.history = history
        self.token = token
        self.clock = clock
        self.hosts = {}
        self.lock = threading.RLock()
        self.last_minute = int(clock() // 60)

    # ---- รับข้อมูลจาก Host ------------------------------------------------
    def ingest(self, data):
        host_id = str(data.get("host", "")).strip()
        if not re.fullmatch(r"[0-9A-Za-z_-]{1,32}", host_id):
            raise ValueError("bad host id")
        stations = data.get("stations", [])
        if not isinstance(stations, list) or len(stations) > MAX_STATIONS:
            raise ValueError("bad stations")
        now = self.clock()
        with self.lock:
            h = self.hosts.setdefault(host_id, HostState(host_id))
            ack = int(data.get("ack", 0) or 0)
            # พีซีรีสตาร์ตแล้วเลขลำดับเริ่มจาก 1 ใหม่ แต่ Host จำเลขที่เคยทำไว้ ถ้าไม่ตามให้ทัน
            # คำสั่งใหม่ทุกคำสั่งจะถูก Host ข้ามเพราะเลขต่ำกว่าที่เคยรับ (เงียบ ๆ ไม่มีใครรู้)
            h.next_seq = max(h.next_seq, ack + 1)
            h.cmds = [c for c in h.cmds if c["seq"] > ack and now - c["at"] < CMD_TTL_S]
            h.snap = data
            h.last_seen = now
            for st in stations:
                sid = int(st.get("id", 0))
                if not 1 <= sid <= MAX_STATIONS:
                    continue
                a = h.acc.setdefault(sid, {"last": 0, "base": False, "drops": 0})
                total = int(st.get("totalDrops", 0))
                if a["base"]:
                    a["drops"] += total - a["last"] if total >= a["last"] else total
                a["base"] = True
                a["last"] = total
            return self._response(h, now)

    def _response(self, h, now):
        lines = ["T %d" % int(now)]
        for c in h.cmds[:CMD_BATCH]:
            lines.append(c["line"])
        return "\n".join(lines) + "\n"

    # ---- คำสั่งจากหน้าเว็บ -------------------------------------------------
    def enqueue(self, host_id, kind, station, kv=None):
        with self.lock:
            h = self.hosts.get(host_id)
            if h is None:
                raise KeyError("unknown host")
            seq = h.next_seq
            h.next_seq += 1
            parts = [kind, str(seq), str(station)]
            for k, v in (kv or {}).items():
                parts.append("%s=%s" % (k, quote(str(v), safe="")))
            h.cmds.append({"seq": seq, "line": " ".join(parts), "at": self.clock()})
            if kind == "R":                      # เริ่มถุงใหม่: ตั้งฐานนับหยดใหม่ และเปิดชุดบันทึกใหม่
                h.acc[station] = {"last": 0, "base": False, "drops": 0}
                self.history.new_bag(host_id, station)
            return seq

    # ---- เขียนบันทึกรายนาที ------------------------------------------------
    def tick(self):
        """เรียกถี่ ๆ ได้ เขียนแถวใหม่ก็ต่อเมื่อข้ามนาที"""
        now = self.clock()
        minute = int(now // 60)
        if minute == self.last_minute:
            return
        self.last_minute = minute
        with self.lock:
            for h in self.hosts.values():
                if not h.online(now):
                    continue                     # Host เงียบ = ไม่มีข้อมูลจริงให้บันทึก ไม่แต่งตัวเลข
                count = int(h.snap.get("count", 0))
                by_id = {int(s.get("id", 0)): s for s in h.snap.get("stations", [])}
                for sid in range(1, min(count, MAX_STATIONS) + 1):
                    s = by_id.get(sid)
                    a = h.acc.setdefault(sid, {"last": 0, "base": False, "drops": 0})
                    drops, a["drops"] = a["drops"], 0
                    if s is None:
                        continue
                    df = safe_df(int(s.get("dropFactor", 20)))
                    online = bool(s.get("online"))
                    self.history.add(h.id, sid, now, {
                        "drops": min(drops, 65535),
                        "volume": float(s.get("volumeMl", 0)),
                        "rate": drops / df * 60.0,
                        "target": float(s.get("targetRate", 0)),
                        "alert": int(s.get("alertCode", 0)),
                        "rssi": int(s.get("rssi", 0)) if online else 0,
                        "battery": float(s.get("battery", 0)),
                        "df": df,
                        "plan": float(s.get("planVolume", 0)),
                        "near": safe_near(int(s.get("nearEndPct", 80))),
                    })

    # ---- JSON ให้หน้าเว็บ (รูปแบบเดียวกับ /api/data ของ Host-OLED) -----------
    def pick(self, host_id=None):
        with self.lock:
            if host_id and host_id in self.hosts:
                return self.hosts[host_id]
            if self.hosts:
                return sorted(self.hosts.values(), key=lambda x: x.id)[0]
            return None

    def api_data(self, host_id=None):
        now = self.clock()
        with self.lock:
            h = self.pick(host_id)
            if h is None:
                return {"version": SERVER_VERSION, "pollMs": 1000, "netMode": "PC", "netIP": "-",
                        "staRssi": 0, "activeCount": 5, "currentTime": local_stamp(now),
                        "timeSynced": True, "timeApprox": False, "apClients": 0, "apMaxClients": 0,
                        "channel": 0, "hostBatVolts": 0, "hostBatPct": 0, "snoozed": False,
                        "linkWeakPct": 60, "syncFail": 0, "heardBeyond": 0, "stations": [],
                        "hostOnline": False, "gatewayId": "-", "uplinkOk": False,
                        "noHost": True}
            d = dict(h.snap)
            online = h.online(now)
            d.update({
                "version": "%s + %s" % (SERVER_VERSION, h.snap.get("v", "?")),
                "pollMs": 1000, "netMode": "PC", "netIP": h.snap.get("ip", "-"),
                "activeCount": h.snap.get("count", 5),
                "currentTime": local_stamp(now), "timeSynced": True, "timeApprox": False,
                "apClients": 0, "apMaxClients": 0,
                "hostBatVolts": h.snap.get("bat", 0), "hostBatPct": h.snap.get("batPct", 0),
                "snoozed": h.snap.get("snooze", False),
                "staRssi": h.snap.get("rssi", 0),
                "hostOnline": online, "gatewayId": h.id, "uplinkOk": online,
                "gatewayAgeS": round(now - h.last_seen, 1),
            })
            # ฟิลด์ที่หน้าเว็บเดิมอ่าน แต่ Host ไม่ได้ส่งมา (หรือ Host รุ่นเก่ากว่า) ให้ค่าปริยายที่ไม่ทำให้หน้าพัง
            for k, v in (("channel", 0), ("syncFail", 0), ("heardBeyond", 0),
                         ("linkWeakPct", 60), ("resetReason", "")):
                d.setdefault(k, v)
            sts = []
            for s in h.snap.get("stations", []):
                s = dict(s)
                if not online:                    # Host เงียบ = ข้อมูลเก่า อย่าให้ดูเหมือนเตียงปกติ
                    s["online"] = False
                    s["rssi"] = 0
                    s["flowRateHr"] = 0
                sts.append(s)
            d["stations"] = sts
            return d

    def api_logs(self, host_id, station):
        h = self.pick(host_id)
        if h is None:
            return {"stationId": station, "dropFactor": 20, "totalLogs": 0, "logs": []}
        rows = self.history.rows(h.id, station, limit=600)
        df = 20
        for s in h.snap.get("stations", []):
            if int(s.get("id", 0)) == station:
                df = safe_df(int(s.get("dropFactor", 20)))
        logs = [{"min": r[0], "time": local_stamp(r[1]), "drops": r[2], "vol": round(r[3], 2),
                 "rate": round(r[4], 2), "target": round(r[5], 1), "alert": r[6],
                 "rssi": r[7], "battery": round(r[8], 2)} for r in rows]
        return {"stationId": station, "dropFactor": df, "totalLogs": len(logs), "logs": logs}

    def csv_for(self, host_id, station, all_bags=False):
        h = self.pick(host_id)
        out = io.StringIO()
        out.write("﻿" + CSV_HEADER + "\r\n")
        if h is not None:
            rows = self.history.rows(h.id, station)
            for r in rows:
                out.write("%d,%s,%d,%d,%.2f,%.2f,%.1f,%.0f,%d,%d,%s,%d,%.2f\r\n" % (
                    r[0], local_stamp(r[1]), r[2], r[9], r[3], r[4], r[5], r[10], r[11],
                    r[6], ALERT_TEXT_EN.get(r[6], "NORMAL"), r[7], r[8]))
        return out.getvalue()


# ============================================================================
# การตรวจค่าที่รับจากหน้าเว็บ (ข้อความเดียวกับที่ Host-OLED เคยตอบ)
# ============================================================================
def validate_config(form):
    """คืน (kv, error) — kv คือคีย์ที่จะส่งต่อให้ Host"""
    kv = {}
    try:
        if "rate" in form:
            r = float(form["rate"])
            if not 0 <= r <= RATE_MAX:
                return None, "Invalid rate"
            kv["rate"] = "%g" % r
        if "volume" in form:
            v = float(form["volume"])
            if not 0 <= v <= VOLUME_MAX:
                return None, "Invalid volume"
            kv["vol"] = "%g" % v
        if "df" in form:
            df = int(form["df"])
            if df not in ALLOWED_DF:
                return None, "Invalid drop factor"
            kv["df"] = df
        if "nearpct" in form:
            n = int(form["nearpct"])
            if not NEAR_MIN <= n <= NEAR_MAX:
                return None, "Invalid near-end percent (50-95)"
            kv["near"] = n
    except ValueError:
        return None, "Invalid number"
    if "name" in form:
        name = re.sub(r"[\x00-\x1f]", " ", form["name"])
        kv["name"] = name.encode("utf-8")[:63].decode("utf-8", "ignore")
    return kv, None


# ============================================================================
# หน้าเว็บ: ใช้หน้าเดิมของ Host แล้วปะส่วนที่ไม่เกี่ยวกับพีซี
# ============================================================================
PATCH_SCRIPT = r"""
<style>
  #gwBar{position:fixed;top:0;left:0;right:0;z-index:9999;display:none;background:#c53030;color:#fff;
    padding:9px 14px;font-size:14.5px;text-align:center;font-weight:600}
  #gwHostSel{position:fixed;right:8px;bottom:8px;z-index:9998;display:none;background:#fff;
    border:1.5px solid #cbd5e0;border-radius:8px;padding:5px 8px;font-size:13px}
</style>
<div id="gwBar">⚠️ พีซีไม่ได้รับข้อมูลจากเครื่อง Host-Gateway — ตัวเลขด้านล่างเป็นข้อมูลเก่า
 การเตือนหน้างานที่เครื่อง Host ยังทำงานตามปกติ</div>
<select id="gwHostSel" title="เลือกเครื่อง Host"></select>
<script>
(function(){
  var KEY='iv-host';
  function cur(){ try{ return new URLSearchParams(location.search).get('host') || localStorage.getItem(KEY) || ''; }catch(e){ return ''; } }
  var _fetch = window.fetch.bind(window);
  window.fetch = function(u, o){
    if (typeof u === 'string' && u.charAt(0) === '/') {
      var h = cur();
      if (h) u += (u.indexOf('?') < 0 ? '?' : '&') + 'host=' + encodeURIComponent(h);
    }
    return _fetch(u, o).then(function(r){
      if (typeof u === 'string' && u.indexOf('/api/data') === 0) {
        r.clone().json().then(function(d){
          document.getElementById('gwBar').style.display = (d.hostOnline === false) ? 'block' : 'none';
        }).catch(function(){});
      }
      return r;
    });
  };
  _fetch('/api/hosts').then(function(r){ return r.json(); }).then(function(d){
    var sel = document.getElementById('gwHostSel');
    if (!d.hosts || d.hosts.length < 2) return;
    d.hosts.forEach(function(h){
      var o = document.createElement('option'); o.value = h.id;
      o.textContent = 'Host ' + h.id + (h.online ? '' : ' (หลุด)'); sel.appendChild(o);
    });
    sel.value = cur() || d.hosts[0].id; sel.style.display = 'block';
    sel.onchange = function(){ try{ localStorage.setItem(KEY, sel.value); }catch(e){} location.reload(); };
  }).catch(function(){});
})();
// แทนหน้าต่าง "จุดเชื่อมต่อ" เดิม ซึ่งพูดถึง AP/Wi-Fi ของตัวเครื่อง Host ที่ไม่เกี่ยวกับรุ่นนี้
function fetchApStatus(){
  fetch('/api/ap/status').then(function(r){return r.json();}).then(function(d){
    document.getElementById('apDetailBox').innerHTML =
      '<b>เครื่อง Host-Gateway:</b> ' + d.gatewayId + '<br>' +
      '<b>สถานะลิงก์มาพีซี:</b> ' + (d.uplinkOk ? '🟢 ปกติ' : '🔴 ขาด (' + d.ageS + ' วินาทีที่แล้ว)') + '<br>' +
      '<b>ที่อยู่ของ Host:</b> ' + d.ip + '<br>' +
      '<b>สัญญาณเราเตอร์ที่ Host:</b> ' + d.staRssi + ' dBm<br>' +
      '<b>ช่องสัญญาณ ESP-NOW:</b> ' + d.channel + '<br>' +
      '<b>เวลาเครื่องพีซี:</b> ' + d.currentTime;
    document.getElementById('apHintBox').innerHTML =
      'รุ่นนี้ <b>เครื่อง Host ไม่เปิดหน้าเว็บเอง</b> ให้เปิดหน้านี้ที่พีซีเท่านั้น: <b>' + location.origin + '</b><br>' +
      'ถ้าพีซีหรือเราเตอร์ล่ม การเตือนหน้างานที่เครื่อง Host ยังทำงานตามปกติ';
  }).catch(function(){ document.getElementById('apDetailBox').innerText = 'อ่านข้อมูลไม่สำเร็จ'; });
}
</script>
"""

# ข้อความจุดที่ต้องแก้ในหน้าเดิม — ถ้าหน้าเดิมเปลี่ยนจนหาไม่เจอ ต้องล้มทันทีตอนเริ่ม
# ดีกว่าเสิร์ฟหน้าที่ครึ่ง ๆ กลาง ๆ
PATCHES = [
    ("📶 จุดเชื่อมต่อ</button>", "🖥️ สถานะ Host</button>"),
    ("📶 จุดเชื่อมต่อของเครื่อง Host</h3>", "🖥️ สถานะ Host-Gateway</h3>"),
    ("`AP: ${data.netIP} · CH ${data.channel} · อุปกรณ์ ${data.apClients}/${data.apMaxClients} เครื่อง`",
     "`พีซี ← Host ${data.gatewayId} · CH ${data.channel} · สัญญาณเราเตอร์ ${data.staRssi} dBm`"),
]
WIFI_BLOCK = re.compile(
    r'<h4 style="margin:18px 0 8px; font-size:15\.5px;">เชื่อมต่อผ่าน Wi-Fi ของโรงพยาบาล</h4>.*?'
    r'<button class="btn btn-secondary" onclick="closeApModal\(\)">ปิด</button>\s*</div>',
    re.S)
WIFI_REPLACEMENT = ('<div class="btn-group" style="margin-top:14px;">'
                    '<button class="btn btn-secondary" onclick="closeApModal()">ปิด</button></div>')


def build_dashboard(path=DASHBOARD_PATH):
    with open(path, encoding="utf-8") as f:
        html = f.read()
    for old, new in PATCHES:
        if old not in html:
            raise RuntimeError("หน้าเว็บเดิมเปลี่ยนไป หาข้อความนี้ไม่เจอ: " + old[:50])
        html = html.replace(old, new, 1)
    html, n = WIFI_BLOCK.subn(WIFI_REPLACEMENT, html, count=1)
    if n != 1:
        raise RuntimeError("หน้าเว็บเดิมเปลี่ยนไป หาบล็อกตั้งค่าเราเตอร์ไม่เจอ")
    if "</body>" not in html:
        raise RuntimeError("ไม่พบแท็ก </body>")
    return html.replace("</body>", PATCH_SCRIPT + "</body>", 1)


# ============================================================================
# HTTP
# ============================================================================
class Handler(BaseHTTPRequestHandler):
    server_version = "IVPC/" + SERVER_VERSION
    protocol_version = "HTTP/1.1"          # รองรับ keep-alive ที่ Host ใช้ต่อซ้ำ

    gw = None          # type: Gateway
    page = ""
    auth = None        # "user:pass" หรือ None
    quiet = False

    def log_message(self, fmt, *args):
        if not self.quiet:
            sys.stderr.write("%s %s\n" % (self.address_string(), fmt % args))

    # ---- ตัวช่วย ----
    def _send(self, code, body, ctype="application/json; charset=utf-8", headers=None):
        if isinstance(body, (dict, list)):
            body = json.dumps(body, ensure_ascii=False)
        if isinstance(body, str):
            body = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        for k, v in (headers or {}).items():
            self.send_header(k, v)
        self.end_headers()
        self.wfile.write(body)

    def _authed(self):
        if not self.auth:
            return True
        h = self.headers.get("Authorization", "")
        if h.startswith("Basic "):
            try:
                if base64.b64decode(h[6:]).decode("utf-8") == self.auth:
                    return True
            except Exception:
                pass
        self._send(401, "Unauthorized", "text/plain",
                   {"WWW-Authenticate": 'Basic realm="Smart IV Alert"'})
        return False

    def _form(self):
        n = int(self.headers.get("Content-Length", 0) or 0)
        raw = self.rfile.read(min(n, 65536)).decode("utf-8", "replace") if n else ""
        form = {k: v[0] for k, v in parse_qs(raw, keep_blank_values=True).items()}
        return form

    def _station(self, q):
        try:
            st = int(q.get("station", ["0"])[0])
        except ValueError:
            return None
        return st if 1 <= st <= MAX_STATIONS else None

    # ---- GET ----
    def do_GET(self):
        u = urlparse(self.path)
        q = parse_qs(u.query)
        host = (q.get("host", [""])[0]) or None
        if u.path == "/healthz":
            return self._send(200, {"ok": True, "version": SERVER_VERSION})
        if not self._authed():
            return
        gw = self.gw
        if u.path in ("/", "/index.html"):
            return self._send(200, self.page, "text/html; charset=utf-8")
        if u.path == "/api/data":
            return self._send(200, gw.api_data(host))
        if u.path == "/api/hosts":
            now = gw.clock()
            with gw.lock:
                hosts = [{"id": h.id, "online": h.online(now)} for h in sorted(
                    gw.hosts.values(), key=lambda x: x.id)]
            return self._send(200, {"hosts": hosts})
        if u.path == "/api/logs":
            st = self._station(q) or 1
            return self._send(200, gw.api_logs(host, st))
        if u.path == "/api/logs/csv":
            st = self._station(q) or 1
            return self._send(200, gw.csv_for(host, st), "text/csv; charset=utf-8", {
                "Content-Disposition": "attachment; filename=IV_Report_Bed_%d.csv" % st})
        if u.path == "/api/ap/status":
            d = gw.api_data(host)
            now = gw.clock()
            return self._send(200, {
                "gatewayId": d.get("gatewayId", "-"), "uplinkOk": d.get("hostOnline", False),
                "ageS": d.get("gatewayAgeS", 0), "ip": d.get("netIP", "-"),
                "staRssi": d.get("staRssi", 0), "channel": d.get("channel", 0),
                "currentTime": local_stamp(now)})
        self._send(404, "Not found", "text/plain")

    # ---- POST ----
    def do_POST(self):
        u = urlparse(self.path)
        q = parse_qs(u.query)
        host = (q.get("host", [""])[0]) or None
        gw = self.gw

        if u.path == "/api/ingest":
            if gw.token and self.headers.get("X-Gateway-Token", "") != gw.token:
                return self._send(403, "Bad token", "text/plain")
            n = int(self.headers.get("Content-Length", 0) or 0)
            if n <= 0 or n > 65536:
                return self._send(400, "Bad length", "text/plain")
            try:
                data = json.loads(self.rfile.read(n).decode("utf-8"))
                body = gw.ingest(data)
            except (ValueError, TypeError, KeyError) as e:
                return self._send(400, "Bad payload: %s" % e, "text/plain")
            return self._send(200, body, "text/plain; charset=utf-8")

        if not self._authed():
            return
        h = gw.pick(host)
        if u.path == "/api/time/set":                 # พีซีเป็นเจ้าของเวลา ไม่รับเวลาจากเบราว์เซอร์
            return self._send(200, local_stamp(gw.clock()), "text/plain")
        if h is None:
            return self._send(503, "No host connected", "text/plain")

        if u.path in ("/api/stations/config", "/api/stations/reset", "/api/stations/ack"):
            form = self._form()
            st = self._station({"station": [form.get("station", q.get("station", ["0"])[0])]})
            if st is None:
                return self._send(400, "Invalid station", "text/plain")
            if u.path.endswith("config"):
                kv, err = validate_config(form)
                if err:
                    return self._send(400, err, "text/plain")
                gw.enqueue(h.id, "C", st, kv)
            else:
                gw.enqueue(h.id, "R" if u.path.endswith("reset") else "A", st)
            return self._send(200, {"ok": True})
        if u.path == "/api/stations/set":
            try:
                n = int(q.get("count", ["0"])[0])
            except ValueError:
                n = 0
            if not 1 <= n <= MAX_STATIONS:
                return self._send(400, "Invalid count", "text/plain")
            gw.enqueue(h.id, "N", 0, {"count": n})
            return self._send(200, "OK", "text/plain")
        self._send(404, "Not found", "text/plain")


def make_server(addr, gateway, page, auth=None, quiet=False):
    handler = type("H", (Handler,), {"gw": gateway, "page": page, "auth": auth, "quiet": quiet})
    srv = ThreadingHTTPServer(addr, handler)
    srv.daemon_threads = True
    return srv


def lan_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("10.255.255.255", 1))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except OSError:
        return "127.0.0.1"


def main():
    ap = argparse.ArgumentParser(description="Smart IV Alert — PC Dashboard")
    ap.add_argument("--bind", default="0.0.0.0")
    ap.add_argument("--port", type=int, default=8080)
    ap.add_argument("--db", default=os.path.join(HERE, "iv_history.sqlite"))
    ap.add_argument("--token", default=os.environ.get("IV_GATEWAY_TOKEN", ""),
                    help="รหัสที่ Host-Gateway ต้องส่งมาใน X-Gateway-Token (ว่าง = ไม่ตรวจ)")
    ap.add_argument("--auth", default=os.environ.get("IV_DASH_AUTH", ""),
                    help="user:password ของหน้าเว็บ (ว่าง = ไม่ตรวจ)")
    args = ap.parse_args()

    gw = Gateway(History(args.db), token=args.token)
    page = build_dashboard()
    srv = make_server((args.bind, args.port), gw, page, auth=args.auth or None)

    def ticker():
        while True:
            time.sleep(1)
            try:
                gw.tick()
            except Exception as e:           # ห้ามให้เธรดนี้ตาย ไม่งั้นบันทึกรายนาทีหยุดเงียบ ๆ
                sys.stderr.write("tick error: %r\n" % (e,))
    threading.Thread(target=ticker, daemon=True).start()

    print("Smart IV Alert PC Dashboard %s" % SERVER_VERSION)
    print("  เปิดหน้าเว็บ : http://%s:%d" % (lan_ip(), args.port))
    print("  ตั้งใน Host  : PC = %s  พอร์ต %d" % (lan_ip(), args.port))
    if not args.auth:
        print("  คำเตือน: ยังไม่ตั้ง --auth หน้าเว็บเปิดให้ทุกเครื่องในเครือข่ายเข้าได้")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
