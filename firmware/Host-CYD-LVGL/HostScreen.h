/**
 * @file      HostScreen.h
 * @brief     ทุกอย่างที่วาดลงจอ 320x240 ของ Host บอร์ด CYD ด้วยวิตเจ็ต LVGL
 * @version   1.0.0-LVGL
 * @date      2026-11-23
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 * อาจารย์ที่ปรึกษา ดร.กรรณิการ์ กาศสมบูรณ์
 *
 * @par Description
 * เขียนใหม่จาก `Host-CYD28/HostScreen.h` ให้วาดด้วย LVGL 9.x ผังตำแหน่งทุกหน้าเหมือนผังแนวนอนของสาย Host-CYD28
 * แต่ละหน้าสร้างจากวิตเจ็ต (กล่อง ข้อความ แถบ) บนหน้าจอ LVGL หน้าเดียวที่ล้างแล้วสร้างใหม่เมื่อค่าที่แสดงเปลี่ยน
 * (ค่าเปลี่ยนไม่ถี่ ไม่เกินราวหนึ่งครั้งต่อวินาที) ยกเว้นหยดน้ำเกลือที่ขยับทุกรอบโดยไม่สร้างหน้าใหม่
 *
 * วิตเจ็ตทุกตัวไม่รับการแตะ ให้หน้าจอรากรับการคลิกเพียงที่เดียวแล้วส่งพิกัดเข้า `dispatchTap()` ในไฟล์หลัก
 * ซึ่งส่งต่อให้ตัวจัดการเดิม `handle*Tap` ที่ตรวจโซนปุ่มด้วยตาราง ZONE_* ชุดเดียวกับที่ใช้วาดที่นี่
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 1.0.0-LVGL | 2026-11-23 | สร้างใหม่ วาดด้วย LVGL 9.x แทนการวาดพิกเซลด้วย Adafruit_GFX |
 *
 * @warning  ถูก `#include` ท้ายไฟล์หลักก่อน `setup()` ห้ามย้ายขึ้นไปบนสุด
 *           เพราะโค้ดในนี้ใช้ตัวแปรและฟังก์ชันช่วยที่ประกาศไว้เหนือบรรทัด include
 *           ถ้าโค้ดเหนือบรรทัดนั้นเรียกฟังก์ชันในไฟล์นี้ ต้องประกาศล่วงหน้าในไฟล์หลักด้วย
 *           ตรวจด้วย `bash tools/split-check/run.sh`
 * @warning  **ยังไม่เคยคอมไพล์กับไลบรารี LVGL จริง** เขียนตาม API ของ LVGL 9.x (lv_display_*, lv_indev_*, lv_obj_*)
 *           ตรวจได้เฉพาะกับตัวแทนบน PC ถ้าคอมไพล์จริงแล้วขึ้นชื่อฟังก์ชันที่ไม่รู้จัก ให้เทียบเวอร์ชัน LVGL ก่อน
 * @warning  ข้อความที่วาดต้องเป็น ASCII ล้วน ฟอนต์ Montserrat ที่เปิดใน lv_conf.h ไม่มีอักษรไทย
 * @note     Arduino IDE แสดงไฟล์นี้เป็นแท็บของสเก็ตช์เดียวกัน เปิดคู่กับไฟล์หลักได้เลย
 */

#pragma once

// ---- ฟอนต์ (ต้องเปิดใน lv_conf.h: LV_FONT_MONTSERRAT_12 / 16 / 20 / 28 / 48 เป็น 1) ----
#define F_XS   (&lv_font_montserrat_12)
#define F_S    (&lv_font_montserrat_16)
#define F_M    (&lv_font_montserrat_20)
#define F_L    (&lv_font_montserrat_28)
#define F_XL   (&lv_font_montserrat_48)

// ---- สถานะของ LVGL ----
lv_obj_t *lvRoot = nullptr;        // หน้าจอรากที่ใช้ทุกหน้า
lv_obj_t *lvDrop = nullptr;        // หยดที่ขยับในหน้า BED (ว่างเมื่อไม่ได้อยู่หน้านั้น)
lv_obj_t *lvPoFill = nullptr;      // แถบความคืบหน้าหน้าปิดเครื่อง
String    lvKey = "";              // กุญแจของค่าที่แสดงอยู่ เปลี่ยนเมื่อไรต้องสร้างหน้าใหม่
static uint8_t lvBuf[SCR_W * 24 * 2] __attribute__((aligned(4)));   // บัฟเฟอร์วาดทีละ 24 บรรทัด (RGB565)

// สี RGB565 ของเฟิร์มแวร์ -> สีของ LVGL
lv_color_t lvc(uint16_t c) {
  return lv_color_make((uint8_t)(((c >> 11) & 0x1F) * 255 / 31),
                       (uint8_t)(((c >> 5) & 0x3F) * 255 / 63),
                       (uint8_t)((c & 0x1F) * 255 / 31));
}

// ---- ตัวช่วยสร้างวิตเจ็ต (ทุกตัวไม่รับการแตะและไม่เลื่อน) ----
static void lvPlain(lv_obj_t *o) {
  lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *lvBox(int x, int y, int w, int h, uint16_t col, int radius) {
  lv_obj_t *o = lv_obj_create(lvRoot);
  lvPlain(o);
  lv_obj_set_pos(o, x, y);
  lv_obj_set_size(o, w, h);
  lv_obj_set_style_bg_color(o, lvc(col), 0);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(o, radius, 0);
  lv_obj_set_style_border_width(o, 0, 0);
  lv_obj_set_style_pad_all(o, 0, 0);
  return o;
}

// กรอบเส้นอย่างเดียว (ไม่ทึบ)
lv_obj_t *lvRing(int x, int y, int w, int h, uint16_t col, int radius) {
  lv_obj_t *o = lvBox(x, y, w, h, col, radius);
  lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(o, 1, 0);
  lv_obj_set_style_border_color(o, lvc(col), 0);
  return o;
}

lv_obj_t *lvText(int x, int y, const String &s, const lv_font_t *f, uint16_t col) {
  lv_obj_t *l = lv_label_create(lvRoot);
  lvPlain(l);
  lv_label_set_text(l, s.c_str());
  lv_obj_set_style_text_font(l, f, 0);
  lv_obj_set_style_text_color(l, lvc(col), 0);
  lv_obj_set_pos(l, x, y);
  return l;
}

// ข้อความกึ่งกลางในช่วงกว้าง w เริ่มที่ x
lv_obj_t *lvTextC(int x, int w, int y, const String &s, const lv_font_t *f, uint16_t col) {
  lv_obj_t *l = lvText(x, y, s, f, col);
  lv_label_set_long_mode(l, LV_LABEL_LONG_CLIP);
  lv_obj_set_width(l, w);
  lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
  return l;
}

// ข้อความชิดขวา ปลายขวาอยู่ที่ xr
lv_obj_t *lvTextR(int xr, int y, const String &s, const lv_font_t *f, uint16_t col) {
  int w = (xr < 200) ? xr : 200;
  lv_obj_t *l = lvText(xr - w, y, s, f, col);
  lv_label_set_long_mode(l, LV_LABEL_LONG_CLIP);
  lv_obj_set_width(l, w);
  lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_RIGHT, 0);
  return l;
}

lv_obj_t *lvBar(int x, int y, int w, int h, int pct, uint16_t fill) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  lv_obj_t *b = lv_bar_create(lvRoot);
  lvPlain(b);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_size(b, w, h);
  lv_bar_set_range(b, 0, 100);
  lv_bar_set_value(b, pct, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(b, lvc(C_BAR), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(b, h / 2, LV_PART_MAIN);
  lv_obj_set_style_bg_color(b, lvc(fill), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_radius(b, h / 2, LV_PART_INDICATOR);
  return b;
}

// ปุ่ม (วาดอย่างเดียว การแตะตรวจที่ dispatchTap ด้วยโซนเดียวกัน)
void lvButton(const TouchZone &z, const String &label, uint16_t bg, uint16_t fg, const lv_font_t *f) {
  lvBox(z.x, z.y, z.w, z.h, bg, 6);
  lv_obj_t *t = lvTextC(z.x, z.w, z.y + (z.h - lv_font_get_line_height(f)) / 2, label, f, fg);
  (void)t;
}

// ล้างหน้าจอแล้วเริ่มวาดหน้าใหม่ด้วยสีพื้น bg
void lvClear(uint16_t bg) {
  lvDrop = nullptr;
  lvPoFill = nullptr;
  lv_obj_clean(lvRoot);
  lv_obj_set_style_bg_color(lvRoot, lvc(bg), 0);
  lv_obj_set_style_bg_opa(lvRoot, LV_OPA_COVER, 0);
}

// ต้องสร้างหน้าใหม่ไหม: หน้าเพิ่งเปลี่ยน (force) หรือค่าที่แสดงเปลี่ยน (key ต่าง)
bool lvNeed(bool force, const String &key) {
  if (force || key != lvKey) {
    lvKey = key;
    return true;
  }
  return false;
}

// สั่งให้ LVGL วาดทันที (ใช้ตอนไม่ได้อยู่ใน loop เช่นหน้าต้อนรับและหน้าปิดเครื่อง)
void lvRefreshNow() {
  lv_refr_now(NULL);
}

// ============================================================================
// เริ่ม LVGL: ผูกกับจอ TFT_eSPI และทัช XPT2046
// ============================================================================
static uint32_t lvTickCb() { return millis(); }

static void lvFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
  uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)px_map, w * h, true);   // true = สลับไบต์ให้ตรงลำดับที่จอต้องการ
  tft.endWrite();
  lv_display_flush_ready(disp);
}

static void lvTouchRead(lv_indev_t *indev, lv_indev_data_t *data) {
  (void)indev;
  data->state = LV_INDEV_STATE_RELEASED;
  if (uiScreen == SCR_CALIB || isPowerOffProgressActive) return;   // หน้า CALIB อ่านค่าดิบเอง
  if (ts.tirqTouched() && ts.touched()) {
    TS_Point p = ts.getPoint();
    if (p.z >= TOUCH_MIN_PRESSURE) {
      int tx, ty;
      mapTouch(p.x, p.y, tx, ty);
      data->point.x = tx;
      data->point.y = ty;
      data->state = LV_INDEV_STATE_PRESSED;
      lastUserActivityTime = millis();
    }
  }
}

// LVGL รับการคลิกที่หน้าจอราก (วิตเจ็ตลูกไม่รับการแตะ) แล้วส่งพิกัดให้ตัวจัดการของหน้านั้น
static void lvRootClicked(lv_event_t *e) {
  (void)e;
  lv_point_t p;
  lv_indev_get_point(lv_indev_active(), &p);
  dispatchTap(p.x, p.y);
}

void lvglBegin() {
  lv_init();
  lv_tick_set_cb(lvTickCb);

  lv_display_t *disp = lv_display_create(SCR_W, SCR_H);
  lv_display_set_flush_cb(disp, lvFlush);
  lv_display_set_buffers(disp, lvBuf, NULL, sizeof(lvBuf), LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, lvTouchRead);

  lvRoot = lv_obj_create(NULL);
  lvPlain(lvRoot);
  lv_obj_set_style_pad_all(lvRoot, 0, 0);
  lv_obj_set_style_border_width(lvRoot, 0, 0);
  lv_obj_set_style_radius(lvRoot, 0, 0);
  lv_obj_add_event_cb(lvRoot, lvRootClicked, LV_EVENT_CLICKED, NULL);
  lv_obj_add_flag(lvRoot, LV_OBJ_FLAG_CLICKABLE);   // เฉพาะหน้าจอรากที่รับการแตะ
  lv_screen_load(lvRoot);
}

// เรียกทุกรอบ loop
void lvglService() {
  lv_timer_handler();
}

// ============================================================================
// แถบบนสุดที่ใช้ร่วมกันทุกหน้า: นาฬิกา สัญญาณ Wi-Fi แบตเตอรี่
// ============================================================================
String lvStatusKey() {
  return getDisplayClockStr() + "|" + String(WiFi.softAPgetStationNum()) + "|" + String(hostBatteryPct / 5);
}

void lvStatusBar() {
  lvBox(0, 0, SCR_W, TOPBAR_H, C_TOPBAR, 0);
  lvText(6, 4, getDisplayClockStr(), F_XS, isTimeApprox ? C_YELLOW : C_WHITE);
  int clients = (int)WiFi.softAPgetStationNum();
  const int hh[4] = {3, 6, 9, 12};
  for (int i = 0; i < 4; i++)
    lvBox(SCR_W - 50 + i * 4, 5 + (12 - hh[i]), 3, hh[i], (clients > i) ? C_GREEN : C_LINE, 0);
  int pct = hostBatteryPct;
  lvRing(SCR_W - 28, 6, 20, 10, C_DIM, 2);
  lvBox(SCR_W - 8, 9, 2, 4, C_DIM, 0);
  uint16_t c = (pct <= 20) ? C_RED : (pct <= 50 ? C_YELLOW : C_GREEN);
  int w = (16 * pct) / 100;
  if (w > 0) lvBox(SCR_W - 26, 8, w, 6, c, 0);
}

void lvTitleBar(const String &title) {
  lvBox(0, TOPBAR_H, SCR_W, 36, C_CARD, 0);
  lvButton(ZONE_BACK, "<", C_BTN, C_WHITE, F_M);
  lvTextC(52, SCR_W - 104, 32, title, F_M, C_WHITE);
}

// ============================================================================
// หน้า HOME
// ============================================================================
void lvHomeBottomText(String &line, uint16_t &bg, uint16_t &fg) {
  int alarm = -1, near = -1, online = 0;
  for (int i = 0; i < activeStationCount; i++) {
    if (alarm < 0 && bedIsAlarm(bedUiStatus(i))) alarm = i;
    if (near < 0 && stations[i].alertCode == ALERT_NEAR_END && !stations[i].nearEndAck) near = i;
    if (isStationOnline(i)) online++;
  }
  char buf[40];
  if (alarm >= 0) {
    snprintf(buf, sizeof(buf), "! BED %02d %s", alarm + 1, alertTextEn(stations[alarm].alertCode));
    bg = C_RED; fg = C_WHITE;
  } else if (anyIdConflict()) {
    int dup = -1;
    for (int i = 0; i < activeStationCount; i++) if (stations[i].idConflict) { dup = i; break; }
    snprintf(buf, sizeof(buf), "! BED %02d ID CLASH", dup + 1);
    bg = C_RED; fg = C_WHITE;
  } else if (heardBeyondBedCount() > 0) {
    snprintf(buf, sizeof(buf), "HEARD BED %02d NOT ON", heardBeyondBedCount());
    bg = C_ORANGE; fg = C_BG;
  } else if (near >= 0) {
    snprintf(buf, sizeof(buf), "BED %02d NEXT BAG", near + 1);
    bg = C_ORANGE; fg = C_BG;
  } else {
    snprintf(buf, sizeof(buf), "ALL NORMAL  %d/%d", online, activeStationCount);
    bg = C_CARD; fg = C_GREEN;
  }
  line = String(buf);
}

void drawHomeScreen(bool force) {
  int focus = currentFocusBed();
  BedUiStatus st = bedUiStatus(focus);
  const StationData &s = stations[focus];
  uint16_t sc = bedColor(st);

  String bottom; uint16_t bbg, bfg;
  lvHomeBottomText(bottom, bbg, bfg);

  // กุญแจ: ทุกค่าที่หน้านี้แสดง ถ้าไม่เปลี่ยนก็ไม่ต้องสร้างใหม่
  String key = lvStatusKey() + "|" + String(focus) + ":" + String((int)st) + ":" +
               String((int)(s.flowRate_ml_hr + 0.5f)) + ":" + String((int)s.cfg.targetRateHr) + ":" +
               String(bedPct(focus)) + ":" + String(bedMinutesLeft(focus)) + ":" + String((int)s.totalVolumeMl) +
               "|" + bottom + "|" + String(activeStationCount);
  for (int i = 0; i < activeStationCount; i++)
    key += "|" + String((int)bedUiStatus(i)) + ":" + String((int)(stations[i].flowRate_ml_hr + 0.5f));
  if (!lvNeed(force, key)) return;

  lvClear(C_BG);
  lvStatusBar();

  // ---- แผงเตียงที่ต้องดู ----
  char head[16];
  snprintf(head, sizeof(head), "BED %02d", focus + 1);
  lvBox(FOCUS_X, FOCUS_Y, FOCUS_W, FOCUS_H, C_CARD, 8);
  lvBox(FOCUS_X, FOCUS_Y, FOCUS_W, 26, sc, 8);
  lvBox(FOCUS_X, FOCUS_Y + 18, FOCUS_W, 8, sc, 0);
  lvText(FOCUS_X + 8, FOCUS_Y + 3, String(head), F_M, C_BG);
  lvTextR(FOCUS_X + FOCUS_W - 8, FOCUS_Y + 8, String(bedWord(st)), F_XS, C_BG);

  String rateStr = (st == BU_OFFLINE || st == BU_PAUSED) ? String("--") : String((int)(s.flowRate_ml_hr + 0.5f));
  lvText(FOCUS_X + 8, 60, rateStr, F_XL, bedIsAlarm(st) ? sc : C_WHITE);
  lvText(FOCUS_X + 8, 120, "mL/h", F_XS, C_DIM);
  String setStr = (s.cfg.targetRateHr > 0) ? ("SET " + String((int)(s.cfg.targetRateHr + 0.5f))) : String("SET --");
  lvTextR(FOCUS_X + FOCUS_W - 8, 120, setStr, F_XS, C_SKY);

  int pct = bedPct(focus);
  lvBar(FOCUS_X + 8, 140, FOCUS_W - 16, 12, (pct < 0) ? 0 : pct, (st == BU_NEAREND) ? C_ORANGE : C_CYAN);

  int mins = bedMinutesLeft(focus);
  char tl[16];
  if (mins >= 0) snprintf(tl, sizeof(tl), "%dh %02dm", mins / 60, mins % 60);
  else           snprintf(tl, sizeof(tl), "--h --m");
  char vol[28];
  if (s.cfg.planVolumeMl > 0) snprintf(vol, sizeof(vol), "%d/%d mL", (int)s.totalVolumeMl, (int)s.cfg.planVolumeMl);
  else                        snprintf(vol, sizeof(vol), "%d mL", (int)s.totalVolumeMl);
  lvText(FOCUS_X + 8, 160, String(vol), F_XS, C_DIM);
  lvText(FOCUS_X + 8, 172, String(tl), F_L, bedIsAlarm(st) ? sc : C_WHITE);

  // ---- รายการเตียงอื่น (ตัวแปร homeRowBed/homeRowCount/homeRowH ใช้ตอนแตะ) ----
  int n = activeStationCount - 1;
  homeRowCount = 0;
  if (n > 0) {
    int gap = 4;
    int rowH = (LIST_H - gap * (n - 1)) / n;
    if (rowH > 40) rowH = 40;
    if (rowH < 22) rowH = 22;
    homeRowH = rowH;
    int slot = 0;
    for (int i = 0; i < activeStationCount; i++) {
      if (i == focus) continue;
      int y = LIST_Y + slot * (rowH + gap);
      if (y + rowH > LIST_Y + LIST_H) break;
      homeRowBed[homeRowCount++] = i;

      BedUiStatus rs = bedUiStatus(i);
      const StationData &rd = stations[i];
      bool dim = (rs == BU_OFFLINE);
      uint16_t card = dim ? C_CARD2 : C_CARD;
      uint16_t rc = bedColor(rs);
      lvBox(LIST_X, y, LIST_W, rowH, card, 5);
      lvBox(LIST_X, y, 5, rowH, rc, 0);
      char bed[8];
      snprintf(bed, sizeof(bed), "B%02d", i + 1);
      String rr;
      if (rs == BU_OFFLINE)     rr = "OFF";
      else if (rs == BU_PAUSED) rr = "||";
      else                      rr = String((int)(rd.flowRate_ml_hr + 0.5f));
      int ty = y + (rowH - 19) / 2;
      lvText(LIST_X + 12, ty, String(bed), F_S, dim ? C_DIM : C_WHITE);
      lvTextR(LIST_X + LIST_W - 40, ty, rr, F_S, bedIsAlarm(rs) ? rc : (dim ? C_DIM : C_WHITE));
      lvTextR(LIST_X + LIST_W - 10, ty, ">", F_S, C_DIM);
      slot++;
    }
  }

  // ---- แถบล่าง ----
  lvBox(ZONE_ALERT.x, ZONE_ALERT.y, ZONE_ALERT.w, ZONE_ALERT.h, bbg, 6);
  lvTextC(ZONE_ALERT.x, ZONE_ALERT.w, ZONE_ALERT.y + (ZONE_ALERT.h - 14) / 2, bottom, F_XS, bfg);
  lvButton(ZONE_GEAR, "SET", C_BTN, C_WHITE, F_XS);
}

// ============================================================================
// หน้า BED
// ============================================================================
void drawBedScreen(bool force) {
  int i = uiSelectedBed;
  BedUiStatus st = bedUiStatus(i);
  const StationData &s = stations[i];
  uint16_t sc = bedColor(st);
  int pct = bedPct(i);
  int mins = bedMinutesLeft(i);

  String diag;
  uint16_t diagCol = C_DIM;
  if (!isStationOnline(i)) {
    diag = "LINK --";
  } else if (s.idConflict) {
    diag = "BED ID CLASH " + macTail(s);
    diagCol = C_RED;
  } else {
    diag = "LINK " + String(s.linkPct) + "%  " + macTail(s);
    diagCol = (s.linkPct >= 90) ? C_GREEN : (s.linkPct >= LINK_WEAK_PCT ? C_SKY : C_RED);
  }

  String key = lvStatusKey() + "|" + String(i) + ":" + String((int)st) + ":" + String((int)(s.flowRate_ml_hr + 0.5f)) + ":" +
               String((int)s.cfg.targetRateHr) + ":" + String(pct) + ":" + String(mins) + ":" + String((int)s.totalVolumeMl) +
               ":" + String((int)s.cfg.planVolumeMl) + "|" + diag + "|" + String(s.cfg.patientName);

  if (lvNeed(force, key)) {
    lvClear(C_BG);
    lvStatusBar();
    char title[16];
    snprintf(title, sizeof(title), "BED %02d", i + 1);
    lvTitleBar(String(title));

    lvBox(6, 62, 308, 96, C_CARD, 8);
    lvBox(6, 164, 308, 46, C_CARD, 8);
    String name = String(s.cfg.patientName);
    if (name.length() > 14) name = name.substring(0, 14);
    lvText(14, 68, name, F_XS, C_DIM);
    lvBox(222, 66, 86, 20, sc, 5);
    lvTextC(222, 86, 69, String(bedWord(st)), F_XS, C_BG);

    String rateStr = (st == BU_OFFLINE || st == BU_PAUSED) ? String("--") : String((int)(s.flowRate_ml_hr + 0.5f));
    lvText(14, 84, rateStr, F_XL, bedIsAlarm(st) ? sc : C_WHITE);
    lvText(14, 142, "mL/h", F_XS, C_DIM);
    lvTextR(306, 142, s.cfg.targetRateHr > 0 ? ("SET " + String((int)s.cfg.targetRateHr) + " mL/h") : String("SET --"), F_XS, C_SKY);
    lvText(186, 112, diag, F_XS, diagCol);

    // กระเปาะหยด (ตัวหยดขยับทุกรอบด้านล่าง ไม่ต้องสร้างหน้าใหม่)
    lvRing(138, 90, 42, 46, C_DIM, 4);
    lvBox(138 + 21 - 2, 92, 4, 5, C_SKY, 0);
    lvBox(141, 90 + 46 - 7, 36, 5, C_SKY, 2);
    lvDrop = lvBox(138 + 21 - 2, 98, 5, 5, C_CYAN, 2);

    lvText(14, 170, "INFUSED", F_XS, C_DIM);
    lvTextR(306, 170, pct >= 0 ? (String(pct) + "%") : String("--"), F_XS, C_WHITE);
    lvBar(14, 182, 292, 10, (pct < 0) ? 0 : pct, (st == BU_NEAREND) ? C_ORANGE : C_CYAN);
    char sub[44];
    if (mins >= 0) snprintf(sub, sizeof(sub), "%d/%d mL   %dh %02dm left", (int)s.totalVolumeMl, (int)s.cfg.planVolumeMl, mins / 60, mins % 60);
    else           snprintf(sub, sizeof(sub), "%d/%d mL   --h --m left", (int)s.totalVolumeMl, (int)s.cfg.planVolumeMl);
    lvText(14, 196, String(sub), F_XS, C_DIM);

    lvButton(ZONE_BED_NEWBAG, "NEW BAG", C_BTN, C_WHITE, F_XS);
    lvButton(ZONE_BED_ACK, "ACK", C_BTN, C_WHITE, F_XS);
    lvButton(ZONE_BED_SET, "SET", C_BTN, C_WHITE, F_XS);
  }

  // หยดน้ำเกลือขยับทุกรอบ ตามเฟสที่ล็อกกับหยดจริง (ไม่มีหยดแล้วซ่อนไว้)
  if (lvDrop) {
    int ph = dripAnimPhase(i);
    if (ph >= 0) {
      int top = 98, bot = 90 + 46 - 7 - 3;
      lv_obj_set_y(lvDrop, top + ((bot - top) * ph) / 100);
      lv_obj_remove_flag(lvDrop, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(lvDrop, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

// ============================================================================
// หน้า BED SET / SYSTEM ใช้ช่องตั้งค่าแบบเดียวกัน (ป้าย ค่า หน่วย ปุ่ม - +)
// ============================================================================
void lvSettingCell(int x, int y, const char *label, const String &value, const char *unit) {
  lvBox(x, y, SET_ROW_W, SET_ROW_H, C_CARD, 7);
  lvText(x + 8, y + 4, String(label), F_XS, C_DIM);
  lvBox(x + SET_M_X, y + SET_BTN_Y, SET_M_W, SET_BTN_H, C_BTN, 6);
  lvTextC(x + SET_M_X, SET_M_W, y + SET_BTN_Y + 5, "-", F_M, C_WHITE);
  lvBox(x + SET_P_X, y + SET_BTN_Y, SET_P_W, SET_BTN_H, C_BTN, 6);
  lvTextC(x + SET_P_X, SET_P_W, y + SET_BTN_Y + 5, "+", F_M, C_WHITE);
  lvText(x + 8, y + 16, value, F_M, C_WHITE);
  lvText(x + 8, y + 36, String(unit), F_XS, C_DIM);
}

void drawBedSetScreen(bool force) {
  String key = lvStatusKey() + "|" + String(uiSelectedBed) + ":" + String((int)editTargetRate) + ":" +
               String((int)editPlanVolume) + ":" + String((int)editDropFactor) + ":" + String((int)editNearPct);
  if (!lvNeed(force, key)) return;
  lvClear(C_BG);
  lvStatusBar();
  char title[16];
  snprintf(title, sizeof(title), "SET B%02d", uiSelectedBed + 1);
  lvTitleBar(String(title));
  lvButton(ZONE_SET_SAVE, "SAVE", C_GREEN, C_BG, F_M);
  lvButton(ZONE_SET_CANCEL, "CANCEL", C_BTN, C_WHITE, F_M);
  lvSettingCell(SET_ROW_X[0], SET_ROW_Y[0], "TARGET RATE", String((int)editTargetRate), "mL/h");
  lvSettingCell(SET_ROW_X[1], SET_ROW_Y[1], "PLAN VOLUME", String((int)editPlanVolume), "mL");
  lvSettingCell(SET_ROW_X[2], SET_ROW_Y[2], "DROP FACTOR", String((int)editDropFactor), "gtt/mL");
  lvSettingCell(SET_ROW_X[3], SET_ROW_Y[3], "NEXT BAG AT", String((int)editNearPct), "%");
}

// ============================================================================
// หน้า NUMPAD
// ============================================================================
void drawNumpadScreen(bool force) {
  String v = (editNumBuf[0] == '\0') ? String("_") : String(editNumBuf);
  String key = lvStatusKey() + "|" + v + "|" + String(editNumLabel);
  if (!lvNeed(force, key)) return;
  lvClear(C_BG);
  lvStatusBar();
  lvTitleBar("ENTER");
  lvBox(6, 62, 102, 72, C_CARD, 7);
  lvText(14, 66, String(editNumLabel), F_XS, C_DIM);
  lvText(14, 86, v, F_L, C_WHITE);
  lvText(14, 120, String(editNumUnit), F_XS, C_DIM);
  const char *keys[12] = { "1","2","3","4","5","6","7","8","9","C","0","OK" };
  for (int i = 0; i < 12; i++) {
    TouchZone z = numpadKeyZone(i);
    bool ok = (i == 11), clr = (i == 9);
    lvButton(z, String(keys[i]), ok ? C_GREEN : (clr ? C_ORANGE : C_BTN), (ok || clr) ? C_BG : C_WHITE, F_M);
  }
}

// ============================================================================
// หน้า SYSTEM
// ============================================================================
void drawSysScreen(bool force) {
  String devTxt = String((int)WiFi.softAPgetStationNum()) + " dev";
  String rtcTxt = rtcStatusText();
  String sdTxt  = sdStatusText();
  String key = lvStatusKey() + "|" + String(activeStationCount) + ":" + String(screenBrightness) + "|" + devTxt + "|" + rtcTxt + "|" + sdTxt;
  if (!lvNeed(force, key)) return;
  lvClear(C_BG);
  lvStatusBar();
  lvTitleBar("SETTINGS");
  lvButton(ZONE_SYS_CALIB, "TOUCH CALIBRATION", C_BTN, C_WHITE, F_XS);
  lvSettingCell(SYS_CELL_X[0], SYS_CELL_Y[0], "ACTIVE BEDS", String(activeStationCount), "beds");
  lvSettingCell(SYS_CELL_X[1], SYS_CELL_Y[1], "BRIGHTNESS", String(screenBrightness), "%");

  lvBox(6, 154, 308, 82, C_CARD, 7);
  lvText(14, 160, "WI-FI / WEB", F_XS, C_DIM);
  lvText(164, 160, "CLOCK / SD", F_XS, C_DIM);
  lvTextR(306, 160, "v" APP_VERSION, F_XS, C_DIM);
  lvText(14, 176, String(default_ap_ssid), F_XS, C_WHITE);
  lvText(14, 192, "pass " + String(default_ap_pass), F_XS, C_DIM);
  lvText(14, 208, "http://" + WiFi.softAPIP().toString(), F_XS, C_SKY);
  lvBox(156, 160, 1, 70, C_DIM, 0);
  lvText(164, 176, "RTC " + rtcTxt, F_XS, (rtcPresent && rtcTimeOk) ? C_GREEN : C_YELLOW);
  lvText(164, 192, "SD  " + sdTxt, F_XS, sdPresent ? C_GREEN : C_YELLOW);
  lvText(164, 208, devTxt, F_XS, C_DIM);
}

// ============================================================================
// หน้า ALARM: เตือนเต็มจอ แตะที่ไหนก็ได้เพื่อพักเสียง
// ============================================================================
void drawAlarmScreen(bool force) {
  int bed = autoFocusBed();
  if (bed < 0) bed = 0;
  const StationData &s = stations[bed];
  uint8_t code = s.alertCode;
  String rate = String((int)(s.flowRate_ml_hr + 0.5f));
  String key = String(bed) + ":" + String(code) + ":" + rate + ":" + String((int)s.cfg.targetRateHr);
  if (!lvNeed(force, key)) return;

  lvClear(C_RED);
  const char *sub = "CHECK IV SYSTEM";
  if (code == ALERT_OCCLUSION)     sub = "LINE BLOCKED - CHECK TUBING";
  else if (code == ALERT_TOO_FAST) sub = "FLOW ABOVE TARGET";
  else if (code == ALERT_TOO_SLOW) sub = "FLOW BELOW TARGET";
  else if (code == ALERT_COMPLETE) sub = "INFUSION FINISHED";

  char head[16];
  snprintf(head, sizeof(head), "BED %02d", bed + 1);
  lvBox(10, 10, 300, 42, C_WHITE, 8);
  lvTextC(10, 300, 13, String(head), F_L, C_RED);
  lvTextC(0, SCR_W, 56, String(alertTextEn(code)), F_L, C_WHITE);
  lvTextC(0, SCR_W, 90, String(sub), F_XS, C_YELLOW);

  lvBox(16, 108, 288, 66, C_BG, 8);
  lvText(28, 112, "RATE", F_XS, C_DIM);
  lvText(28, 124, rate, F_L, C_WHITE);
  lvText(28, 158, "mL/h", F_XS, C_DIM);
  lvTextR(292, 158, s.cfg.targetRateHr > 0 ? ("SET " + String((int)s.cfg.targetRateHr) + " mL/h") : String("SET --"), F_XS, C_SKY);

  lvBox(16, 182, 288, 48, C_WHITE, 10);
  lvTextC(16, 288, 192, "TAP = SNOOZE 2 MIN", F_XS, C_RED);
  lvTextC(16, 288, 210, "BUTTON = SILENCE", F_XS, C_RED);
}

// ============================================================================
// หน้า CALIB: แตะกากบาท 2 จุด (ค่าดิบอ่านโดย serviceCalibScreen ในไฟล์หลัก)
// ============================================================================
void drawCalibScreen(bool force) {
  String key = String(calibStep);
  if (!lvNeed(force, key)) return;
  lvClear(C_BG);
  lvTextC(0, SCR_W, 90, "TOUCH CALIBRATION", F_M, C_WHITE);
  lvTextC(0, SCR_W, 120, calibStep == 0 ? "tap the cross (1 of 2)" : "tap the cross (2 of 2)", F_XS, C_DIM);
  lvTextC(0, SCR_W, 140, "use a stylus for best accuracy", F_XS, C_DIM);
  int x = CALIB_PT[calibStep][0], y = CALIB_PT[calibStep][1];
  lvBox(x - 12, y, 25, 1, C_YELLOW, 0);
  lvBox(x, y - 12, 1, 25, C_YELLOW, 0);
  lvRing(x - 8, y - 8, 17, 17, C_YELLOW, 8);
}

// ข้อความบันทึกค่าปรับทัชแล้ว (แสดงสั้น ๆ ก่อนกลับหน้า SYSTEM)
void drawCalibSaved() {
  lvClear(C_BG);
  lvTextC(0, SCR_W, SCR_H / 2 - 20, "CALIBRATION SAVED", F_M, C_GREEN);
  lvRefreshNow();
}

// ---- หน้าจอเฉพาะกิจ: ปิดเครื่อง / ลาก่อน / ต้อนรับ / จอดับ ----
void drawPowerOffProgress(int pct) {
  const int barX = (SCR_W - 204) / 2;
  if (uiScreenDrawn != SCR_POWER) {
    uiScreenDrawn = SCR_POWER;
    lvClear(C_BG);
    lvTextC(0, SCR_W, 60, "POWER OFF ?", F_L, C_WHITE);
    lvTextC(0, SCR_W, 100, "KEEP HOLDING TO TURN OFF", F_XS, C_DIM);
    lvRing(barX, 128, 204, 24, C_WHITE, 6);
    lvPoFill = lvBox(barX + 2, 130, 1, 20, C_RED, 0);
  }
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  int w = (200 * pct) / 100;
  if (lvPoFill) lv_obj_set_width(lvPoFill, w > 0 ? w : 1);
  lvRefreshNow();
}

void drawGoodbyeScreen() {
  uiScreenDrawn = SCR_POWER;
  lvClear(C_BG);
  lvTextC(0, SCR_W, 70, "GOODBYE", F_XL, C_SKY);
  lvTextC(0, SCR_W, 130, "RELEASE BUTTON TO TURN OFF", F_XS, C_DIM);
  lvTextC(0, SCR_W, 150, "HOLD 2s TO POWER ON AGAIN", F_XS, C_DIM);
  lvRefreshNow();
}

void setDisplaySleep(bool sleep) {
  displaySleeping = sleep;
  if (sleep) {
    setBacklightRaw(0);
    lvClear(C_BG);
    lvRefreshNow();
  } else {
    setBacklightRaw(screenBrightness);
    uiScreenDrawn = (UiScreen)-1;
    uiNeedFramework = true;
    resetUiCaches();
  }
}

void drawSplashScreen() {
  lvClear(C_BG);
  lvBox(60, 20, 200, 96, C_TOPBAR, 10);
  lvTextC(60, 200, 30, "SMART IV", F_L, C_WHITE);
  lvTextC(60, 200, 76, "CENTRAL HOST", F_M, C_SKY);
  lvTextC(0, SCR_W, 128, "Host v" APP_VERSION, F_XS, C_DIM);
  char buf[40];
  snprintf(buf, sizeof(buf), "BEDS %d   TOUCH SCREEN", activeStationCount);
  lvTextC(0, SCR_W, 146, String(buf), F_XS, C_DIM);
  lvTextC(0, SCR_W, 172, String(default_ap_ssid), F_XS, C_WHITE);
  lvTextC(0, SCR_W, 200, "BCN Phrae Innovation", F_XS, C_GREEN);
  lvRefreshNow();
}
