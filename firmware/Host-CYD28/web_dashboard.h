/**
 * @file      web_dashboard.h
 * @brief     หน้าเว็บแดชบอร์ด (หน้าเดียว) เก็บใน flash
 * @version   2.0.3-CYD
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
:root{--bg:#10141a;--card:#1b222c;--line:#2d3744;--fg:#e8edf2;--dim:#8b98a8;--ok:#2fbf71;--warn:#f0b429;--bad:#ef4444;--idle:#5b6b7e;--pause:#38bdf8}
@media(prefers-color-scheme:light){:root{--bg:#f3f5f8;--card:#fff;--line:#d5dbe3;--fg:#17202b;--dim:#5d6b7c}}
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
button.alt{background:var(--line);color:var(--fg)}button.danger{background:var(--bad)}button:disabled{opacity:.4;cursor:default}
section{padding:0 16px 24px}table{width:100%;border-collapse:collapse;font-size:13px}th,td{text-align:left;padding:6px 8px;border-bottom:1px solid var(--line)}th{color:var(--dim);font-weight:600}
#toast{position:fixed;left:50%;bottom:16px;transform:translateX(-50%);background:var(--card);border:1px solid var(--line);padding:10px 16px;border-radius:8px;display:none}
.note{color:var(--dim);font-size:12px;padding:0 16px 16px}
</style></head><body>
<header><h1>Smart IV Alert &ndash; Host</h1><div class="meta" id="meta">กำลังเชื่อมต่อ...</div></header>
<main id="beds"></main>
<section><h2 style="font-size:16px">ประวัติเหตุการณ์</h2>
<table><thead><tr><th>เวลา</th><th>เตียง</th><th>เหตุการณ์</th><th>รายละเอียด</th></tr></thead><tbody id="ev"></tbody></table></section>
<div class="note">ดูข้อมูลได้ทันที การแก้ค่า/เริ่มถุงใหม่/รับทราบ ต้องล็อกอิน (เบราว์เซอร์จะถามชื่อผู้ใช้และรหัสผ่าน) ระบบนี้ไม่ใช่ระบบเตือนที่ได้รับการรับรองทางการแพทย์</div>
<div id="toast"></div>
<script>
"use strict";
var STATE_CLASS={"OK":"ok","TOO FAST":"bad","TOO SLOW":"bad","NO FLOW":"bad","DONE":"bad","NO DROP":"bad","LOST":"bad","NEAR END":"warn","PAUSED":"pause","OFFLINE":"idle","NO DATA":"idle"};
var editing=null, timeSent=false, cards={};
function $(id){return document.getElementById(id)}
function el(tag,cls,text){var e=document.createElement(tag);if(cls)e.className=cls;if(text!==undefined)e.textContent=text;return e}
function toast(m){var t=$("toast");t.textContent=m;t.style.display="block";clearTimeout(toast.h);toast.h=setTimeout(function(){t.style.display="none"},3500)}
function fmtLeft(m){if(m<0)return"--";return m>=60?Math.floor(m/60)+" ชม. "+(m%60)+" นาที":m+" นาที"}
function fmtTime(e,up){if(e>0){var d=new Date(e*1000);return d.toLocaleString("th-TH")}return"เปิดเครื่อง +"+up+" วิ"}
function post(url,data){
  var body=new URLSearchParams(data).toString();
  return fetch(url,{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:body,credentials:"same-origin"}).then(function(r){
    return r.text().then(function(t){if(!r.ok)throw new Error(r.status===401?"ต้องล็อกอิน":t||("ผิดพลาด "+r.status));return t})})
}
function buildCard(b){
  var c=el("div","card idle"),top=el("div","top"),bed=el("span","bed","เตียง "+b.id),st=el("span","state");
  top.appendChild(bed);top.appendChild(st);c.appendChild(top);
  var rate=el("div","rate"),rv=el("span"),ru=el("small"," mL/h");rate.appendChild(rv);rate.appendChild(ru);c.appendChild(rate);
  var bar=el("div","bar"),fill=el("i");bar.appendChild(fill);c.appendChild(bar);
  var kv=el("div","kv");c.appendChild(kv);
  var f=el("form");
  function field(name,label,type,attrs){var d=el("div"),l=el("label",null,label),i=el(type==="select"?"select":"input");i.name=name;
    if(type==="select"){[10,15,20,60].forEach(function(v){var o=el("option",null,v+" gtt/mL");o.value=v;i.appendChild(o)})}else{i.type="number";i.step="any";for(var k in attrs)i.setAttribute(k,attrs[k])}
    d.appendChild(l);d.appendChild(i);f.appendChild(d);return i}
  var iRate=field("rate","อัตราเป้าหมาย (mL/h)","number",{min:0,max:3000}),iPlan=field("plan","ปริมาตรถุง (mL)","number",{min:0,max:10000}),
      iDf=field("df","Drop factor","select"),iNear=field("near","เตือนใกล้หมดที่ (%)","number",{min:50,max:95,step:1});
  var row=el("div","row"),save=el("button",null,"บันทึก"),bag=el("button","danger","เริ่มถุงใหม่"),ack=el("button","alt","รับทราบใกล้หมด");
  save.type="button";bag.type="button";ack.type="button";row.appendChild(save);row.appendChild(bag);row.appendChild(ack);f.appendChild(row);c.appendChild(f);
  [iRate,iPlan,iDf,iNear].forEach(function(i){i.addEventListener("focus",function(){editing=b.id});i.addEventListener("blur",function(){if(editing===b.id)editing=null})});
  save.onclick=function(){post("/api/bed",{bed:b.id,rate:iRate.value,plan:iPlan.value,df:iDf.value,near:iNear.value}).then(function(){toast("บันทึกเตียง "+b.id+" แล้ว");editing=null;poll()}).catch(function(e){toast(e.message)})};
  bag.onclick=function(){if(confirm("เริ่มถุงใหม่ที่เตียง "+b.id+"?\nตัวนับหยดที่ Station จะถูกล้าง"))post("/api/newbag",{bed:b.id,confirm:1}).then(function(){toast("เริ่มถุงใหม่เตียง "+b.id);poll()}).catch(function(e){toast(e.message)})};
  ack.onclick=function(){post("/api/ack",{bed:b.id}).then(function(){toast("รับทราบเตียง "+b.id);poll()}).catch(function(e){toast(e.message)})};
  return {root:c,st:st,rv:rv,fill:fill,kv:kv,iRate:iRate,iPlan:iPlan,iDf:iDf,iNear:iNear,ack:ack};
}
function render(s){
  var meta=(s.synced?"เวลา Host ตรงแล้ว":"ยังไม่ได้ตั้งเวลา")+" · ช่อง "+s.channel+" · SD "+(s.sd?"พร้อม":"ไม่มี")+" · v"+s.version+" · เปิดเครื่อง "+Math.floor(s.uptime/60)+" นาที"+(s.snooze>0?" · พักเสียง "+s.snooze+" วิ":"");
  $("meta").textContent=meta;
  var host=$("beds");
  s.beds.forEach(function(b){
    var c=cards[b.id];if(!c){c=cards[b.id]=buildCard(b);host.appendChild(c.root)}
    var cls=STATE_CLASS[b.state]||"idle";c.root.className="card "+cls;c.st.textContent=b.state;c.rv.textContent=Math.round(b.rate);
    c.fill.style.width=(b.plan>0?Math.min(100,b.vol/b.plan*100):0)+"%";
    c.kv.textContent="";
    [["เป้าหมาย",b.target+" mL/h"],["ปริมาตร",b.vol+" / "+b.plan+" mL"],["เหลือ",fmtLeft(b.left)],["หยดรวม",b.drops],["ลิงก์",b.link+"% ("+b.rssi+" dBm)"],["แบตเตอรี่",b.batt>0?b.batt+" V":"-"]].forEach(function(p){
      c.kv.appendChild(el("span",null,p[0]));c.kv.appendChild(el("b",null,String(p[1])))});
    if(b.conflict){var w=el("b",null,"เลขเตียงซ้ำกับอีกบอร์ด!");w.style.color="var(--bad)";c.kv.appendChild(w)}
    c.ack.disabled=!(b.alert===3&&!b.ack);
    if(editing!==b.id){c.iRate.value=b.target;c.iPlan.value=b.plan;c.iDf.value=b.df;c.iNear.value=b.near}
  });
  Object.keys(cards).forEach(function(id){if(id>s.active){cards[id].root.remove();delete cards[id]}});
  if(!s.synced&&!timeSent){timeSent=true;post("/api/time",{epoch:Math.floor(Date.now()/1000)}).catch(function(){timeSent=false})}
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
poll();setInterval(poll,2000);
</script></body></html>)HTML";
