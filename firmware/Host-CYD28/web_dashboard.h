/**
 * @file      web_dashboard.h
 * @brief     หน้าเว็บแดชบอร์ด (หน้าเดียว) เก็บใน flash
 * @version   2.3.1-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * หน้าเดียว ไม่พึ่งไลบรารีภายนอก (Host ไม่มีอินเทอร์เน็ต) ดึง /api/state ทุก 2 วินาที แสดงการ์ดเตียง แก้ค่าตั้ง เริ่มถุงใหม่ (ถามยืนยัน) รับทราบใกล้หมด และประวัติเหตุการณ์ เปิดหน้าครั้งแรกจะส่งเวลาของเครื่องที่เปิดไปตั้งนาฬิกา Host ถ้ายังไม่ได้ตั้ง ข้อความทุกตัวใส่ผ่าน textContent เพื่อกัน HTML แทรก การสั่งแก้ค่าต้องล็อกอิน (เบราว์เซอร์จะถามรหัสเอง)
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.3.1-CYD | 2026-11-27 | ผ่านตัวตรวจ W3C Nu Html Checker (HTML+CSS ทั้งหน้าจริงและ DOM หลัง JavaScript วาด) และ axe-core WCAG 2.1 AA ทั้งธีมมืด/สว่าง: ผูก label กับช่องกรอกทุกช่อง (for/id) · landmark ครบ (section มีหัวข้อ, footer) · สีตัวอักษรสถานะและปุ่มลบผ่านอัตราส่วน 4.5:1 · กราฟมี role=img และคำอธิบาย · ตารางมี scope · toast เป็น role=status |
 * | 2.3.0-CYD | 2026-11-27 | ปุ่มพิมพ์รายงานในการ์ดเตียง: สร้างใบรายงาน A4 (หัวเรื่อง เตียง/ผู้ป่วย กราฟ สรุป เหตุการณ์ของเตียง ช่องลงนาม) ฝั่งเบราว์เซอร์ด้วย window.print() ใช้พิมพ์หรือบันทึกเป็น PDF ได้ ไม่เพิ่มเส้นทาง API |
 * | 2.2.0-CYD | 2026-11-27 | กราฟแนวโน้มอัตราไหล (SVG วาดด้วย createElementNS ไม่ใช้ innerHTML) เลือกช่วง 30/60/240 นาที พร้อมรายงานสรุป ปุ่มดาวน์โหลด CSV รายนาที และแผงไฟล์บนการ์ด SD |
 * | 2.1.0-CYD | 2026-11-27 | ช่องชื่อ/ป้ายผู้ป่วยในการ์ดเตียง · แผงตั้งค่า Wi-Fi/รหัสผ่านเว็บ (เตือนถ้ายังใช้รหัสปริยาย) · แปลข้อความผิดพลาดจากเครื่องเป็นภาษาไทย |
 * | 2.0.3-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 4 (เว็บ นาฬิกา SD ประวัติเหตุการณ์) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <Arduino.h>   // PROGMEM

static const char DASHBOARD_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="th"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart IV Alert</title>
<style>
:root{--bg:#10141a;--card:#1b222c;--line:#2d3744;--fg:#e8edf2;--dim:#8b98a8;--ok:#2fbf71;--warn:#f0b429;--bad:#f87171;--idle:#9aa8b8;--pause:#38bdf8;--link:#7fb0ff}
@media(prefers-color-scheme:light){:root{--bg:#f3f5f8;--card:#fff;--line:#d5dbe3;--fg:#17202b;--dim:#5d6b7c;--ok:#12703d;--warn:#8a5600;--bad:#c62828;--idle:#4a5a6c;--pause:#0b6a99;--link:#1a4fb8}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.4 system-ui,sans-serif}
header{display:flex;flex-wrap:wrap;gap:8px 16px;align-items:center;justify-content:space-between;padding:12px 16px;border-bottom:1px solid var(--line)}
h1{font-size:18px;margin:0}.meta{color:var(--dim);font-size:13px}
main{padding:16px;display:grid;gap:16px;grid-template-columns:repeat(auto-fill,minmax(300px,1fr))}
.card{background:var(--card);border:2px solid var(--line);border-radius:10px;padding:14px}
.card.ok{border-color:var(--ok)}.card.warn{border-color:var(--warn)}.card.bad{border-color:var(--bad)}.card.pause{border-color:var(--pause)}
.top{display:flex;justify-content:space-between;align-items:baseline}.bed{font-weight:700;font-size:17px}
.state{font-weight:700;font-size:14px}.ok .state{color:var(--ok)}.warn .state{color:var(--warn)}.bad .state{color:var(--bad)}.pause .state{color:var(--pause)}.idle .state{color:var(--idle)}
.rate{font-size:40px;font-weight:700;margin:6px 0}.rate small{font-size:14px;color:var(--dim);font-weight:400}
.bar{height:10px;background:var(--line);border-radius:5px;overflow:hidden;margin:6px 0}.bar i{display:block;height:100%;background:var(--ok)}
.warn .bar i{background:var(--warn)}.bad .bar i{background:var(--bad)}.pause .bar i{background:var(--pause)}.idle .bar i{background:var(--idle)}
.kv{display:grid;grid-template-columns:1fr 1fr;gap:2px 12px;color:var(--dim);font-size:13px}.kv b{color:var(--fg);font-weight:600}
form{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:12px}label{font-size:12px;color:var(--dim);display:block}
input,select{width:100%;padding:8px;border:1px solid var(--line);border-radius:6px;background:var(--bg);color:var(--fg);font-size:15px}
.row{grid-column:1/-1;display:flex;gap:8px;flex-wrap:wrap}button{padding:9px 14px;border:0;border-radius:6px;background:#2f6feb;color:#fff;font-size:14px;cursor:pointer}
button.alt{background:var(--line);color:var(--fg)}button.danger{background:#c62828}button:disabled{opacity:.4;cursor:default}
details.tr{margin-top:12px;border-top:1px solid var(--line);padding-top:8px}details.tr summary{cursor:pointer;color:var(--dim);font-size:13px}svg.chart{width:100%;height:auto;background:var(--bg);border:1px solid var(--line);border-radius:6px;margin-top:6px}
.rep{display:grid;grid-template-columns:1fr 1fr;gap:2px 12px;font-size:13px;color:var(--dim);margin-top:6px}.rep b{color:var(--fg);font-weight:600}.legend{font-size:11px;color:var(--dim);margin-top:4px}a.dl{color:var(--link);font-size:13px}
section{padding:0 16px 24px}table{width:100%;border-collapse:collapse;font-size:13px}th,td{text-align:left;padding:6px 8px;border-bottom:1px solid var(--line)}th{color:var(--dim);font-weight:600}
#toast{position:fixed;left:50%;bottom:16px;transform:translateX(-50%);background:var(--card);border:1px solid var(--line);padding:10px 16px;border-radius:8px;display:none}
.note{color:var(--dim);font-size:12px;padding:0 16px 16px}
/* [2.3.0-CYD] เพิ่ม: ใบรายงานสำหรับพิมพ์ ซ่อนทุกอย่างยกเว้น #sheet ตอนสั่งพิมพ์ */
#sheet{display:none}
@media print{@page{size:A4;margin:14mm}body>*:not(#sheet){display:none!important}body{background:#fff}#sheet{display:block;color:#000;background:#fff;font:12pt/1.45 "TH Sarabun New","Sarabun",system-ui,sans-serif}#sheet h2{margin:0 0 4px;font-size:18pt}#sheet table{margin-top:8px}#sheet th,#sheet td{border:1px solid #777;padding:3px 6px;color:#000}#sheet svg{width:100%;height:auto;border:1px solid #777;background:#fff;margin-top:8px}#sheet .sign{display:flex;gap:40px;margin-top:48px}#sheet .sign div{flex:1;border-top:1px solid #000;padding-top:4px;text-align:center}#sheet .fine{font-size:10pt;margin-top:16px}}
</style></head><body>
<header><h1>Smart IV Alert &ndash; Host</h1><div class="meta" id="meta">กำลังเชื่อมต่อ...</div></header>
<main id="beds" aria-label="เตียงที่เฝ้าระวัง"></main>
<section aria-labelledby="hEv"><h2 id="hEv" style="font-size:16px">ประวัติเหตุการณ์</h2>
<table><thead><tr><th scope="col">เวลา</th><th scope="col">เตียง</th><th scope="col">เหตุการณ์</th><th scope="col">รายละเอียด</th></tr></thead><tbody id="ev"></tbody></table></section>
<section id="sdPanel" aria-labelledby="hSd"><h2 id="hSd" style="font-size:16px">ไฟล์บันทึกบนการ์ด SD</h2><div class="meta" id="sdInfo">กำลังโหลด...</div><div id="sdLinks" style="margin-top:6px;display:flex;flex-wrap:wrap;gap:6px 14px"></div>
<div class="meta">ดาวน์โหลดต้องล็อกอิน (เบราว์เซอร์จะถามชื่อผู้ใช้และรหัสผ่าน) ไฟล์รายวันเก็บทุกนาทีทุกเตียงที่มีข้อมูล วันนับตามเขตเวลาของเครื่องที่ตั้งเวลาให้ Host</div></section>
<section id="wifiPanel" aria-labelledby="hWifi"><h2 id="hWifi" style="font-size:16px">ตั้งค่า Wi-Fi และรหัสผ่าน</h2>
<div class="meta" id="wifiInfo">กำลังโหลด...</div><div class="meta" id="wifiWarn" style="color:var(--warn)"></div>
<form id="wifiForm" style="max-width:420px;margin-top:8px">
<div><label for="wSsid">ชื่อ Wi-Fi ของ Host (1-31 ตัว: A-Z a-z 0-9 ช่องว่าง _ . -)</label><input id="wSsid" autocomplete="off" maxlength="31"></div>
<div><label for="wPass">รหัส Wi-Fi (8-63 ตัวอักษร ASCII)</label><input id="wPass" type="password" autocomplete="new-password" maxlength="63"></div>
<div><label for="wUser">ชื่อผู้ใช้เว็บ (3-16 ตัว: A-Z a-z 0-9 _ . -)</label><input id="wUser" autocomplete="off" maxlength="16"></div>
<div><label for="wWeb">รหัสผ่านเว็บ (6-32 ตัว ไม่มีช่องว่าง)</label><input id="wWeb" type="password" autocomplete="new-password" maxlength="32"></div>
<div class="row"><button type="button" id="wSave">บันทึก (เครื่องจะรีสตาร์ท)</button></div>
<div class="meta">เว้นช่องที่ไม่ต้องการเปลี่ยนไว้ว่าง ค่าใหม่มีผลหลังรีสตาร์ท ต้องต่อ Wi-Fi ใหม่และล็อกอินใหม่ ลืมรหัส: กดปุ่ม BOOT บนบอร์ดค้างไว้ตอนเปิดเครื่อง 2 วินาทีเพื่อคืนค่าปริยาย</div>
</form></section>
<footer class="note">ดูข้อมูลได้ทันที การแก้ค่า/เริ่มถุงใหม่/รับทราบ ต้องล็อกอิน (เบราว์เซอร์จะถามชื่อผู้ใช้และรหัสผ่าน) ระบบนี้ไม่ใช่ระบบเตือนที่ได้รับการรับรองทางการแพทย์</footer>
<div id="sheet"></div>
<div id="toast" role="status" aria-live="polite"></div>
<script>
"use strict";
var STATE_CLASS={"OK":"ok","TOO FAST":"bad","TOO SLOW":"bad","NO FLOW":"bad","DONE":"bad","NO DROP":"bad","LOST":"bad","NEAR END":"warn","PAUSED":"pause","OFFLINE":"idle","NO DATA":"idle"};
var editing=null, timeSent=false, cards={}, openTrends=[], SVGNS="http://www.w3.org/2000/svg";
function $(id){return document.getElementById(id)}
function el(tag,cls,text){var e=document.createElement(tag);if(cls)e.className=cls;if(text!==undefined)e.textContent=text;return e}
function toast(m){var t=$("toast");t.textContent=m;t.style.display="block";clearTimeout(toast.h);toast.h=setTimeout(function(){t.style.display="none"},3500)}
function fmtLeft(m){if(m<0)return"--";return m>=60?Math.floor(m/60)+" ชม. "+(m%60)+" นาที":m+" นาที"}
function fmtTime(e,up){if(e>0){var d=new Date(e*1000);return d.toLocaleString("th-TH")}return"เปิดเครื่อง +"+up+" วิ"}
var ERRTXT={"bad value":"ค่าที่กรอกไม่ถูกต้อง (ตรวจช่วงตัวเลข ความยาว หรืออักขระที่ไม่อนุญาต)","bad bed":"เลขเตียงไม่ถูกต้อง","no fields":"ไม่มีค่าที่จะบันทึก","confirm required":"ต้องยืนยันก่อน","nothing to acknowledge":"ไม่มีอะไรให้รับทราบ","bad time":"เวลาไม่ถูกต้อง"};
function svgEl(tag,attrs){var e=document.createElementNS(SVGNS,tag);for(var k in attrs)e.setAttribute(k,attrs[k]);return e}
var CRIT={1:1,2:1,4:1,5:1,6:1,7:1};
function drawChart(svg,d){
  while(svg.firstChild)svg.removeChild(svg.firstChild);
  var W=300,H=120,n=d.r.length,max=10,i;
  for(i=0;i<n;i++){if(d.r[i]>max)max=d.r[i];if(d.t[i]>max)max=d.t[i]}
  max*=1.15;
  function X(k){return n>1?2+k*(W-4)/(n-1):W/2}
  function Y(v){return H-4-(v/max)*(H-8)}
  for(i=1;i<=3;i++)svg.appendChild(svgEl("line",{x1:0,x2:W,y1:H*i/4,y2:H*i/4,stroke:"#8886","stroke-width":0.5}));
  if(!n){var t=svgEl("text",{x:W/2,y:H/2,"text-anchor":"middle","font-size":9,fill:"#889"});t.textContent="ยังไม่มีข้อมูล (บันทึกนาทีละ 1 ตัวอย่าง)";svg.appendChild(t);return}
  for(i=0;i<n;i++){
    var valid=(d.f[i]&1)!==0;
    if(!valid){svg.appendChild(svgEl("line",{x1:X(i),x2:X(i),y1:H-6,y2:H-1,stroke:"#999","stroke-width":1.5}));continue}
    if(CRIT[d.a[i]])svg.appendChild(svgEl("line",{x1:X(i),x2:X(i),y1:2,y2:H-2,stroke:"#ef4444","stroke-width":1.5,opacity:0.6}));
    else if(d.a[i]===3)svg.appendChild(svgEl("line",{x1:X(i),x2:X(i),y1:H-12,y2:H-2,stroke:"#f0b429","stroke-width":1.5}));
  }
  function poly(vals,valid,color,w){var pts=[];function flush(){if(pts.length>1)svg.appendChild(svgEl("polyline",{points:pts.join(" "),fill:"none",stroke:color,"stroke-width":w}));else if(pts.length===1){var p=pts[0].split(",");svg.appendChild(svgEl("circle",{cx:p[0],cy:p[1],r:w,fill:color}))}pts=[]}
    for(var k=0;k<n;k++){if(!valid(k)){flush();continue}pts.push(X(k).toFixed(1)+","+Y(vals[k]).toFixed(1))}flush()}
  poly(d.t,function(k){return(d.f[k]&1)&&d.t[k]>0},"#38bdf8",1);
  poly(d.r,function(k){return(d.f[k]&1)!==0},"#2fbf71",1.6);
  var lab=svgEl("text",{x:3,y:9,"font-size":8,fill:"#889"});lab.textContent=Math.round(max)+" mL/h";svg.appendChild(lab);
}
function reportRows(s){
  var al=Object.keys(s.alertMinutes||{}).map(function(k){return k+" "+s.alertMinutes[k]+" นาที"}).join(", ")||"ไม่มี";
  return [["ช่วงที่สรุป",s.minutes+" นาที"],["อัตราไหลเฉลี่ย",s.avgRate+" mL/h"],["ต่ำสุด / สูงสุด",s.minRate+" / "+s.maxRate+" mL/h"],["เป้าหมายเฉลี่ย",s.avgTarget+" mL/h"],["ปริมาตรที่ให้",s.totalMl+" mL"],["คลาดจากเป้าเฉลี่ย",s.devPct+" %"],["อยู่ใน ±10% ของเป้า",s.withinPct+" % ของเวลา"],["ครั้งที่เกิดเตือนวิกฤต",s.episodes],["เวลาที่เตือน",al],["ไม่มีข้อมูล (Station เงียบ)",s.gap+" นาที"]];
}
function showReport(box,s){
  box.textContent="";
  reportRows(s).forEach(function(p){box.appendChild(el("span",null,p[0]));box.appendChild(el("b",null,String(p[1])))});
}
/* [2.3.0-CYD] เพิ่ม: ใบรายงานพิมพ์ได้ ดึงข้อมูลชุดเดียวกับกราฟ แล้วสั่ง window.print() */
function printReport(id,name,mins){
  Promise.all(["/api/trend?bed="+id+"&mins="+mins,"/api/report?bed="+id+"&mins="+mins,"/api/events"].map(function(u){return fetch(u).then(function(r){if(!r.ok)throw new Error("ดึงข้อมูลไม่ได้");return r.json()})})).then(function(a){
    var sh=$("sheet"),tb,tr;sh.textContent="";
    sh.appendChild(el("h2",null,"รายงานการให้สารน้ำทางหลอดเลือดดำ"));
    sh.appendChild(el("div",null,"เตียง "+id+(name?" · ผู้ป่วย "+name:"")));
    sh.appendChild(el("div",null,"ข้อมูล "+mins+" นาทีล่าสุด · พิมพ์เมื่อ "+new Date().toLocaleString("th-TH")+" (ตามนาฬิกาเครื่องที่สั่งพิมพ์)"));
    var svg=document.createElementNS(SVGNS,"svg");svg.setAttribute("viewBox","0 0 300 120");svg.setAttribute("role","img");svg.setAttribute("aria-label","กราฟอัตราไหลของเตียง "+id);drawChart(svg,a[0]);sh.appendChild(svg);
    sh.appendChild(el("div","fine","เขียว=อัตราไหล ฟ้า=เป้าหมาย แดง=เหตุเตือนวิกฤต เหลือง=ใกล้หมด เทา=ไม่มีข้อมูล (ไม่ใช่ศูนย์)"));
    tb=el("table");reportRows(a[1]).forEach(function(p){tr=el("tr");tr.appendChild(el("th",null,p[0]));tr.appendChild(el("td",null,String(p[1])));tb.appendChild(tr)});sh.appendChild(tb);
    var ev=a[2].filter(function(e){return Number(e.bed)===Number(id)}).slice(0,20);
    sh.appendChild(el("h3",null,"เหตุการณ์ของเตียงนี้ (ล่าสุดไม่เกิน 20 รายการ)"));
    tb=el("table");tr=el("tr");["เวลา","เหตุการณ์","รายละเอียด"].forEach(function(t){tr.appendChild(el("th",null,t))});tb.appendChild(tr);
    ev.forEach(function(e){tr=el("tr");[fmtTime(e.epoch,e.up),e.event,e.detail].forEach(function(t){tr.appendChild(el("td",null,t))});tb.appendChild(tr)});
    if(!ev.length){tr=el("tr");var td=el("td",null,"ไม่มีเหตุการณ์");td.colSpan=3;tr.appendChild(td);tb.appendChild(tr)}
    sh.appendChild(tb);
    var sg=el("div","sign");["ผู้บันทึก","ผู้ตรวจสอบ"].forEach(function(t){sg.appendChild(el("div",null,t))});sh.appendChild(sg);
    sh.appendChild(el("div","fine","ข้อมูลจากอุปกรณ์เสริมเฝ้าระวัง ไม่ใช่ระบบเตือนที่ได้รับการรับรองทางการแพทย์ ต้องตรวจสอบสารน้ำและผู้ป่วยโดยตรงเสมอ"));
    window.print();
  }).catch(function(e){toast(e.message)});
}
function loadSd(){
  fetch("/api/sd").then(function(r){return r.json()}).then(function(s){
    $("sdInfo").textContent=s.ok?"การ์ด SD พร้อมใช้งาน":"ไม่พบการ์ด SD (เสียบการ์ด FAT32 แล้วรีสตาร์ท Host)";
    var box=$("sdLinks");box.textContent="";
    if(!s.ok)return;
    function link(t,u){var a=el("a","dl",t);a.href=u;box.appendChild(a)}
    link("ประวัติเหตุการณ์ (events.csv)","/api/sdfile?file=events");
    s.days.forEach(function(d){link("แนวโน้ม "+(d==="unsync"?"(ยังไม่ได้ตั้งเวลา)":d.substr(0,4)+"-"+d.substr(4,2)+"-"+d.substr(6,2)),"/api/sdfile?file=trend&day="+d)});
  }).catch(function(){$("sdInfo").textContent="อ่านสถานะ SD ไม่ได้"});
}
function post(url,data){
  var body=new URLSearchParams(data).toString();
  return fetch(url,{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:body,credentials:"same-origin"}).then(function(r){
    return r.text().then(function(t){if(!r.ok)throw new Error(r.status===401?"ต้องล็อกอิน":ERRTXT[t]||t||("ผิดพลาด "+r.status));return t})})
}
function buildCard(b){
  var c=el("div","card idle"),top=el("div","top"),bed=el("span","bed","เตียง "+b.id),st=el("span","state");
  top.appendChild(bed);top.appendChild(st);c.appendChild(top);
  var rate=el("div","rate"),rv=el("span"),ru=el("small"," mL/h");rate.appendChild(rv);rate.appendChild(ru);c.appendChild(rate);
  var bar=el("div","bar"),fill=el("i");bar.appendChild(fill);c.appendChild(bar);
  var kv=el("div","kv");c.appendChild(kv);
  var f=el("form");
  function field(name,label,type,attrs){var d=el("div"),l=el("label",null,label),i=el(type==="select"?"select":"input");i.name=name;i.id="f"+b.id+"_"+name;l.htmlFor=i.id;
    if(type==="select"){[10,15,20,60].forEach(function(v){var o=el("option",null,v+" gtt/mL");o.value=v;i.appendChild(o)})}else{i.type=type;if(type==="number")i.step="any";for(var k in attrs)i.setAttribute(k,attrs[k])}
    d.appendChild(l);d.appendChild(i);f.appendChild(d);return i}
  var iName=field("name","ชื่อ/ป้ายผู้ป่วย (ไม่เกิน 63 ไบต์ ไทยได้ประมาณ 21 ตัว)","text",{maxlength:63,autocomplete:"off"});iName.parentNode.style.gridColumn="1/-1";
  var iRate=field("rate","อัตราเป้าหมาย (mL/h)","number",{min:0,max:3000}),iPlan=field("plan","ปริมาตรถุง (mL)","number",{min:0,max:10000}),
      iDf=field("df","Drop factor","select"),iNear=field("near","เตือนใกล้หมดที่ (%)","number",{min:50,max:95,step:1});
  var det=el("details","tr"),dsum=el("summary",null,"กราฟแนวโน้มและรายงาน"),rangeSel=el("select"),chart=document.createElementNS(SVGNS,"svg"),legend=el("div","legend","เขียว=อัตราไหล ฟ้า=เป้าหมาย แดง=เหตุเตือนวิกฤต เหลือง=ใกล้หมด เทา=ไม่มีข้อมูล (ไม่ใช่ศูนย์)"),rep=el("div","rep"),dl=el("a","dl","ดาวน์โหลด CSV รายนาที (ล็อกอิน)"),prn=el("button","alt","พิมพ์รายงาน / บันทึกเป็น PDF");prn.type="button";prn.style.margin="6px 8px 0 0";
  [[30,"30 นาที"],[60,"1 ชั่วโมง"],[240,"4 ชั่วโมง"]].forEach(function(o){var op=el("option",null,o[1]);op.value=o[0];rangeSel.appendChild(op)});rangeSel.value=60;rangeSel.style.margin="6px 0 0";
  chart.setAttribute("viewBox","0 0 300 120");chart.setAttribute("class","chart");chart.setAttribute("role","img");chart.setAttribute("aria-label","กราฟอัตราไหลของเตียง "+b.id+" (สรุปตัวเลขอยู่ในรายงานด้านล่าง)");rangeSel.setAttribute("aria-label","ช่วงเวลาของกราฟ เตียง "+b.id);
  det.appendChild(dsum);det.appendChild(rangeSel);det.appendChild(chart);det.appendChild(legend);det.appendChild(rep);det.appendChild(prn);det.appendChild(dl);
  function loadTrend(){
    var m=rangeSel.value;dl.href="/api/trend.csv?bed="+b.id+"&mins=240";
    fetch("/api/trend?bed="+b.id+"&mins="+m).then(function(r){return r.json()}).then(function(d){drawChart(chart,d)}).catch(function(){});
    fetch("/api/report?bed="+b.id+"&mins="+m).then(function(r){return r.json()}).then(function(s){showReport(rep,s)}).catch(function(){});
  }
  prn.onclick=function(){printReport(b.id,obj.curName,rangeSel.value)};
  det.addEventListener("toggle",function(){if(det.open)loadTrend()});rangeSel.onchange=loadTrend;
  openTrends.push(function(){if(det.open)loadTrend()});
  var row=el("div","row"),save=el("button",null,"บันทึก"),bag=el("button","danger","เริ่มถุงใหม่"),ack=el("button","alt","รับทราบใกล้หมด");
  save.type="button";bag.type="button";ack.type="button";row.appendChild(save);row.appendChild(bag);row.appendChild(ack);f.appendChild(row);c.appendChild(f);c.appendChild(det);
  [iName,iRate,iPlan,iDf,iNear].forEach(function(i){i.addEventListener("focus",function(){editing=b.id});i.addEventListener("blur",function(){if(editing===b.id)editing=null})});
  var obj;
  save.onclick=function(){post("/api/bed",{bed:b.id,rate:iRate.value,plan:iPlan.value,df:iDf.value,near:iNear.value}).then(function(){if(iName.value!==obj.curName)return post("/api/name",{bed:b.id,name:iName.value})}).then(function(){toast("บันทึกเตียง "+b.id+" แล้ว");editing=null;poll()}).catch(function(e){toast(e.message)})};
  bag.onclick=function(){if(confirm("เริ่มถุงใหม่ที่เตียง "+b.id+"?\nตัวนับหยดที่ Station จะถูกล้าง"))post("/api/newbag",{bed:b.id,confirm:1}).then(function(){toast("เริ่มถุงใหม่เตียง "+b.id);poll()}).catch(function(e){toast(e.message)})};
  ack.onclick=function(){post("/api/ack",{bed:b.id}).then(function(){toast("รับทราบเตียง "+b.id);poll()}).catch(function(e){toast(e.message)})};
  obj={root:c,bed:bed,st:st,rv:rv,fill:fill,kv:kv,iName:iName,iRate:iRate,iPlan:iPlan,iDf:iDf,iNear:iNear,ack:ack,curName:""};
  return obj;
}
function render(s){
  var meta=(s.synced?"เวลา Host ตรงแล้ว":"ยังไม่ได้ตั้งเวลา")+" · ช่อง "+s.channel+" · SD "+(s.sd?"พร้อม":"ไม่มี")+" · v"+s.version+" · เปิดเครื่อง "+Math.floor(s.uptime/60)+" นาที"+(s.snooze>0?" · พักเสียง "+s.snooze+" วิ":"");
  $("meta").textContent=meta;
  var host=$("beds");
  s.beds.forEach(function(b){
    var c=cards[b.id];if(!c){c=cards[b.id]=buildCard(b);host.appendChild(c.root)}
    var cls=STATE_CLASS[b.state]||"idle";c.root.className="card "+cls;c.st.textContent=b.state;c.bed.textContent="เตียง "+b.id+(b.name?" · "+b.name:"");c.curName=b.name||"";c.rv.textContent=Math.round(b.rate);
    c.fill.style.width=(b.plan>0?Math.min(100,b.vol/b.plan*100):0)+"%";
    c.kv.textContent="";
    [["เป้าหมาย",b.target+" mL/h"],["ปริมาตร",b.vol+" / "+b.plan+" mL"],["เหลือ",fmtLeft(b.left)],["หยดรวม",b.drops],["ลิงก์",b.link+"% ("+b.rssi+" dBm)"],["แบตเตอรี่",b.batt>0?b.batt+" V":"-"]].forEach(function(p){
      c.kv.appendChild(el("span",null,p[0]));c.kv.appendChild(el("b",null,String(p[1])))});
    if(b.conflict){var w=el("b",null,"เลขเตียงซ้ำกับอีกบอร์ด!");w.style.color="var(--bad)";c.kv.appendChild(w)}
    c.ack.disabled=!(b.alert===3&&!b.ack);
    if(editing!==b.id){c.iName.value=b.name||"";c.iRate.value=b.target;c.iPlan.value=b.plan;c.iDf.value=b.df;c.iNear.value=b.near}
  });
  Object.keys(cards).forEach(function(id){if(id>s.active){cards[id].root.remove();delete cards[id]}});
  if(!s.synced&&!timeSent){timeSent=true;post("/api/time",{epoch:Math.floor(Date.now()/1000),tz:-new Date().getTimezoneOffset()}).catch(function(){timeSent=false})}
}
function renderEvents(list){
  var tb=$("ev");tb.textContent="";
  list.forEach(function(e){var tr=el("tr");[fmtTime(e.epoch,e.up),e.bed?("เตียง "+e.bed):"ระบบ",e.event,e.detail].forEach(function(t){tr.appendChild(el("td",null,t))});tb.appendChild(tr)});
  if(!list.length){var tr=el("tr"),td=el("td",null,"ยังไม่มีเหตุการณ์");td.colSpan=4;tr.appendChild(td);tb.appendChild(tr)}
}
function poll(){
  fetch("/api/state").then(function(r){return r.json()}).then(render).catch(function(){$("meta").textContent="ขาดการเชื่อมต่อกับ Host"});
  fetch("/api/events").then(function(r){return r.json()}).then(renderEvents).catch(function(){});
}
function loadWifi(){
  fetch("/api/wifi").then(function(r){return r.json()}).then(function(w){
    $("wifiInfo").textContent="Wi-Fi: "+w.ssid+" · ที่อยู่ "+w.ip+" · เครื่องที่เชื่อมต่อ "+w.clients+" · ผู้ใช้เว็บ: "+w.webUser;
    var warn=[];if(w.defaultWifiPass)warn.push("รหัส Wi-Fi");if(w.defaultWebPass)warn.push("รหัสผ่านเว็บ");
    $("wifiWarn").textContent=warn.length?("ยังใช้ "+warn.join(" และ ")+"ปริยายอยู่ ควรเปลี่ยนก่อนใช้งานจริง"):"";
    if(!$("wSsid").value)$("wSsid").placeholder=w.ssid;if(!$("wUser").value)$("wUser").placeholder=w.webUser;
  }).catch(function(){$("wifiInfo").textContent="อ่านค่า Wi-Fi ไม่ได้"});
}
$("wSave").onclick=function(){
  var d={ssid:$("wSsid").value,pass:$("wPass").value,webuser:$("wUser").value,webpass:$("wWeb").value};
  if(!d.ssid&&!d.pass&&!d.webuser&&!d.webpass){toast("ยังไม่ได้กรอกค่าที่จะเปลี่ยน");return}
  if(!confirm("บันทึกค่า Wi-Fi/รหัสผ่านใหม่ และรีสตาร์ท Host?\nต้องต่อ Wi-Fi ใหม่ด้วยค่าที่ตั้ง ถ้าพิมพ์ผิดจะเข้าเครื่องไม่ได้ (คืนค่าปริยายด้วยปุ่ม BOOT ตอนเปิดเครื่อง)"))return;
  post("/api/wifi",d).then(function(){toast("บันทึกแล้ว Host กำลังรีสตาร์ท ต่อ Wi-Fi ใหม่ตามค่าที่ตั้ง");["wPass","wWeb"].forEach(function(k){$(k).value=""})}).catch(function(e){toast(e.message)});
};
poll();loadWifi();loadSd();setInterval(poll,2000);setInterval(function(){openTrends.forEach(function(f){f()})},15000);
</script></body></html>)HTML";
