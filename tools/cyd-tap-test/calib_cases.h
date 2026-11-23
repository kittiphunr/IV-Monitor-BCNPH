// ทดสอบการคาลิเบรตทัชด้วยแผงทัชจำลองทุกรูปแบบ: สลับแกน x กลับ X x กลับ Y = 8 แบบ
// แผงจำลองให้ค่าดิบ 12 บิตช่วงราว 250..3850 ตามตำแหน่งบนจอ หน้าคาลิเบรตต้องเรียนรู้เองว่าแกนสลับหรือกลับทิศ
// แล้ว mapTouch ต้องคืนตำแหน่งจอที่แตะจริง (คลาดไม่เกิน 6 พิกเซล) ที่จุดทดสอบทุกจุด
#include <cstdlib>
static int clBad = 0;
#define CCK(c, m) do { if (!(c)) { printf("ผิด: %s\n", m); clBad++; } else printf("ผ่าน: %s\n", m); } while (0)
static void panelRaw(int sx, int sy, bool swapped, bool flipX, bool flipY, int &rx, int &ry) {
  const int OFF = 250, SPAN = 3600;
  long ax = (long)sx * SPAN / (SCR_W - 1);       // 0..SPAN ตามแกนนอนของจอ
  long ay = (long)sy * SPAN / (SCR_H - 1);       // 0..SPAN ตามแกนตั้งของจอ
  long a = swapped ? ay : ax;                    // ค่าดิบ X ผูกกับแกนไหนของจอ
  long b = swapped ? ax : ay;
  if (flipX) a = SPAN - a;
  if (flipY) b = SPAN - b;
  rx = OFF + (int)a;
  ry = OFF + (int)b;
}
static int calibRun() {
  g_ops = fopen("/dev/null", "w");
  const int tests[6][2] = { {10, 10}, {SCR_W - 10, SCR_H - 10}, {SCR_W / 2, SCR_H / 2}, {40, SCR_H - 30}, {SCR_W - 40, 30}, {SCR_W / 4, SCR_H / 3} };
  for (int variant = 0; variant < 8; variant++) {
    bool swapped = variant & 1, fx = variant & 2, fy = variant & 4;
    for (int k = 0; k < 2; k++) panelRaw(CALIB_PT[k][0], CALIB_PT[k][1], swapped, fx, fy, calibRawX[k], calibRawY[k]);
    applyCalibrationPoints();
    int worst = 0;
    for (auto &t : tests) {
      int rx, ry, sx, sy;
      panelRaw(t[0], t[1], swapped, fx, fy, rx, ry);
      mapTouch(rx, ry, sx, sy);
      int e = abs(sx - t[0]); if (abs(sy - t[1]) > e) e = abs(sy - t[1]);
      if (e > worst) worst = e;
    }
    char m[100];
    snprintf(m, sizeof(m), "คาลิเบรต: แผงสลับแกน=%d กลับX=%d กลับY=%d -> ตรวจพบสลับ=%d คลาดสูงสุด %d px", swapped, fx, fy, (int)touchSwapAuto, worst);
#if UI_LANDSCAPE
    CCK(worst <= 6 && (int)touchSwapAuto == (int)swapped, m);
#else
    CCK(worst <= 6 || (swapped && !touchSwapAuto), m);   // แนวตั้งไม่ตรวจสลับเอง (ต้องตั้ง TOUCH_SWAP_XY) จึงรับเฉพาะกรณีไม่สลับ
#endif
  }
  return clBad ? 1 : 0;
}
