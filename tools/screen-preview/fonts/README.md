# ฟอนต์ตัวจริงสำหรับภาพหน้าจอจำลอง

ไฟล์ในโฟลเดอร์นี้คือ **ข้อมูลฟอนต์ต้นฉบับของไลบรารีที่เฟิร์มแวร์ใช้จริง**
คัดลอกมาไม่แก้ไขแม้แต่ไบต์เดียว ตัวเรนเดอร์ภาพจำลองอ่านไฟล์เหล่านี้โดยตรง
แล้วถอดรหัสด้วยวิธีเดียวกับไลบรารี (`../bitmapfont.py`)
ภาพใน `docs/screens-*` จึงเป็นพิกเซลเดียวกับที่จอจริงแสดง ไม่ใช่ฟอนต์หน้าตาคล้าย ๆ

ก่อนหน้านี้ตัวเรนเดอร์ใช้ฟอนต์ DejaVu (TrueType) วาดแทน ตำแหน่งกับขนาดใกล้เคียง
แต่รูปตัวอักษรไม่ใช่ของจริง และซ่อนปัญหาการจัดวางที่เกิดบนจอจริงไว้
เช่นนาฬิกาหน้าพักจอที่ชิดซ้ายเพราะเดาความกว้างของฟอนต์ logisoso ผิด

## ที่มา

ดาวน์โหลดเมื่อ 28 กันยายน 2026 จากสาขา `master` ของแต่ละโครงการ

| ไฟล์ | ใช้กับ | แหล่งที่มา |
|---|---|---|
| `u8g2_font_*_tf.c` | จอ OLED (Host-OLED, Station-C3-OLED) | [olikraus/u8g2](https://github.com/olikraus/u8g2) `tools/font/build/single_font_files/` |
| `glcdfont.c` | จอ TFT (Station, Station-Stable, Host-TFT28, Host-Touch24) | [adafruit/Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library) `glcdfont.c` |

## สัญญาอนุญาต

หัวไฟล์ต้นฉบับของทุกไฟล์ยังอยู่ครบ ระบุชื่อฟอนต์และลิขสิทธิ์ไว้ตามเดิม
ไฟล์เหล่านี้ใช้เฉพาะในเครื่องมือจำลองบน PC ไม่ได้ถูกคอมไพล์ลงเฟิร์มแวร์
(เฟิร์มแวร์ดึงฟอนต์จากไลบรารีที่ติดตั้งใน Arduino IDE เอง)

| ฟอนต์ | ต้นกำเนิด | สัญญาอนุญาต |
|---|---|---|
| 4x6, 5x8, 6x10, 7x13, 7x14B | X11 misc-fixed | Public domain |
| helvB10, helvB12 | X11 Adobe Helvetica 75dpi | ลิขสิทธิ์ Adobe และ DEC (ดูหัวไฟล์) แจกจ่ายมากับ X11 และ U8g2 ตามเงื่อนไขที่ U8g2 ระบุไว้ใน [u8g2 wiki](https://github.com/olikraus/u8g2/wiki/fntgrp) |
| logisoso16, logisoso32 | Logisoso โดย Mathieu Gabiot | ตามที่ U8g2 ระบุไว้ใน [u8g2 wiki](https://github.com/olikraus/u8g2/wiki/fntgrp) |
| glcdfont (5x7 classic) | Adafruit GFX Library | BSD ตาม `license.txt` ของไลบรารี |

## เพิ่มฟอนต์ใหม่

ถ้าเฟิร์มแวร์เริ่มใช้ฟอนต์ U8g2 ตัวใหม่ ต้องทำสามอย่างให้ครบ

1. ดาวน์โหลด `u8g2_font_<ชื่อ>_tf.c` มาวางที่นี่ (ไม่แก้ไข)
2. เพิ่มชื่อฟอนต์ใน `../U8g2lib.h` (`setFont()`) และในรายการ `FONTS` ของ `../gen_u8g2_widths.py`
3. รัน `python3 ../gen_u8g2_widths.py` เพื่อสร้าง `../u8g2_widths.h` ใหม่
   ตัวจำลองใช้ตารางนี้ตอบ `u8g2.getStrWidth()` ให้ตรงกับจอจริง

ตรวจว่าถอดรหัสได้ถูกต้องครบทุกตัวอักษรด้วย `bash tools/font-check/run.sh`
