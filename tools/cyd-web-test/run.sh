#!/usr/bin/env bash
# ทดสอบด่านที่ 4 ของ Host-CYD28: API เว็บ ล็อกอิน ตรวจค่า ประวัติเหตุการณ์ นาฬิกา CSV
# แล้วตรวจ JSON ที่เฟิร์มแวร์สร้างด้วย Python ตรวจไวยากรณ์ JavaScript ของหน้าเว็บด้วย Node และถ่ายภาพหน้าเว็บ
set -euo pipefail
cd "$(dirname "$0")"
SK=../../firmware/Host-CYD28
OUT="${1:-../../docs/screens-cyd2}"
python3 ../check-ino-types.py "$SK/Host-CYD28.ino"
cp "$SK/Host-CYD28.ino" sketch.cpp
FLAGS="-std=gnu++17 -I. -I../screen-preview -I$SK -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-format-truncation"
g++ $FLAGS -o /tmp/cyd_web_test test.cpp
timeout 60 /tmp/cyd_web_test

# RTC DS3231 (ไม่บังคับ): เปิดได้เมื่อไม่ชนขาบัซเซอร์ และต้องปฏิเสธเมื่อชน
g++ $FLAGS -DCYD_RTC_SDA=22 -DCYD_RTC_SCL=16 -fsyntax-only test.cpp
if g++ $FLAGS -DCYD_RTC_SDA=27 -DCYD_RTC_SCL=22 -fsyntax-only test.cpp 2>/dev/null; then echo "ล้มเหลว: ขา RTC ชนขาบัซเซอร์ต้องคอมไพล์ไม่ผ่าน"; exit 1; fi
echo "  RTC: เปิดได้เมื่อไม่ชนขา และปฏิเสธเมื่อชนขาบัซเซอร์"
rm -f sketch.cpp

python3 - <<'PY'
import json
for f in ['state_empty', 'state_worst', 'state_show']:
    d = json.load(open(f'/tmp/cyd_web_{f}.json'))
    assert isinstance(d['beds'], list) and len(d['beds']) == d['active'], f
    for b in d['beds']:
        for k in ('id','state','alert','online','running','rate','target','plan','vol','drops','df','near','left','link','rssi','batt','conflict','ack'):
            assert k in b, (f, k)
        assert isinstance(b['online'], bool) and isinstance(b['rate'], (int, float))
for f in ['events', 'events_show']:
    ev = json.load(open(f'/tmp/cyd_web_{f}.json'))
    assert isinstance(ev, list) and all({'up','epoch','bed','event','detail'} <= set(e) for e in ev), f
print('  JSON: ผ่านตัวตรวจ json.loads และมีฟิลด์ครบทุกชุด')
PY

python3 - <<'PY'
import re
s = open('/tmp/cyd_web_page.html', encoding='utf-8').read()
open('/tmp/cyd_web_page.js', 'w').write(re.search(r'<script>([\s\S]*?)</script>', s).group(1))
PY
node --check /tmp/cyd_web_page.js
echo "  JavaScript ของหน้าเว็บ: ไวยากรณ์ถูกต้อง"

CHROME=""
for c in /opt/pw-browsers/chromium /opt/pw-browsers/chromium-*/chrome-linux*/chrome /opt/pw-browsers/chromium-1194/chrome-linux/chrome "$(command -v chromium || true)" "$(command -v chromium-browser || true)"; do
  if [ -n "$c" ] && [ -f "$c" ] && [ -x "$c" ]; then CHROME="$c"; break; fi
done
if [ -n "$CHROME" ] && [ -x "$CHROME" ]; then
  mkdir -p "$OUT"
  python3 make_preview.py /tmp/cyd_web_preview.html
  "$CHROME" --headless --disable-gpu --no-sandbox --hide-scrollbars --virtual-time-budget=3000 --window-size=1200,1100 \
    --screenshot="$OUT/web_dashboard.png" "file:///tmp/cyd_web_preview.html" >/dev/null 2>&1
  echo "  ภาพ: $OUT/web_dashboard.png"
else
  echo "  (ไม่พบ Chromium จึงข้ามการถ่ายภาพหน้าเว็บ)"
fi
