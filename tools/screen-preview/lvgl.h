#pragma once
// ตัวแทน (mock) ของ LVGL 9.x สำหรับตรวจคอมไพล์และเรนเดอร์ภาพจำลองบนเครื่อง PC
// เขียนตามความเข้าใจ API ของ LVGL 9.x เฉพาะชุดฟังก์ชันที่เฟิร์มแวร์ Host-CYD-LVGL ใช้ ไม่ใช่ LVGL จริง
// เก็บต้นไม้วิตเจ็ต (ตำแหน่ง สี ข้อความ) แล้วส่งออกเป็นคำสั่งวาดชุดเดียวกับ Adafruit_GFX ให้ render.py เรนเดอร์
// ข้อจำกัด: ฟอนต์ Montserrat ถูกประมาณด้วยฟอนต์บิตแมป 5x7 ขยายเป็นจำนวนเต็มเท่า จึงใช้ตรวจผัง (ตำแหน่ง ซ้อนทับ สี)
// ไม่ใช่ตรวจหน้าตาตัวอักษร ผลจริงบนบอร์ดจะต่างเล็กน้อย
#include "Arduino.h"
#include <vector>
#include <string>
#include <cstdio>
#include <cstdint>

extern FILE* g_ops;

struct lv_color_t { uint8_t r, g, b; };
inline lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b) { return lv_color_t{r, g, b}; }
struct lv_font_t { int px; };
inline const lv_font_t lv_font_montserrat_12 = {12};
inline const lv_font_t lv_font_montserrat_16 = {16};
inline const lv_font_t lv_font_montserrat_20 = {20};
inline const lv_font_t lv_font_montserrat_28 = {28};
inline const lv_font_t lv_font_montserrat_48 = {48};
inline int lv_font_get_line_height(const lv_font_t* f) { return f->px + 3; }

typedef int lv_part_t;
#define LV_PART_MAIN 0
#define LV_PART_INDICATOR 0x20000
#define LV_OPA_COVER 255
#define LV_OPA_TRANSP 0
#define LV_ANIM_OFF 0
#define LV_LABEL_LONG_CLIP 3
#define LV_TEXT_ALIGN_LEFT 1
#define LV_TEXT_ALIGN_CENTER 2
#define LV_TEXT_ALIGN_RIGHT 3
#define LV_OBJ_FLAG_CLICKABLE 1
#define LV_OBJ_FLAG_SCROLLABLE 2
#define LV_OBJ_FLAG_HIDDEN 4
#define LV_EVENT_CLICKED 7
#define LV_INDEV_STATE_RELEASED 0
#define LV_INDEV_STATE_PRESSED 1
#define LV_INDEV_TYPE_POINTER 1
#define LV_DISPLAY_RENDER_MODE_PARTIAL 0

struct lv_obj_t;
struct lv_event_t { lv_obj_t* target; };
typedef void (*lv_event_cb_t)(lv_event_t*);
struct lv_obj_t {
  lv_obj_t* parent = nullptr;
  std::vector<lv_obj_t*> kids;
  int kind = 0;               // 0 กล่อง 1 ข้อความ 2 แถบ
  int x = 0, y = 0, w = 0, h = 0, radius = 0, border = 0;
  lv_color_t bg{0, 0, 0}, bdr{0, 0, 0}, tcol{255, 255, 255}, ind{0, 0, 0};
  int bgopa = 255, flags = 0, align = 1, px = 12, pct = 0;
  bool autoW = true;
  std::string text;
  lv_event_cb_t cb = nullptr;
};
struct lv_area_t { int x1, y1, x2, y2; };
struct lv_point_t { int x, y; };
struct lv_display_t { int w, h; };
struct lv_indev_t { int dummy; };
struct lv_indev_data_t { lv_point_t point; int state; };
typedef void (*lv_display_flush_cb_t)(lv_display_t*, const lv_area_t*, uint8_t*);
typedef void (*lv_indev_read_cb_t)(lv_indev_t*, lv_indev_data_t*);
typedef uint32_t (*lv_tick_get_cb_t)(void);

inline lv_obj_t* g_lv_screen = nullptr;
inline lv_point_t g_lv_point{0, 0};
inline lv_indev_t g_lv_indev;
inline lv_indev_read_cb_t g_lv_read = nullptr;

inline void lv_init() {}
inline void lv_tick_set_cb(lv_tick_get_cb_t) {}
inline lv_display_t* lv_display_create(int w, int h) { return new lv_display_t{w, h}; }
inline void lv_display_set_flush_cb(lv_display_t*, lv_display_flush_cb_t) {}
inline void lv_display_set_buffers(lv_display_t*, void*, void*, uint32_t, int) {}
inline void lv_display_flush_ready(lv_display_t*) {}
inline lv_indev_t* lv_indev_create() { return &g_lv_indev; }
inline void lv_indev_set_type(lv_indev_t*, int) {}
inline void lv_indev_set_read_cb(lv_indev_t*, lv_indev_read_cb_t cb) { g_lv_read = cb; }
inline lv_indev_t* lv_indev_active() { return &g_lv_indev; }
inline void lv_indev_get_point(lv_indev_t*, lv_point_t* p) { *p = g_lv_point; }
inline void lv_timer_handler() {}

inline lv_obj_t* lv_obj_create(lv_obj_t* parent) {
  lv_obj_t* o = new lv_obj_t();
  o->parent = parent; o->flags = LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE;
  if (parent) parent->kids.push_back(o);
  return o;
}
inline lv_obj_t* lv_label_create(lv_obj_t* parent) { lv_obj_t* o = lv_obj_create(parent); o->kind = 1; o->bgopa = 0; return o; }
inline lv_obj_t* lv_bar_create(lv_obj_t* parent) { lv_obj_t* o = lv_obj_create(parent); o->kind = 2; return o; }
inline void lv_screen_load(lv_obj_t* o) { g_lv_screen = o; }
inline void lv_obj_clean(lv_obj_t* o) { o->kids.clear(); }
inline void lv_obj_remove_flag(lv_obj_t* o, int f) { o->flags &= ~f; }
inline void lv_obj_add_flag(lv_obj_t* o, int f) { o->flags |= f; }
inline void lv_obj_set_pos(lv_obj_t* o, int x, int y) { o->x = x; o->y = y; }
inline void lv_obj_set_y(lv_obj_t* o, int y) { o->y = y; }
inline void lv_obj_set_size(lv_obj_t* o, int w, int h) { o->w = w; o->h = h; o->autoW = false; }
inline void lv_obj_set_width(lv_obj_t* o, int w) { o->w = w; o->autoW = false; }
inline void lv_obj_set_style_bg_color(lv_obj_t* o, lv_color_t c, int part) { if (part == LV_PART_INDICATOR) o->ind = c; else o->bg = c; }
inline void lv_obj_set_style_bg_opa(lv_obj_t* o, int v, int part) { if (part != LV_PART_INDICATOR) o->bgopa = v; }
inline void lv_obj_set_style_radius(lv_obj_t* o, int r, int part) { if (part != LV_PART_INDICATOR) o->radius = r; }
inline void lv_obj_set_style_border_width(lv_obj_t* o, int w, int) { o->border = w; }
inline void lv_obj_set_style_border_color(lv_obj_t* o, lv_color_t c, int) { o->bdr = c; }
inline void lv_obj_set_style_pad_all(lv_obj_t*, int, int) {}
inline void lv_obj_set_style_text_font(lv_obj_t* o, const lv_font_t* f, int) { o->px = f->px; }
inline void lv_obj_set_style_text_color(lv_obj_t* o, lv_color_t c, int) { o->tcol = c; }
inline void lv_obj_set_style_text_align(lv_obj_t* o, int a, int) { o->align = a; }
inline void lv_label_set_text(lv_obj_t* o, const char* t) { o->text = t; }
inline void lv_label_set_long_mode(lv_obj_t*, int) {}
inline void lv_bar_set_range(lv_obj_t*, int, int) {}
inline void lv_bar_set_value(lv_obj_t* o, int v, int) { o->pct = v; }
inline void lv_obj_add_event_cb(lv_obj_t* o, lv_event_cb_t cb, int, void*) { o->cb = cb; }

// ---- ส่งออกเป็นคำสั่งวาด ----
inline uint16_t lvm565(lv_color_t c) { return (uint16_t)(((c.r >> 3) << 11) | ((c.g >> 2) << 5) | (c.b >> 3)); }
inline int lvm_size(int px) { return px <= 14 ? 1 : (px <= 22 ? 2 : (px <= 32 ? 3 : 6)); }
inline void lvm_flat(lv_obj_t* o, int ox, int oy, std::vector<std::pair<lv_obj_t*, std::pair<int, int>>>& out) {
  for (lv_obj_t* k : o->kids) {
    out.push_back({k, {ox + k->x, oy + k->y}});
    lvm_flat(k, ox + k->x, oy + k->y, out);
  }
}
inline uint16_t lvm_bg_at(const std::vector<std::pair<lv_obj_t*, std::pair<int, int>>>& all, size_t upto, int px, int py, uint16_t dflt) {
  uint16_t c = dflt;
  for (size_t i = 0; i < upto; i++) {
    lv_obj_t* o = all[i].first; int ox = all[i].second.first, oy = all[i].second.second;
    if (o->kind == 1 || o->bgopa == 0) continue;
    if (px >= ox && px < ox + o->w && py >= oy && py < oy + o->h) c = lvm565(o->bg);
  }
  return c;
}
inline void lvDumpOps() {
  if (!g_lv_screen) return;
  fprintf(g_ops, "FILLSCREEN %u\n", lvm565(g_lv_screen->bg));
  std::vector<std::pair<lv_obj_t*, std::pair<int, int>>> all;
  lvm_flat(g_lv_screen, 0, 0, all);
  for (size_t i = 0; i < all.size(); i++) {
    lv_obj_t* o = all[i].first; int x = all[i].second.first, y = all[i].second.second;
    if (o->flags & LV_OBJ_FLAG_HIDDEN) continue;
    if (o->kind == 1) {
      int sz = lvm_size(o->px); int tw = (int)o->text.size() * 6 * sz;
      int tx = x;
      if (!o->autoW) { if (o->align == LV_TEXT_ALIGN_CENTER) tx = x + (o->w - tw) / 2; else if (o->align == LV_TEXT_ALIGN_RIGHT) tx = x + o->w - tw; }
      int ty = y + (o->px + 3 - 8 * sz) / 2;
      fprintf(g_ops, "TEXT %d %d %d %u %u %s\n", tx, ty, sz, lvm565(o->tcol), lvm_bg_at(all, i, tx + 1, ty + 1, lvm565(g_lv_screen->bg)), o->text.c_str());
      continue;
    }
    if (o->kind == 2) {
      fprintf(g_ops, "RRECT %d %d %d %d %d %u 1\n", x, y, o->w, o->h, o->radius, lvm565(o->bg));
      int iw = o->w * o->pct / 100;
      if (iw >= o->h) fprintf(g_ops, "RRECT %d %d %d %d %d %u 1\n", x, y, iw, o->h, o->h / 2, lvm565(o->ind));
      else if (iw > 0) fprintf(g_ops, "RECT %d %d %d %d %u 1\n", x, y, iw, o->h, lvm565(o->ind));
      continue;
    }
    if (o->bgopa > 0) {
      if (o->radius > 0) fprintf(g_ops, "RRECT %d %d %d %d %d %u 1\n", x, y, o->w, o->h, o->radius, lvm565(o->bg));
      else fprintf(g_ops, "RECT %d %d %d %d %u 1\n", x, y, o->w, o->h, lvm565(o->bg));
    }
    if (o->border > 0) {
      if (o->radius > 0) fprintf(g_ops, "RRECT %d %d %d %d %d %u 0\n", x, y, o->w, o->h, o->radius, lvm565(o->bdr));
      else fprintf(g_ops, "RECT %d %d %d %d %u 0\n", x, y, o->w, o->h, lvm565(o->bdr));
    }
  }
}
inline void lv_refr_now(lv_display_t*) { lvDumpOps(); }
// ตัวช่วยของตัวจำลอง: จำลองการคลิกที่หน้าจอราก (ใช้ทดสอบเส้นทาง LVGL -> dispatchTap)
inline void lvMockClick(int x, int y) {
  g_lv_point = {x, y};
  if (g_lv_screen && g_lv_screen->cb) { lv_event_t e{g_lv_screen}; g_lv_screen->cb(&e); }
}
