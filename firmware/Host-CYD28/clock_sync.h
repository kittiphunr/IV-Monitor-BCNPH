/**
 * @file      clock_sync.h
 * @brief     นาฬิกาของ Host: ตั้งจากเบราว์เซอร์ (และ RTC DS3231 ถ้าเปิดใช้)
 * @version   2.2.0-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * Host ไม่มีอินเทอร์เน็ต (จุดเชื่อมต่อ Wi-Fi ของตัวเองเท่านั้น) จึงรับเวลาจากเครื่องที่เปิดหน้าเว็บ แล้วส่งให้ Station ผ่าน Sync clockEpoch() คืน 0 จนกว่าจะตั้งเวลาแล้ว เพื่อไม่ให้ Station เข้าใจผิดว่าเวลาตรงทั้งที่ยังไม่ได้ตั้ง ไม่มี NTP โดยตั้งใจ เพราะต้องเชื่อมเราเตอร์ซึ่งบังคับให้ช่องสัญญาณ ESP-NOW เปลี่ยนตามเราเตอร์
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.2.0-CYD | 2026-11-27 | clockTzMin: เขตเวลาที่เบราว์เซอร์ส่งมา ใช้ตั้งชื่อไฟล์รายวันบน SD ตามเวลาท้องถิ่น |
 * | 2.0.3-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 4 (เว็บ นาฬิกา SD ประวัติเหตุการณ์) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <sys/time.h>
#include <time.h>
#include "board.h"
#if CYD_RTC_SDA >= 0
  #include <Wire.h>
  #include <RTClib.h>
  static RTC_DS3231 rtc;
  static_assert(CYD_RTC_SDA != CYD_BUZZER_PIN && CYD_RTC_SCL != CYD_BUZZER_PIN, "ขา RTC ชนกับขาบัซเซอร์ ย้ายบัซเซอร์ก่อน (CYD_BUZZER_PIN)");
#endif

static bool clockSynced = false;
static int clockTzMin = 0;                          // เขตเวลาท้องถิ่น (นาที จาก UTC) ที่เบราว์เซอร์ส่งมา ไทย = 420 ใช้ตั้งชื่อไฟล์รายวันบน SD
constexpr uint32_t EPOCH_MIN = 1600000000UL;      // ก่อนหน้านี้ถือว่าไม่ใช่เวลาจริง (ก.ย. 2020)
constexpr uint32_t EPOCH_MAX = 4100000000UL;

static uint32_t clockEpoch() {
  return clockSynced ? (uint32_t)time(nullptr) : 0;
}

static void clockSet(uint32_t epoch) {
  struct timeval tv = { (time_t)epoch, 0 };
  settimeofday(&tv, nullptr);
  clockSynced = true;
#if CYD_RTC_SDA >= 0
  rtc.adjust(DateTime(epoch));
#endif
}

static bool clockBegin() {
#if CYD_RTC_SDA >= 0
  Wire.begin(CYD_RTC_SDA, CYD_RTC_SCL);
  if (rtc.begin(&Wire) && !rtc.lostPower()) {
    const uint32_t e = rtc.now().unixtime();
    if (e >= EPOCH_MIN && e <= EPOCH_MAX) {
      struct timeval tv = { (time_t)e, 0 };
      settimeofday(&tv, nullptr);
      clockSynced = true;
    }
  }
#endif
  return clockSynced;
}
