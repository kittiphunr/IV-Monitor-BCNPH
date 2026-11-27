#!/usr/bin/env python3
"""สร้างหน้าเว็บสำหรับถ่ายภาพ: ใช้หน้าเว็บจริงจาก web_dashboard.h และ JSON จริงที่เฟิร์มแวร์สร้าง (ผ่าน fetch จำลอง)"""
import json, re, sys
page = open('/tmp/cyd_web_page.html', encoding='utf-8').read()
state = open('/tmp/cyd_web_state_show.json', encoding='utf-8').read()
events = open('/tmp/cyd_web_events_show.json', encoding='utf-8').read()
wifi = open('/tmp/cyd_web_wifi_show.json', encoding='utf-8').read()
json.loads(wifi)
trend = open('/tmp/cyd_web_trend_show.json', encoding='utf-8').read()
report = open('/tmp/cyd_web_report_show.json', encoding='utf-8').read()
sd = open('/tmp/cyd_web_sd_show.json', encoding='utf-8').read()
for x in (trend, report, sd): json.loads(x)
json.loads(state); json.loads(events)
stub = ("<script>window.fetch=function(u,o){var b=(u.indexOf('/api/state')===0)?%s:(u.indexOf('/api/wifi')===0)?%s:(u.indexOf('/api/trend')===0)?%s:(u.indexOf('/api/report')===0)?%s:(u.indexOf('/api/sd')===0)?%s:%s;"
        "return Promise.resolve({ok:true,status:200,json:function(){return Promise.resolve(b)},text:function(){return Promise.resolve('ok')}})};"
        "window.setInterval=function(){};window.addEventListener('load',function(){setTimeout(function(){var d=document.querySelectorAll('details');if(d[0])d[0].open=true;if(d[1])d[1].open=true},300)});</script>") % (state, wifi, trend, report, sd, events)
page = page.replace('<script>\n"use strict";', stub + '<script>\n"use strict";', 1)
assert stub in page
open(sys.argv[1], 'w', encoding='utf-8').write(page)
