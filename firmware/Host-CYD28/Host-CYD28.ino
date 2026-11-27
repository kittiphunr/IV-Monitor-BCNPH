/**
 * @file      Host-CYD28.ino
 * @brief     Host บอร์ด CYD (ESP32-2432S028) เขียนใหม่ ด่านที่ 1: ฐานบอร์ด จอ ไฟหลังจอ ทัช
 * @version   2.0.0-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Hardware
 * ESP32-2432S028 (CYD) จอ ILI9341 หรือ ST7789 240x320 ทัช XPT2046 (ขาแยกจากบัสจอ)
 *
 * @par Description
 * รุ่นนี้ยังไม่ใช่ Host เต็มรูป (ยังไม่มี ESP-NOW เว็บ หรือหน้าเตียง) เป็นด่านที่ 1 ของการเขียนใหม่
 * ไว้พิสูจน์ฮาร์ดแวร์บนบอร์ดจริงให้ได้ก่อน: แถบสีทดสอบจอ ช่องวาดตามนิ้ว หน้าคาลิเบรตสามจุด
 * และปุ่ม CAL / CLR / - / + ใช้หาค่า CYD_PANEL CYD_INVERT CYD_BGR ที่ถูกต้องของบอร์ดแต่ละใบได้เร็ว
 *
 * @par คำสั่ง Serial (115200)
 *   h = ช่วยเหลือ · i = สลับกลับสี · b = สลับ RGB/BGR (ST7789) · r = เปลี่ยนทิศ (ต้องคาลิเบรตใหม่)
 *   c = คาลิเบรต · + / - = ความสว่าง · d = พิมพ์รหัสชิปจอ (ILI9341)
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.0-CYD | 2026-11-27 | เขียนใหม่ทั้งหมดด่านที่ 1 ไม่นำโค้ดรุ่น 1.x มาปรับ: คาลิเบรต affine สามจุด (ไม่ต้องเดาแกนสลับ) ตัวติดตามนิ้ว DOWN/MOVE/UP สวิตช์สีขณะรัน |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#include <Preferences.h>
#include "board.h"
#include "display.h"
#include "touch_cal.h"
#include "touch_xpt2046.h"

#define APP_VERSION "2.0.0-CYD"

static const uint32_t CAL_MAGIC = 0x43414C32;      // "CAL2"

struct StoredCal {
  uint32_t magic;
  uint8_t  rotation;
  uint8_t  panel;        // 0 = ILI9341, 1 = ST7789
  uint16_t w, h;
  TouchCal k;
};

struct Button { int16_t x, y, w, h; const char *label; };

XptBitbang touchChip(cyd::TOUCH_CLK, cyd::TOUCH_MOSI, cyd::TOUCH_MISO, cyd::TOUCH_CS, cyd::TOUCH_IRQ);
TouchTracker tracker;
TouchCal cal;
Preferences prefs;
int brightness = 100;
int16_t lastX = 0, lastY = 0, lastRx = 0, lastRy = 0, lastZ = 0;

static const uint16_t C_BLACK = 0x0000, C_WHITE = 0xFFFF, C_RED = 0xF800, C_GREEN = 0x07E0, C_BLUE = 0x001F,
                      C_YELLOW = 0xFFE0, C_CYAN = 0x07FF, C_MAGENTA = 0xF81F, C_GRAY = 0x8410;

static uint16_t calPanelId() { return (CYD_PANEL == 7789) ? 1 : 0; }

static bool loadCal() {
  StoredCal s;
  prefs.begin("cyd", true);
  size_t n = prefs.getBytesLength("cal");
  bool ok = (n == sizeof(s)) && prefs.getBytes("cal", &s, sizeof(s)) == sizeof(s);
  prefs.end();
  if (!ok || s.magic != CAL_MAGIC || s.rotation != g_rotation || s.panel != calPanelId()
      || s.w != tft.width() || s.h != tft.height() || !s.k.valid) return false;
  cal = s.k;
  return true;
}

static void saveCal() {
  StoredCal s = {CAL_MAGIC, g_rotation, (uint8_t)calPanelId(), (uint16_t)tft.width(), (uint16_t)tft.height(), cal};
  prefs.begin("cyd", false);
  prefs.putBytes("cal", &s, sizeof(s));
  prefs.end();
}

static void printSettings() {
  Serial.print("panel=");   Serial.print(String(CYD_PANEL));
  Serial.print(" rot=");    Serial.print(String((int)g_rotation));
  Serial.print(" invert="); Serial.print(String((int)g_invert));
  Serial.print(" bgr=");    Serial.print(String((int)g_bgr));
  Serial.print(" bright="); Serial.println(String(brightness));
}

// ---- วาดหน้าทดสอบ ----------------------------------------------------------------------------
static Button buttons[4];
static int16_t paintTop, paintBottom;

static void layoutTest() {
  const int w = tft.width(), h = tft.height();
  const int barH = 36, rampH = 14, btnH = 34;
  paintTop = barH + rampH;
  paintBottom = h - btnH - 2;
  const char *labels[4] = {"CAL", "CLR", "-", "+"};
  const int bw = (w - 5 * 4) / 4;
  for (int i = 0; i < 4; i++) buttons[i] = { (int16_t)(4 + i * (bw + 4)), (int16_t)(h - btnH), (int16_t)bw, (int16_t)(btnH - 2), labels[i] };
}

static void drawButton(const Button &b) {
  tft.fillRect(b.x, b.y, b.w, b.h, 0x2104);
  tft.drawRect(b.x, b.y, b.w, b.h, C_GRAY);
  tft.setTextSize(2);
  tft.setTextColor(C_WHITE, 0x2104);
  tft.setCursor(b.x + b.w / 2 - (int)strlen(b.label) * 6, b.y + 9);
  tft.print(b.label);
}

static void drawStatus() {
  char s[48];
  snprintf(s, sizeof(s), "%3d,%3d raw %4d,%4d z%4d  ", lastX, lastY, lastRx, lastRy, lastZ);
  tft.setTextSize(1);
  tft.setTextColor(C_WHITE, C_BLACK);
  tft.setCursor(4, paintBottom - 10);
  tft.print(s);
}

static void drawTestScreen() {
  const int w = tft.width();
  layoutTest();
  tft.fillScreen(C_BLACK);
  const uint16_t bars[8] = {C_WHITE, C_YELLOW, C_CYAN, C_GREEN, C_MAGENTA, C_RED, C_BLUE, C_BLACK};
  for (int i = 0; i < 8; i++) tft.fillRect(i * w / 8, 0, w / 8 + 1, 36, bars[i]);
  for (int i = 0; i < 16; i++) {                      // ไล่เฉดเทา 16 ระดับ ดูว่าดำสนิทหรือซีด
    uint16_t v = (uint16_t)(i * 2);
    tft.fillRect(i * w / 16, 36, w / 16 + 1, 14, (uint16_t)((v << 11) | ((v * 2) << 5) | v));
  }
  tft.drawRect(0, paintTop, w, paintBottom - paintTop, C_GRAY);
  for (int i = 0; i < 4; i++) drawButton(buttons[i]);
  drawStatus();
}

// ---- ทัช ------------------------------------------------------------------------------------
static bool readTouch(int16_t &rx, int16_t &ry, int16_t &z) { return touchChip.read(rx, ry, z); }

static void waitRelease() {
  uint32_t quiet = millis();
  int16_t a, b, c;
  while (millis() - quiet < 120) {
    if (readTouch(a, b, c)) quiet = millis();
    delay(5);
  }
}

// เก็บตัวอย่างค่าดิบที่นิ่ง (เบี่ยงจากค่าเฉลี่ยไม่เกิน 150) ครบ 12 ตัวอย่าง
static void captureStable(float &ox, float &oy) {
  for (;;) {
    int16_t rx, ry, z;
    if (!readTouch(rx, ry, z)) { delay(5); continue; }
    float sx = rx, sy = ry;
    int n = 1;
    uint32_t t0 = millis();
    while (n < 12 && millis() - t0 < 1500) {
      if (!readTouch(rx, ry, z)) { delay(5); continue; }
      const float mx = sx / n, my = sy / n;
      if (fabsf(rx - mx) > 150 || fabsf(ry - my) > 150) { n = 0; sx = sy = 0; }
      sx += rx; sy += ry; n++;
      delay(8);
    }
    if (n >= 12) { ox = sx / n; oy = sy / n; return; }
  }
}

static void drawCross(int x, int y, uint16_t col) {
  tft.drawFastHLine(x - 12, y, 25, col);
  tft.drawFastVLine(x, y - 12, 25, col);
  tft.drawCircle(x, y, 6, col);
}

static void runCalibration() {
  const int w = tft.width(), h = tft.height();
  for (;;) {
    CalPoint p[3];
    touchCalTargets(w, h, p);
    for (int i = 0; i < 3; i++) {
      tft.fillScreen(C_BLACK);
      tft.setTextSize(2);
      tft.setTextColor(C_WHITE, C_BLACK);
      tft.setCursor(8, h / 2 - 8);
      tft.print("TOUCH THE CROSS");
      char s[16];
      snprintf(s, sizeof(s), "%d / 3", i + 1);
      tft.setCursor(8, h / 2 + 12);
      tft.print(s);
      drawCross((int)p[i].sx, (int)p[i].sy, C_GREEN);
      waitRelease();
      captureStable(p[i].rx, p[i].ry);
      Serial.print("cal point ");
      Serial.print(String(i + 1));
      Serial.print(" raw ");
      Serial.print(String((int)p[i].rx));
      Serial.print(",");
      Serial.println(String((int)p[i].ry));
      drawCross((int)p[i].sx, (int)p[i].sy, C_RED);
      waitRelease();
    }
    TouchCal k;
    if (touchCalSolve(p, w, h, k)) { cal = k; saveCal(); Serial.println("cal OK, saved"); return; }
    tft.fillScreen(C_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(C_RED, C_BLACK);
    tft.setCursor(8, h / 2 - 8);
    tft.print("BAD POINTS, RETRY");
    Serial.println("cal FAILED (points collinear or out of range), retry");
    delay(1500);
  }
}

// ---- Serial และปุ่ม --------------------------------------------------------------------------
static void applyBrightness(int delta) {
  brightness += delta;
  if (brightness < 10) brightness = 10;
  if (brightness > 100) brightness = 100;
  backlightSet(brightness);
  printSettings();
}

static void handleSerial() {
  while (Serial.available()) {
    int c = Serial.read();
    if (c == 'i') { panelSetInvert(!g_invert); drawTestScreen(); printSettings(); }
    else if (c == 'b') { panelSetBgr(!g_bgr); drawTestScreen(); printSettings(); }
    else if (c == 'r') { panelSetRotation(g_rotation + 1); runCalibration(); drawTestScreen(); printSettings(); }
    else if (c == 'c') { runCalibration(); drawTestScreen(); }
    else if (c == '+') applyBrightness(+10);
    else if (c == '-') applyBrightness(-10);
    else if (c == 'd') panelPrintId();
    else if (c == 'h') Serial.println("h help | i invert | b bgr | r rotate | c calibrate | +/- brightness | d id");
  }
}

static int hitButton(int x, int y) {
  for (int i = 0; i < 4; i++)
    if (x >= buttons[i].x && x < buttons[i].x + buttons[i].w && y >= buttons[i].y && y < buttons[i].y + buttons[i].h) return i;
  return -1;
}

static void onButton(int i) {
  if (i == 0) { runCalibration(); drawTestScreen(); }
  else if (i == 1) drawTestScreen();
  else if (i == 2) applyBrightness(-10);
  else if (i == 3) applyBrightness(+10);
}

void setup() {
  Serial.begin(115200);
  pinMode(cyd::BOOT_BTN, INPUT_PULLUP);
  panelBegin();
  touchChip.begin();
  Serial.println("Host-CYD28 " APP_VERSION " (stage 1: board bring-up)");
  printSettings();
  panelPrintId();
  if (!loadCal()) runCalibration();
  drawTestScreen();
}

void loop() {
  handleSerial();
  int16_t rx = 0, ry = 0, z = 0;
  const bool hit = readTouch(rx, ry, z);
  const TouchEvt ev = tracker.update(hit, millis());
  if (hit) {
    lastRx = rx; lastRy = ry; lastZ = z;
    touchCalApply(cal, rx, ry, tft.width(), tft.height(), lastX, lastY);
  }
  if (ev == TOUCH_DOWN) {
    const int b = hitButton(lastX, lastY);
    if (b >= 0) onButton(b);
  }
  if (hit && tracker.down() && lastY > paintTop + 2 && lastY < paintBottom - 12) tft.fillCircle(lastX, lastY, 2, C_YELLOW);
  if (ev == TOUCH_DOWN || ev == TOUCH_UP) drawStatus();
  if (digitalRead(cyd::BOOT_BTN) == LOW) {            // กด BOOT ค้าง 1.5 วินาที = คาลิเบรตใหม่
    uint32_t t0 = millis();
    while (digitalRead(cyd::BOOT_BTN) == LOW && millis() - t0 < 1500) delay(10);
    if (millis() - t0 >= 1500) { runCalibration(); drawTestScreen(); }
  }
  delay(4);
}
