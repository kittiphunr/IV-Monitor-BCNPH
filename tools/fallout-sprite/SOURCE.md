# ที่มาของภาพ Vault Boy

โฟลเดอร์นี้มีภาพต้นทางสองไฟล์ ใช้คนละหน้าที่

| ไฟล์ | ใช้ทำอะไร |
|---|---|
| `walk/01.jpg` ถึง `walk/21.jpg` | **เฟรมภาพเคลื่อนไหวต้นฉบับ** ที่ `make_sprite.py` ใช้ครบทุกเฟรม (ตั้งแต่ 1.4.0-FO) |
| `vaultboy_walk.webp` | ภาพนิ่งท่าเดียว ใช้เฉพาะเมื่อไม่มีโฟลเดอร์ `walk/` (1.2.0-FO ถึง 1.3.1-FO) เหมือน `walk/01.jpg` |
| `statpip0.bmp` | **จานสี** ของจอสาย Fallout ทั้งเครื่อง (`tools/fallout-sync/sync.py`) และภาพตัวละครของรุ่น 1.1.0-FO |

## walk/ เฟรมภาพเคลื่อนไหว 21 เฟรม

| | |
|---|---|
| ไฟล์ | `01.jpg` ถึง `21.jpg` (358x498 RGB พื้นดำ เส้นสแกนแบบจอ CRT) |
| ที่มา | ผู้ดูแลโครงการส่งมาเป็นไฟล์ `Archive.zip` เมื่อ 2026-09-29 ไม่ทราบต้นฉบับ |
| SHA-256 ของ zip | `553ba7b70272f4637654f5d2f1fe349492bc92d85df0b5a6401ff93a96113d78` |

คัดลอกมาทั้งไฟล์โดยไม่แก้ไข แค่เติมศูนย์หน้าเลขชื่อไฟล์ (`1.jpg` เป็น `01.jpg`) ให้เรียงถูก
ทิ้งไฟล์ `__MACOSX/._*` ซึ่งเป็นข้อมูลแนบของ macOS ไม่ใช่ภาพ
`01.jpg` เป็นท่าเดียวกับ `vaultboy_walk.webp` (ต่างกันเฉลี่ยไม่ถึง 1 ระดับสี)
เฟรม 1 ถึง 11 กับ 12 ถึง 21 เป็นรอบก้าวเดียวกันสองรอบที่ต่างกันเล็กน้อย
บางคู่เหมือนกันทุกพิกเซล (10 กับ 11, 15 กับ 16) หลังแปลงจึงเก็บครั้งเดียว

## vaultboy_walk.webp

| | |
|---|---|
| ไฟล์ | `vaultboy_walk.webp` (358x498 RGB พื้นดำ เส้นสแกนแบบจอ CRT) |
| ที่มา | ผู้ดูแลโครงการส่งมาเป็นภาพอ้างอิงในการสนทนาเมื่อ 2026-09-28 ไม่ทราบต้นฉบับ |
| SHA-256 | `95cfaef7782af8d5576bc7a1f34cad7552e8ec95a5d222fb3330709298b291a9` |

คัดลอกมาทั้งไฟล์โดยไม่แก้ไข ภาพมีท่าเดินท่าเดียว ท่าอื่นสร้างโดย `make_sprite.py`
ถ้าหาภาพเคลื่อนไหวต้นฉบับครบทุกเฟรมได้ ควรเปลี่ยนมาใช้เฟรมจริงแทนท่าที่สร้างขึ้น

## statpip0.bmp

| | |
|---|---|
| ไฟล์ | `statpip0.bmp` (280x240 สีแบบจานสี 8 บิต ใช้จริง 16 สี) |
| โครงการต้นทาง | Adafruit Learning System Guides — `PipBoy_2040/img/statpip0.bmp` |
| ลิงก์ | https://github.com/adafruit/Adafruit_Learning_System_Guides/blob/main/PipBoy_2040/img/statpip0.bmp |
| ดึงจากคอมมิต | `b951847fc3ccf49d88949dea4de60664c6d2afe6` (แบรนช์ `main` วันที่ 2026-09-28) |
| SHA-256 | `a409fa471cebae8e482ca9ee668c751ff3acac632a526e72beb4efa4725ca5fb` |
| สัญญาอนุญาตของรีโพต้นทาง | MIT License, Copyright (c) 2018 Adafruit Industries |

คัดลอกมาทั้งไฟล์โดยไม่แก้ไข ตอนนี้ใช้เฉพาะจานสี 16 ระดับเป็นชุดสีของจอสาย Fallout

## เครื่องหมายการค้า

Vault Boy, Pip-Boy และ Fallout เป็นเครื่องหมายการค้าของ Bethesda Softworks LLC
สัญญาอนุญาต MIT ของ Adafruit ครอบคลุมเฉพาะสิทธิ์ของ Adafruit ใน `statpip0.bmp`
ไม่ได้ให้สิทธิ์ในตัวละครหรือเครื่องหมายการค้าของ Bethesda
ส่วน `vaultboy_walk.webp` ไม่ทราบต้นฉบับ จึงไม่มีสัญญาอนุญาตใดรับรองเลย

โครงการนี้ใช้ภาพนี้เพื่อการศึกษา ไม่แสวงหากำไร เช่นเดียวกับโครงการต้นทาง
**ถ้าจะนำเครื่องไปจำหน่ายหรือใช้เชิงพาณิชย์** ให้ใช้สายหลัก `firmware/Station/` แทน
หรือเปลี่ยนภาพตัวละครเป็นภาพที่มีสิทธิ์ใช้งาน แล้วสร้าง `pipboy_sprite.h` ใหม่ด้วยสคริปต์เดิม
(ต้องปรับตำแหน่งสะโพก ข้อเท้า และเส้นแบ่งขาในหัวสคริปต์ให้ตรงกับภาพใหม่)

## สัญญาอนุญาต MIT ของรีโพต้นทาง

```
MIT License

Copyright (c) 2018 Adafruit Industries

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
