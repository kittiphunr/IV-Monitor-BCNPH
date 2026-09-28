# ที่มาของภาพ Vault Boy

โฟลเดอร์นี้มีภาพต้นทางสองไฟล์ ใช้คนละหน้าที่

| ไฟล์ | ใช้ทำอะไร |
|---|---|
| `vaultboy_walk.webp` | **ภาพตัวละคร** ที่ `make_sprite.py` ขยับขาแล้วย่อเป็น `pipboy_sprite.h` (ตั้งแต่ 1.2.0-FO) |
| `statpip0.bmp` | **จานสี** ของจอสาย Fallout ทั้งเครื่อง (`tools/fallout-sync/sync.py`) และภาพตัวละครของรุ่น 1.1.0-FO |

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
