#!/usr/bin/env python3
"""
@file      test_server.py
@brief     ทดสอบเซิร์ฟเวอร์ PC Dashboard แบบครบวงจรผ่าน HTTP จริง (พอร์ตสุ่ม นาฬิกาจำลอง)
@version   1.0.0-PC
@date      2026-11-19
"""
import json
import os
import tempfile
import threading
import unittest
import urllib.error
import urllib.request

import server


class Clock:
    def __init__(self, t):
        self.t = t

    def __call__(self):
        return self.t


def station(i, total=0, online=True, **kw):
    d = {"id": i, "online": online, "running": True, "rssi": -60, "link": 100, "idConflict": False,
         "mac": "AA:BB", "rxTotal": 1, "battery": 3.9, "totalDrops": total, "volumeMl": total / 20.0,
         "flowRateHr": 60.0, "msSinceLastDrop": 500, "targetRate": 60.0, "planVolume": 500,
         "dropFactor": 20, "patientName": "ทดสอบ", "alertCode": 0, "nearEndPct": 80,
         "nearEndAck": False, "caseActive": True, "caseStart": ""}
    d.update(kw)
    return d


def snap(stations, ack=0, host="A1B2C3", count=None):
    return {"v": "1.0.0-GW", "host": host, "ack": ack, "count": count or len(stations),
            "bat": 4.0, "batPct": 80, "snooze": False, "rssi": -55, "channel": 6, "ip": "192.168.1.50",
            "stations": stations}


class ServerTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp()
        self.clock = Clock(1_800_000_000.0 + 5)       # ต้นนาทีพอดี + 5 วินาที
        self.gw = server.Gateway(server.History(os.path.join(self.tmp, "t.sqlite")),
                                 token="tok", clock=self.clock)
        self.srv = server.make_server(("127.0.0.1", 0), self.gw, server.build_dashboard(), quiet=True)
        self.port = self.srv.server_address[1]
        threading.Thread(target=self.srv.serve_forever, daemon=True).start()

    def tearDown(self):
        self.srv.shutdown()
        self.srv.server_close()

    def req(self, method, path, body=None, headers=None):
        url = "http://127.0.0.1:%d%s" % (self.port, path)
        data = body.encode("utf-8") if isinstance(body, str) else body
        r = urllib.request.Request(url, data=data, method=method, headers=headers or {})
        try:
            with urllib.request.urlopen(r, timeout=5) as resp:
                return resp.status, resp.read().decode("utf-8")
        except urllib.error.HTTPError as e:
            return e.code, e.read().decode("utf-8")

    def ingest(self, payload, token="tok"):
        return self.req("POST", "/api/ingest", json.dumps(payload), {"X-Gateway-Token": token})

    # ------------------------------------------------------------------
    def test_ingest_requires_token_and_returns_time(self):
        code, _ = self.ingest(snap([station(1)]), token="wrong")
        self.assertEqual(code, 403)
        code, body = self.ingest(snap([station(1)]))
        self.assertEqual(code, 200)
        self.assertTrue(body.startswith("T %d\n" % int(self.clock())))

    def test_bad_payloads_rejected(self):
        self.assertEqual(self.ingest({"host": "bad host!", "stations": []})[0], 400)
        self.assertEqual(self.ingest({"host": "X", "stations": [{}] * 9})[0], 400)
        code, _ = self.req("POST", "/api/ingest", "not json", {"X-Gateway-Token": "tok"})
        self.assertEqual(code, 400)

    def test_command_roundtrip_and_ack(self):
        self.ingest(snap([station(1)]))
        code, _ = self.req("POST", "/api/stations/config",
                           "station=1&rate=60.0&volume=500&df=15&nearpct=85&name=%E0%B8%AA%E0%B8%A1")
        self.assertEqual(code, 200)
        self.req("POST", "/api/stations/ack", "station=1")
        _, body = self.ingest(snap([station(1)], ack=0))
        lines = body.strip().split("\n")
        self.assertEqual(lines[0][0], "T")
        self.assertTrue(lines[1].startswith("C 1 1 "), lines[1])
        self.assertIn("rate=60", lines[1])
        self.assertIn("df=15", lines[1])
        self.assertIn("near=85", lines[1])
        self.assertIn("name=%E0%B8%AA%E0%B8%A1", lines[1])
        self.assertEqual(lines[2], "A 2 1")
        # Host ยังไม่รับรู้ -> ส่งซ้ำ
        _, body = self.ingest(snap([station(1)], ack=1))
        self.assertEqual(body.strip().split("\n")[1:], ["A 2 1"])
        # รับรู้ครบ -> ไม่เหลือคำสั่ง
        _, body = self.ingest(snap([station(1)], ack=2))
        self.assertEqual(body.strip().split("\n"), ["T %d" % int(self.clock())])

    def test_config_validation_matches_firmware(self):
        self.ingest(snap([station(1)]))
        bad = ["rate=1001", "rate=-1", "volume=10001", "df=25", "nearpct=49", "nearpct=96", "rate=abc"]
        for b in bad:
            code, _ = self.req("POST", "/api/stations/config", "station=1&" + b)
            self.assertEqual(code, 400, b)
        self.assertEqual(self.req("POST", "/api/stations/config", "station=9&rate=1")[0], 400)
        self.assertEqual(self.req("POST", "/api/stations/set?count=9")[0], 400)
        self.assertEqual(self.req("POST", "/api/stations/set?count=6")[0], 200)

    def test_commands_without_host_are_refused(self):
        self.assertEqual(self.req("POST", "/api/stations/ack", "station=1")[0], 503)

    def test_sequence_follows_host_ack_after_pc_restart(self):
        # Host เคยทำถึงเลข 40 แล้ว พีซีเพิ่งรีสตาร์ต (เลขเริ่มที่ 1) คำสั่งใหม่ต้องได้เลข > 40
        self.ingest(snap([station(1)], ack=40))
        self.req("POST", "/api/stations/ack", "station=1")
        _, body = self.ingest(snap([station(1)], ack=40))
        self.assertEqual(body.strip().split("\n")[1], "A 41 1")

    def test_old_commands_expire(self):
        self.ingest(snap([station(1)]))
        self.req("POST", "/api/stations/ack", "station=1")
        self.clock.t += server.CMD_TTL_S + 1
        _, body = self.ingest(snap([station(1)]))
        self.assertEqual(len(body.strip().split("\n")), 1)

    def test_minute_logs_count_drops_and_reset_starts_new_bag(self):
        self.ingest(snap([station(1, 100)]))            # ฐาน
        self.clock.t += 20
        self.ingest(snap([station(1, 160)]))            # +60
        self.clock.t += 20
        self.ingest(snap([station(1, 220)]))            # +60
        self.clock.t += 20                              # ข้ามนาที (เธรดนาฬิกาจริงเรียก tick ทุกวินาที)
        self.ingest(snap([station(1, 220)]))            # Host ยังส่งอยู่ ค่าไม่เปลี่ยน
        self.gw.tick()
        self.ingest(snap([station(1, 250)]))            # +30 เข้านาทีใหม่
        code, body = self.req("GET", "/api/logs?station=1")
        logs = json.loads(body)["logs"]
        self.assertEqual(len(logs), 1)
        self.assertEqual(logs[0]["drops"], 120)
        self.assertAlmostEqual(logs[0]["rate"], 120 / 20 * 60)
        # เริ่มถุงใหม่ -> ประวัติถุงใหม่ว่าง ตัวนับเริ่มใหม่
        self.req("POST", "/api/stations/reset", "station=1")
        _, body = self.req("GET", "/api/logs?station=1")
        self.assertEqual(json.loads(body)["logs"], [])
        self.ingest(snap([station(1, 0)]))
        self.clock.t += 60
        self.ingest(snap([station(1, 40)]))
        self.gw.tick()
        _, body = self.req("GET", "/api/logs?station=1")
        self.assertEqual(json.loads(body)["logs"][0]["drops"], 40)

    def test_counter_rollover_when_station_reboots(self):
        self.ingest(snap([station(1, 500)]))
        self.clock.t += 10
        self.ingest(snap([station(1, 30)]))             # Station รีบูต ตัวนับเริ่มจาก 0
        self.assertEqual(self.gw.hosts["A1B2C3"].acc[1]["drops"], 30)

    def test_no_rows_written_while_host_silent(self):
        self.ingest(snap([station(1, 10)]))
        self.clock.t += 120                             # Host เงียบ 2 นาที
        self.gw.tick()
        _, body = self.req("GET", "/api/logs?station=1")
        self.assertEqual(json.loads(body)["logs"], [])

    def test_stale_host_marks_beds_offline(self):
        self.ingest(snap([station(1, 10)]))
        d = json.loads(self.req("GET", "/api/data")[1])
        self.assertTrue(d["hostOnline"])
        self.assertTrue(d["stations"][0]["online"])
        self.assertEqual(d["activeCount"], 1)
        self.clock.t += server.HOST_STALE_S + 1
        d = json.loads(self.req("GET", "/api/data")[1])
        self.assertFalse(d["hostOnline"])
        self.assertFalse(d["stations"][0]["online"])
        self.assertEqual(d["stations"][0]["flowRateHr"], 0)

    def test_api_data_shape_has_every_field_the_dashboard_reads(self):
        self.ingest(snap([station(1, 10)]))
        d = json.loads(self.req("GET", "/api/data")[1])
        for k in ("version", "pollMs", "netMode", "netIP", "staRssi", "activeCount", "currentTime",
                  "timeSynced", "timeApprox", "apClients", "apMaxClients", "channel",
                  "hostBatVolts", "hostBatPct", "stations"):
            self.assertIn(k, d)

    def test_csv_matches_firmware_format(self):
        self.ingest(snap([station(1, 0)]))
        self.clock.t += 30
        self.ingest(snap([station(1, 40)]))
        self.clock.t += 40
        self.ingest(snap([station(1, 50)]))
        self.gw.tick()
        code, body = self.req("GET", "/api/logs/csv?station=1")
        self.assertEqual(code, 200)
        lines = body.lstrip("﻿").split("\r\n")
        self.assertEqual(lines[0], server.CSV_HEADER)
        self.assertEqual(len(lines[1].split(",")), 13)
        self.assertIn(",NORMAL,", lines[1])

    def test_two_hosts_are_separate(self):
        self.ingest(snap([station(1, 10)], host="AAA"))
        self.ingest(snap([station(1, 99)], host="BBB"))
        hosts = json.loads(self.req("GET", "/api/hosts")[1])["hosts"]
        self.assertEqual([h["id"] for h in hosts], ["AAA", "BBB"])
        d = json.loads(self.req("GET", "/api/data?host=BBB")[1])
        self.assertEqual(d["stations"][0]["totalDrops"], 99)
        self.req("POST", "/api/stations/ack?host=BBB", "station=1")
        _, body = self.ingest(snap([station(1, 10)], host="AAA"))
        self.assertEqual(len(body.strip().split("\n")), 1)      # คำสั่งของ BBB ไม่หลุดไป AAA

    def test_dashboard_is_patched(self):
        code, html = self.req("GET", "/")
        self.assertEqual(code, 200)
        self.assertIn("gwBar", html)
        self.assertNotIn('onclick="saveWifi()"', html)                    # ปุ่มตั้งเราเตอร์ถูกถอดแล้ว
        self.assertIn("พีซี ← Host", html)

    def test_basic_auth(self):
        self.srv.RequestHandlerClass.auth = "u:p"
        self.assertEqual(self.req("GET", "/api/data")[0], 401)
        import base64
        h = {"Authorization": "Basic " + base64.b64encode(b"u:p").decode()}
        self.assertEqual(self.req("GET", "/api/data", headers=h)[0], 200)
        self.assertEqual(self.req("GET", "/healthz")[0], 200)   # ตรวจชีพจรไม่ต้องรหัส
        # Host ส่งข้อมูลด้วย token ไม่ใช่ Basic auth
        self.assertEqual(self.ingest(snap([station(1)]))[0], 200)


if __name__ == "__main__":
    unittest.main(verbosity=2)
