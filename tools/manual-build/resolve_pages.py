#!/usr/bin/env python3
"""อ่าน PDF ที่แปลงจากคู่มือ แล้วหาเลขหน้าจริงของทุกบรรทัดในสารบัญ

เลขหน้าที่พิมพ์ในเล่มเริ่มนับ ๑ ใหม่ที่บทที่ ๑ จึงต้องหาค่าชดเชยจากหน้า PDF ก่อน
"""
import json, re, subprocess, sys

pdf = sys.argv[1]
out = sys.argv[2]
entries_js = sys.argv[3]

npages = int(re.search(r'Pages:\s+(\d+)',
             subprocess.run(['pdfinfo', pdf], capture_output=True, text=True).stdout).group(1))

pages = []
for i in range(1, npages + 1):
    t = subprocess.run(['pdftotext', '-f', str(i), '-l', str(i), pdf, '-'],
                       capture_output=True, text=True).stdout
    pages.append(re.sub(r'\s+', '', t))

# ดึงข้อความของทุกบรรทัดสารบัญออกจากไฟล์ต้นฉบับ
src = open(entries_js, encoding='utf-8').read()
keys = [k for k in re.findall(r"toc\('([^']+)'", src) if k.strip() != 'ตารางที่']
# บรรทัดที่สร้างด้วยโค้ด (บทที่ ๘-๑๕ และตารางของภาคผนวกที่เลขเลื่อนตามจำนวนตาราง)
# ไม่ได้เขียนเป็นข้อความตรง ๆ ใน part_front.js จึงถามรายการจาก part_models.js เอง
dyn = subprocess.run(['node', '-e', """
const M = require('./part_models.js');
const n = (k) => 'ตารางที่ ' + M.thai(M.TABLE_NEXT + k) + '  ';
console.log(JSON.stringify([...M.tocList.map((e) => e.text), ...M.tableList, ...M.figureList,
  n(0) + 'อัตราหยดสำเร็จรูป', n(1) + 'สัญลักษณ์ที่ใช้ในสมการ', n(2) + 'สรุปสมการทั้งหมดที่ระบบใช้']));
"""], capture_output=True, text=True, cwd=__import__('os').path.dirname(__import__('os').path.abspath(entries_js)))
if dyn.returncode != 0:
    print(dyn.stderr); sys.exit(1)
keys += json.loads(dyn.stdout)

# ส่วนนำจบที่หน้า "สารบัญภาพ" (หน้าสุดท้ายที่มีคำนี้) เนื้อหาเริ่มหน้าถัดไป
last_front = None
for i, t in enumerate(pages):
    if 'สารบัญภาพ' in t:
        last_front = i
if last_front is None:
    print('หาไม่พบหน้าสารบัญภาพ'); sys.exit(1)
offset = last_front + 1        # index ฐานศูนย์ของหน้าที่พิมพ์เลข ๑
# สารบัญภาพยาวเกินหนึ่งหน้าได้ หน้าต่อของรายการไม่มีคำว่า "สารบัญภาพ" ซ้ำ
# จึงเลื่อนไปหาหน้าแรกที่ขึ้นต้นบทที่ ๑ จริง
for i in range(offset, npages):
    if 'บทที่๑บทนำ' in pages[i]:
        offset = i; break
if 'บทที่๑' not in pages[offset]:
    print('คำเตือน: หน้าแรกของเนื้อหาไม่ได้ขึ้นต้นด้วยบทที่ ๑')

def printed(idx):
    return idx - offset + 1

mapping, missing = {}, []
for k in keys:
    flat = re.sub(r'\s+', '', k)
    if flat in ('คำนำ', 'สารบัญ', 'สารบัญตาราง', 'สารบัญภาพ'):
        continue                                  # ส่วนนำใช้เลขพยัญชนะไทยที่กำหนดไว้แล้ว
    # หัวข้อย่อยค้นได้ตรงตัว ส่วนหัวบท/ภาคผนวกต้องตัดคำนำหน้าออก
    needle = flat
    m = re.match(r'^บทที่[๐-๙]+(.+)$', flat)
    if m: needle = m.group(1)
    m = re.match(r'^ภาคผนวก[ก-ฮ](.+)$', flat)
    if m: needle = m.group(1)
    found = None
    if flat == 'ภาคผนวก':
        # หน้าคั่นภาคผนวกมีคำนี้คำเดียว (กับเลขหน้า) ห้ามจับคำว่า "ภาคผนวก ค" ในเนื้อหา
        for i in range(offset, npages):
            if re.sub(r'^[0-9๐-๙]+', '', pages[i]) == 'ภาคผนวก':
                found = printed(i); break
    # หัวบทลองหาพร้อมคำนำหน้า "บทที่ n" ก่อน เพราะชื่อบทที่แยกตามรุ่น (เช่น "เครื่องศูนย์กลางจอ OLED")
    # ถูกเอ่ยถึงในเนื้อหาของบทก่อนหน้าด้วย ถ้าตัดคำนำหน้าทิ้งเลยจะได้หน้าที่เอ่ยถึงแทนหน้าหัวบท
    for cand in ((flat, needle) if found is None else ()):
        for i in range(offset, npages):
            if cand in pages[i]:
                found = printed(i); break
        if found is not None: break
    if found is None:
        missing.append(k)
    else:
        mapping[flat] = found

json.dump(mapping, open(out, 'w', encoding='utf-8'), ensure_ascii=False, indent=0)
print('จำนวนหน้าใน PDF:', npages, ' หน้าแรกของเนื้อหาอยู่ที่ PDF หน้า', offset + 1)
print('เติมเลขหน้าได้', len(mapping), 'รายการ')
if missing:
    print('หาไม่พบ', len(missing), 'รายการ:')
    for m in missing: print('   ', m)
