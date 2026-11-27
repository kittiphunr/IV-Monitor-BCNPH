/**
 * @file      colors.h
 * @brief     ค่าสี RGB565 ที่ใช้ร่วมกันทุกหน้าจอ
 * @version   2.0.1-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * สีพื้นฐานของหน้าจอทดสอบ หน้าคาลิเบรต และหน้าสถานะ
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.1-CYD | 2026-11-27 | แยกออกจาก Host-CYD28.ino เมื่อเริ่มด่านที่ 2 เพื่อให้ไฟล์หลักเหลือแต่แกนเครือข่าย (โค้ดเดิมจากด่านที่ 1 ไม่เปลี่ยนพฤติกรรม) |
 * | 2.0.0-CYD | 2026-11-27 | เขียนใหม่ทั้งหมดด่านที่ 1 (ฐานบอร์ด) |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <stdint.h>
static const uint16_t C_BLACK = 0x0000, C_WHITE = 0xFFFF, C_RED = 0xF800, C_GREEN = 0x07E0, C_BLUE = 0x001F,
                      C_YELLOW = 0xFFE0, C_CYAN = 0x07FF, C_MAGENTA = 0xF81F, C_GRAY = 0x8410;


