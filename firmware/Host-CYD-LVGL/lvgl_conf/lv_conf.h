/**
 * lv_conf.h แบบย่อสำหรับ Host-CYD-LVGL (ใช้กับ LVGL 9.x)
 *
 * วิธีใช้: คัดลอกไฟล์นี้ไปไว้ที่ Documents/Arduino/libraries/lv_conf.h
 * (ข้างโฟลเดอร์ lvgl ไม่ใช่ข้างใน) แล้วคอมไพล์ใหม่
 *
 * ไฟล์นี้ตั้งเฉพาะค่าที่เฟิร์มแวร์ต้องใช้ ค่าอื่นทั้งหมด LVGL ใช้ค่าปริยายของตัวเองจาก lv_conf_internal.h
 * (LVGL 9.x รองรับไฟล์ตั้งค่าแบบย่อ) ถ้าต้องการปรับละเอียด ให้ใช้ lv_conf_template.h ของไลบรารีแทน
 *
 * ยังไม่ได้ทดสอบกับ LVGL จริง เขียนตามเอกสารของ LVGL 9.x
 */

#if 1   /* ต้องเป็น 1 ไม่งั้น LVGL จะไม่ใช้ไฟล์นี้ */

#ifndef LV_CONF_H
#define LV_CONF_H

/* จอ RGB565 */
#define LV_COLOR_DEPTH 16

/* หน่วยความจำ: ใช้ heap ของระบบ (malloc) แทนกองคงที่ภายในของ LVGL
 * ค่าปริยาย LV_STDLIB_BUILTIN สำรองกอง 64 KB ไว้ใน DRAM แบบคงที่ตั้งแต่เริ่ม ทำให้ ESP32 ขึ้น
 * "section .dram0.bss will not fit in region dram0_0_seg" (เกิน 39,200 ไบต์ในเครื่องผู้ใช้)
 * การใช้ CLIB ย้ายก้อนนี้ไปจองจาก heap ตอนรันแทน (ESP32 มี heap ว่างราว 200 KB) */
#define LV_USE_STDLIB_MALLOC  LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING  LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

/* ฟอนต์ที่เฟิร์มแวร์ใช้ (ดู F_XS/F_S/F_M/F_L/F_XL ใน HostScreen.h) */
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_48 1

#endif /* LV_CONF_H */

#endif /* "Set this to 1 to enable content" */
