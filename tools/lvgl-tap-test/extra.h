// กรณีเพิ่มของสาย LVGL: คลิกผ่านตัวแทน LVGL ที่หน้าจอราก ต้องไปถึงตัวจัดการของหน้านั้นเหมือนเรียก handle*Tap ตรง ๆ
static int lvBad = 0;
#define LCK(c, m) do { if (!(c)) { printf("ผิด: %s\n", m); lvBad++; } else printf("ผ่าน: %s\n", m); } while (0)
static int lvRun() {
  g_ops = fopen("/dev/null", "w");
  activeStationCount = 5;
  for (int i = 0; i < 5; i++) stations[i].active = true;
  displaySleeping = false; isPowerOffProgressActive = false;

  uiScreen = SCR_HOME; uiNeedFramework = true; updateHostDisplay();
  LCK(lvRoot != nullptr && lvRoot->cb != nullptr, "LVGL: หน้าจอรากมีตัวรับการคลิก");
  LCK(lvRoot->flags & LV_OBJ_FLAG_CLICKABLE, "LVGL: หน้าจอรากรับการแตะ");
  bool kidsClickable = false;
  for (lv_obj_t* k : lvRoot->kids) if (k->flags & LV_OBJ_FLAG_CLICKABLE) kidsClickable = true;
  LCK(!kidsClickable, "LVGL: วิตเจ็ตลูกทุกตัวไม่รับการแตะ (ไม่บังการคลิกที่หน้าจอราก)");

  lvMockClick(ZONE_GEAR.x + 5, ZONE_GEAR.y + 5);
  LCK(uiScreen == SCR_SYS, "คลิก LVGL ที่ปุ่ม SET หน้า HOME -> เปิดหน้า SYSTEM");
  updateHostDisplay();
  lvMockClick(ZONE_BACK.x + 5, ZONE_BACK.y + 5);
  LCK(uiScreen == SCR_HOME, "คลิก LVGL ที่ปุ่ม < -> กลับหน้า HOME");
  updateHostDisplay();
  lvMockClick(ZONE_FOCUS.x + 20, ZONE_FOCUS.y + 40);
  LCK(uiScreen == SCR_BED, "คลิก LVGL ที่การ์ดโฟกัส -> เปิดหน้า BED");

  displaySleeping = true;
  uiScreen = SCR_HOME;
  lvMockClick(ZONE_GEAR.x + 5, ZONE_GEAR.y + 5);
  LCK(!displaySleeping && uiScreen == SCR_HOME, "จอดับอยู่: คลิกครั้งแรกปลุกจอ ไม่ส่งต่อให้ปุ่ม");

  uiScreen = SCR_CALIB;
  lvMockClick(ZONE_GEAR.x + 5, ZONE_GEAR.y + 5);
  LCK(uiScreen == SCR_CALIB, "หน้า CALIB: ไม่รับคลิกผ่าน LVGL (อ่านค่าดิบเอง)");

  // ตัวอ่านทัชของ LVGL: ปล่อย = RELEASED, กด = PRESSED พร้อมพิกัดที่แมปแล้ว
  uiScreen = SCR_HOME; isPowerOffProgressActive = false;
  lv_indev_data_t d{};
  XPT2046_Touchscreen::g_touched = false;
  g_lv_read(nullptr, &d);
  LCK(d.state == LV_INDEV_STATE_RELEASED, "ตัวอ่านทัช LVGL: ไม่แตะ -> RELEASED");
  XPT2046_Touchscreen::g_touched = true;
  XPT2046_Touchscreen::g_point.x = 2000; XPT2046_Touchscreen::g_point.y = 2000; XPT2046_Touchscreen::g_point.z = 800;
  g_lv_read(nullptr, &d);
  LCK(d.state == LV_INDEV_STATE_PRESSED && d.point.x >= 0 && d.point.x < SCR_W && d.point.y >= 0 && d.point.y < SCR_H,
      "ตัวอ่านทัช LVGL: แตะ -> PRESSED พิกัดอยู่ในจอ");
  XPT2046_Touchscreen::g_point.z = 50;
  g_lv_read(nullptr, &d);
  LCK(d.state == LV_INDEV_STATE_RELEASED, "ตัวอ่านทัช LVGL: แรงกดน้อยเกิน -> RELEASED");
  uiScreen = SCR_CALIB; XPT2046_Touchscreen::g_point.z = 800;
  g_lv_read(nullptr, &d);
  LCK(d.state == LV_INDEV_STATE_RELEASED, "ตัวอ่านทัช LVGL: หน้า CALIB -> RELEASED (ไม่ให้ LVGL แย่งค่าดิบ)");
  return lvBad ? 1 : 0;
}
