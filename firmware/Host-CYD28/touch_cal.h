/**
 * @file      touch_cal.h
 * @brief     คำนวณและใช้ค่าคาลิเบรตทัชแบบ affine จากจุดสัมผัสสามจุด
 * @version   2.0.0-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * แปลงค่าดิบของ XPT2046 (0-4095) เป็นพิกเซลด้วยสมการ sx=a*rx+b*ry+c, sy=d*rx+e*ry+f ซึ่งครอบคลุมทั้งแกนสลับ กลับด้าน และเอียง จึงไม่ต้องเดาว่าบอร์ดต่อแกนแบบไหน ไม่ขึ้นกับ Arduino เพื่อทดสอบบนเครื่อง PC ได้
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.0-CYD | 2026-11-27 | เขียนใหม่ทั้งหมดด่านที่ 1 (ฐานบอร์ด: จอ ไฟหลังจอ ทัช) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <stdint.h>
#include <math.h>

struct CalPoint { float rx, ry, sx, sy; };      // ค่าดิบที่อ่านได้ กับพิกเซลที่ควรเป็น

struct TouchCal {
  float a = 0, b = 0, c = 0, d = 0, e = 0, f = 0;
  bool valid = false;
};

// จุดเป้าหมายสามจุดสำหรับหน้าคาลิเบรต (ห่างกันเพื่อให้สมการนิ่ง ไม่ติดขอบจอ)
inline void touchCalTargets(int w, int h, CalPoint out[3]) {
  out[0].sx = w * 0.15f; out[0].sy = h * 0.15f;
  out[1].sx = w * 0.85f; out[1].sy = h * 0.20f;
  out[2].sx = w * 0.50f; out[2].sy = h * 0.85f;
}

// แก้ระบบสมการ 3 จุด (Cramer) คืน false ถ้าจุดอยู่ในแนวเดียวกันหรือผลไม่สมเหตุสมผล
inline bool touchCalSolve(const CalPoint p[3], int w, int h, TouchCal &out) {
  out = TouchCal();
  const float det = p[0].rx * (p[1].ry - p[2].ry) - p[0].ry * (p[1].rx - p[2].rx)
                  + (p[1].rx * p[2].ry - p[2].rx * p[1].ry);
  if (fabsf(det) < 20000.0f) return false;       // จุดดิบอยู่เกือบแนวเดียวกัน
  auto solve = [&](float s0, float s1, float s2, float &ka, float &kb, float &kc) {
    ka = (s0 * (p[1].ry - p[2].ry) - p[0].ry * (s1 - s2) + (s1 * p[2].ry - s2 * p[1].ry)) / det;
    kb = (p[0].rx * (s1 - s2) - s0 * (p[1].rx - p[2].rx) + (p[1].rx * s2 - p[2].rx * s1)) / det;
    kc = (p[0].rx * (p[1].ry * s2 - p[2].ry * s1) - p[0].ry * (p[1].rx * s2 - p[2].rx * s1)
          + s0 * (p[1].rx * p[2].ry - p[2].rx * p[1].ry)) / det;
  };
  solve(p[0].sx, p[1].sx, p[2].sx, out.a, out.b, out.c);
  solve(p[0].sy, p[1].sy, p[2].sy, out.d, out.e, out.f);
  // ความสมเหตุสมผล: ช่วงดิบเต็ม 0-4095 ต้องกวาดพื้นที่จอได้อย่างน้อยครึ่งหนึ่งในแต่ละแกน
  float xs[4], ys[4];
  const float rr[4][2] = {{0, 0}, {4095, 0}, {0, 4095}, {4095, 4095}};
  for (int i = 0; i < 4; i++) {
    xs[i] = out.a * rr[i][0] + out.b * rr[i][1] + out.c;
    ys[i] = out.d * rr[i][0] + out.e * rr[i][1] + out.f;
  }
  float xmin = xs[0], xmax = xs[0], ymin = ys[0], ymax = ys[0];
  for (int i = 1; i < 4; i++) {
    if (xs[i] < xmin) xmin = xs[i];
    if (xs[i] > xmax) xmax = xs[i];
    if (ys[i] < ymin) ymin = ys[i];
    if (ys[i] > ymax) ymax = ys[i];
  }
  if ((xmax - xmin) < 0.5f * w || (ymax - ymin) < 0.5f * h) { out = TouchCal(); return false; }
  out.valid = true;
  return true;
}

inline void touchCalApply(const TouchCal &k, float rx, float ry, int w, int h, int16_t &sx, int16_t &sy) {
  float x = k.a * rx + k.b * ry + k.c;
  float y = k.d * rx + k.e * ry + k.f;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x > w - 1) x = (float)(w - 1);
  if (y > h - 1) y = (float)(h - 1);
  sx = (int16_t)(x + 0.5f);
  sy = (int16_t)(y + 0.5f);
}
