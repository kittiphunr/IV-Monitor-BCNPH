/**
 * @file      rate_estimator.h
 * @brief     ตัวคำนวณอัตราการไหลของ Host จากแพ็กเก็ต Protocol v4 (จำนวนหยด + เวลาจากหยดล่าสุด)
 * @version   1.0.0-TG
 * @date      2026-11-19
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * ทุกแพ็กเก็ตบอกว่า "หยดล่าสุดตกเมื่อ msSinceLastDrop มิลลิวินาทีก่อนส่ง และนับถึงหยดที่ totalDrops แล้ว"
 * Host ย้อนไปหาเวลาแน่นอนของหยดนั้นในฐานเวลาของตัวเอง (เวลารับ - msSinceLastDrop) ได้ เรียกว่าหนึ่ง "จุดยึด"
 * หยดที่ตกระหว่างสองจุดยึดไม่รู้เวลาเป็นรายหยด แต่รู้ "จำนวน" แน่นอน (ผลต่างของ totalDrops)
 *   อัตรา = (จำนวนหยดระหว่างจุดยึดแรกกับจุดยึดสุดท้าย) / (เวลาระหว่างสองจุดนั้น)
 * ทั้งเศษและส่วนเป็นค่าแน่นอนทั้งคู่ จึงแม่นเท่ากับที่ Station เคยคำนวณจากเวลาของทุกหยดเอง
 * และไม่ต้องรู้ว่าหยุดที่แพ็กเก็ตหายไปกี่ใบ (ผลต่างของ totalDrops ครอบคลุมเอง)
 *
 * กติกาเดียวกับหน้าต่างของ Station รุ่น 7.7.3 ที่เป็นรากของค่าที่ระบบใช้เตือนอยู่แล้ว
 *   - หน้าต่างไม่เกิน 8 หยด (7 ช่วง)
 *   - ล้างหน้าต่างเมื่อหยุดชั่วคราว/เริ่มใหม่ ตัวนับถอยหลัง หรือมีช่วงห่างเกิน MAX_OCCLUSION_MS
 *   - ล้างหน้าต่างเมื่ออัตราเปลี่ยนก้าวกระโดดเกิน 3 เท่า (หมุนโรลเลอร์แคลมป์) ไม่ให้ค่าเก่าถ่วง
 *
 * ไฟล์นี้ไม่พึ่งไลบรารีใด ทดสอบบนเครื่อง PC ได้ตรง ๆ ด้วย `bash tools/v4-test/run.sh`
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 1.0.0-TG | 2026-11-19 | รุ่นแรก |
 *
 * @warning  ห้ามเรียก millis() ในไฟล์นี้ ต้องส่งเวลารับแพ็กเก็ตเข้ามาทุกครั้ง เพื่อให้ทดสอบด้วยนาฬิกาจำลองได้
 */
#pragma once
#include <stdint.h>

#define RATE_EST_WINDOW_DROPS   8          // เท่ากับ RATE_WINDOW_DROPS ของ Station 7.7.3
#define RATE_EST_ANCHORS        10         // จุดยึดสูงสุดที่จำ (มากกว่าหน้าต่างเผื่อทิ้งตอนหน้าต่างเต็ม)
#define RATE_EST_MAX_GAP_MS     300000UL   // ห่างกว่านี้ = ไม่ใช่การไหลต่อเนื่อง
#define RATE_EST_MIN_GAP_MS     55UL       // เท่ากับ minDropInterval ของ Station: เร็วกว่านี้เป็นสัญญาณรบกวน

struct RateEstimator {
  uint32_t anchorMs[RATE_EST_ANCHORS];     // เวลาแน่นอนของหยดล่าสุดที่แพ็กเก็ตนั้นรายงาน
  uint32_t anchorCnt[RATE_EST_ANCHORS];    // totalDrops ณ จุดนั้น
  uint8_t  n        = 0;                   // จำนวนจุดยึดที่มี (เรียงเก่า -> ใหม่)
  bool     hasPrev  = false;
  bool     prevRunning = true;
  uint32_t prevTotal = 0;
  float    rateHr   = 0.0f;                // ผลล่าสุด (mL/h) 0 = ยังคำนวณไม่ได้
  bool     valid    = false;

  // ล้างหน้าต่างแต่คงค่าที่แสดงอยู่ เหมือน Station 7.7.3 ที่ทิ้งหน้าต่างเก่าแต่ยังโชว์ค่าล่าสุดจนกว่าจะคำนวณใหม่ได้
  void resetWindow() { n = 0; }

  // ล้างทั้งหน้าต่างและค่าที่แสดง ใช้เมื่อหยุดชั่วคราว ตัวนับถอยหลัง หรือสายพับ (อัตราจริงเป็น 0)
  void reset() { n = 0; rateHr = 0.0f; valid = false; }

  // หน่วงทั้งหมดของ "ตัวนับหยดย้อนกลับ" ฯลฯ ใช้เมื่อเริ่มถุงใหม่จากฝั่ง Host
  void clearAll() { reset(); hasPrev = false; }

  // เรียกทุกครั้งที่ได้รับแพ็กเก็ตขึ้นหนึ่งใบ
  //   nowMs      เวลาที่ได้รับ (ฐานเวลาของ Host)
  //   totalDrops จำนวนหยดสะสม
  //   msSince    มิลลิวินาทีจากหยดล่าสุดถึงตอนส่ง (0 = ยังไม่มีหยด)
  //   running    Station กำลังนับอยู่หรือไม่
  //   df         drop factor (หยด/mL)
  // คืนอัตราล่าสุด (mL/h) และตั้ง valid
  float update(uint32_t nowMs, uint32_t totalDrops, uint32_t msSince, bool running, uint8_t df) {
    if (!running) {                          // หยุดชั่วคราว: อัตราเป็น 0 และเริ่มหน้าต่างใหม่เมื่อกลับมา
      reset();
      prevRunning = false;
      prevTotal = totalDrops;
      hasPrev = true;
      return 0.0f;
    }
    bool resumed = hasPrev && !prevRunning;
    bool counterBack = hasPrev && totalDrops < prevTotal;     // Station รีเซ็ตหรือรีบูต
    if (resumed || counterBack) reset();
    prevRunning = true;
    prevTotal = totalDrops;
    hasPrev = true;

    if (msSince == 0 || totalDrops == 0) return rateHr;       // ยังไม่มีหยดให้ยึด
    uint32_t dropAt = nowMs - msSince;

    // หยดล่าสุดเป็นหยดเดิมที่เคยยึดไว้แล้ว (ยังไม่มีหยดใหม่) -> ไม่มีข้อมูลใหม่
    if (n > 0 && anchorCnt[n - 1] == totalDrops) return rateHr;

    if (n > 0) {
      uint32_t dt = dropAt - anchorMs[n - 1];
      uint32_t dc = totalDrops - anchorCnt[n - 1];
      // เวลาย้อนหลัง หรือช่วงผิดปกติ: ไม่เอามาปนกับของเดิม
      if ((int32_t)dt <= 0 || dt >= RATE_EST_MAX_GAP_MS || (dc > 0 && dt / dc < RATE_EST_MIN_GAP_MS)) {
        resetWindow();
      } else if (n >= 2) {
        // อัตราเปลี่ยนก้าวกระโดดเกิน 3 เท่าเทียบกับค่าเฉลี่ยในหน้าต่าง
        uint32_t winDt = anchorMs[n - 1] - anchorMs[0];
        uint32_t winDc = anchorCnt[n - 1] - anchorCnt[0];
        if (winDc > 0 && dc > 0) {
          uint64_t meanIv = winDt / winDc;       // ms ต่อหยดในหน้าต่าง
          uint64_t newIv  = dt / dc;             // ms ต่อหยดในช่วงล่าสุด
          if (newIv > meanIv * 3 || newIv * 3 < meanIv) resetWindow();
        }
      }
    }

    // ใส่จุดยึดใหม่ ทิ้งจุดเก่าสุดเมื่อเต็ม
    if (n == RATE_EST_ANCHORS) {
      for (uint8_t i = 1; i < n; i++) { anchorMs[i - 1] = anchorMs[i]; anchorCnt[i - 1] = anchorCnt[i]; }
      n--;
    }
    anchorMs[n] = dropAt;
    anchorCnt[n] = totalDrops;
    n++;

    // หน้าต่าง: ไม่เกิน RATE_EST_WINDOW_DROPS-1 ช่วง (นับเป็นจำนวนหยด) แต่ต้องเหลืออย่างน้อยสองจุด
    uint8_t first = 0;
    while (n - first > 2 && (anchorCnt[n - 1] - anchorCnt[first]) > (RATE_EST_WINDOW_DROPS - 1)) first++;

    if (n - first >= 2) {
      uint32_t span  = anchorMs[n - 1] - anchorMs[first];
      uint32_t drops = anchorCnt[n - 1] - anchorCnt[first];
      if (span > 0 && drops > 0 && df > 0) {
        rateHr = ((float)drops / (float)df) * 3600000.0f / (float)span;
        valid = true;
      }
    }
    return rateHr;
  }
};
