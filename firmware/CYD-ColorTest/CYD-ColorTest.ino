/**
 * @file      CYD-ColorTest.ino
 * @brief     เครื่องมือทดสอบสีจอบอร์ด CYD หน้าเดียว ไล่ครบทุกชุดค่า (ชิปจอ x กลับสี x ลำดับสี) หาค่าที่ถูกต้อง
 * @version   S1.0.3
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Hardware
 * ESP32-2432S028 (CYD) จอ ILI9341 หรือ ST7789 240x320 ไม่ต้องใช้ทัช ใช้ปุ่ม BOOT (GPIO0) กับ Serial 115200
 *
 * @par Description
 * ปัญหาสีบนบอร์ด CYD มักมาจากตั้งชิปจอ โหมดกลับสี หรือลำดับสี (RGB/BGR) ไม่ตรงกับบอร์ดจริง
 * เครื่องมือนี้วาดหน้าเดียวที่ตอบได้ทันทีว่าสีถูกหรือไม่ แล้วให้สลับชุดค่าทีละแบบโดยไม่ต้องคอมไพล์ใหม่:
 *   1) แถบสี 8 สี (ขาว เหลือง ฟ้า เขียว ม่วง แดง น้ำเงิน ดำ) ตัวอักษรในแถบบอกชื่อสีที่ควรเห็น
 *   2) คำว่า RED / GREEN / BLUE เขียนด้วยสีของมันเอง ถ้าคำว่า RED ออกเป็นสีน้ำเงิน แปลว่าลำดับสีสลับ (BGR)
 *   3) แถบไล่เฉดเทา 32 ระดับ และกล่องดำสนิทในกรอบขาว ถ้าดำออกเป็นเทา/ซีด แปลว่าโหมดกลับสีผิด
 *   4) ช่องดำละเอียด 8 ระดับใกล้ดำ ช่องแรกต้องดำสนิท ช่องถัดไปค่อย ๆ สว่างขึ้นเล็กน้อย
 * ชุดค่าที่ถูก = แถบทุกแถบตรงกับตัวอักษร คำสามคำสีตรงกับชื่อ ดำเป็นดำ ให้จดค่าที่แสดงบนหัวจอไปตั้งในเฟิร์มแวร์
 *
 * วิธีใช้:
 *   กด BOOT สั้น  สลับชุด กลับสี/ลำดับสี (4 ชุดต่อชิป) · กด BOOT ค้าง 1 วินาที สลับชิปจอ ILI9341 <-> ST7789
 *   Serial: n = ชุดถัดไป · p = สลับชิป · i = สลับกลับสี · b = สลับ RGB/BGR · r = หมุนจอ · a = โหมดเดินเองทุก 4 วินาที · h = ช่วยเหลือ
 *           k = จอดำเต็มจอ · w = จอขาวเต็มจอ · + / - = ความสว่างไฟหลังจอ · x = กลับหน้าทดสอบ (ใช้ดูว่าดำสนิทจริงไหม ปิดไฟห้องแล้วดูด้วยตา ภาพถ่ายหลอกได้เพราะกล้องปรับแสงอัตโนมัติ)
 * หมายเหตุ: ลำดับสี (BGR) สั่งได้เฉพาะ ST7789 ในรุ่น ILI9341 ใช้ค่าที่ไลบรารีตั้งให้ (แถวนั้นจึงแสดงชุดเดียว)
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | S1.0.3 | 2026-11-27 | เพิ่มคำสั่ง k (จอดำเต็มจอ) w (จอขาวเต็มจอ) + - (ความสว่าง) x (กลับหน้าทดสอบ) ใช้ดูระดับดำจริงของแผงโดยไม่ให้กล้องปรับแสงหลอก |
 * | S1.0.2 | 2026-11-27 | กดชุดถัดไปบน ST7789 วนทีละบิต (INV0/BGR0 -> INV1/BGR0 -> INV1/BGR1 -> INV0/BGR1) กดครั้งแรกจึงลองกลับสีทันที · ค่าเริ่มต้นเป็น BGR=0 และ ROT=3 ตามผลถ่ายภาพจากบอร์ดจริง (BGR=1 ทำให้แดง/น้ำเงินสลับ ROT=1 ภาพกลับหัว 180 องศา) |
 * | S1.0.1 | 2026-11-27 | เริ่มที่ ST7789 (START_PANEL 1) ตามผลบนบอร์ดจริงที่จอ ILI9341 ให้ภาพไม่เต็มจอและแดง/น้ำเงินสลับ ตั้ง 0 ถ้าบอร์ดเป็นจอ ILI9341 |
 * | S1.0.0 | 2026-11-27 | สร้างเครื่องมือทดสอบสีหน้าเดียว ไล่ชุดค่าได้ด้วยปุ่ม BOOT |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 * @note     ไฟล์นี้ไม่มีโครงสร้างแพ็กเก็ต Protocol v3 จึงไม่อยู่ในขอบเขตของ `tools/protocol-test/run.sh`
 */
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_ST7789.h>

#define APP_VERSION "S1.0.3"

// ขาของบอร์ด CYD (ตายตัวตามผังวงจร)
#define PIN_SCK 14
#define PIN_MISO 12
#define PIN_MOSI 13
#define PIN_CS 15
#define PIN_DC 2
#define PIN_BL 21
#define PIN_BOOT 0
#define SPI_HZ 40000000UL
#define START_PANEL 1        // ชิปจอที่ใช้ตอนเปิดเครื่อง: 1 = ST7789 (บอร์ด USB สองช่อง) · 0 = ILI9341 (USB ช่องเดียว) สลับขณะรันได้ด้วย BOOT ค้าง

SPIClass spiTft(HSPI);
Adafruit_ILI9341 *ili = nullptr;
Adafruit_ST7789 *st = nullptr;

uint8_t panelType = START_PANEL;   // 0 = ILI9341, 1 = ST7789
bool invertOn = false;
bool bgrOn = false;      // ผลจากบอร์ดจริง: BGR=0 ให้ลำดับสีถูก
uint8_t rotation = 3;    // ผลจากบอร์ดจริง: ROT=1 ภาพกลับหัวเมื่อถือพอร์ต USB ไว้ด้านขวา
bool autoMode = false;
uint32_t lastAuto = 0;
int blPct = 100;

static const uint16_t BLACK = 0x0000, WHITE = 0xFFFF, RED = 0xF800, GREEN = 0x07E0, BLUE = 0x001F,
                      YELLOW = 0xFFE0, CYAN = 0x07FF, MAGENTA = 0xF81F;

#define GFX (panelType ? (Adafruit_GFX *)st : (Adafruit_GFX *)ili)

static void backlight(int pct) {
  blPct = pct;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_BL, 5000, 8);
  ledcWrite(PIN_BL, (pct * 255) / 100);
#else
  ledcSetup(0, 5000, 8);
  ledcAttachPin(PIN_BL, 0);
  ledcWrite(0, (pct * 255) / 100);
#endif
}

// ลำดับสี BGR ของ ST7789 (บิต 3 ของ MADCTL) บนฐานของตารางหมุนจอ Adafruit: 0=MX|MY 1=MY|MV 2=ไม่มี 3=MX|MV
static void applyMadctl() {
  if (panelType != 1) return;
  static const uint8_t base[4] = {0xC0, 0xA0, 0x00, 0x60};
  uint8_t m = base[rotation & 3] | (bgrOn ? 0x08 : 0x00);
  st->sendCommand(0x36, &m, 1);
}

static void applyPanelSettings() {
  if (panelType == 1) { st->invertDisplay(invertOn); st->setRotation(rotation & 3); applyMadctl(); }
  else { ili->invertDisplay(invertOn); ili->setRotation(rotation & 3); }
}

static void txt(int x, int y, int size, uint16_t fg, uint16_t bg, const char *s) {
  GFX->setTextSize(size);
  GFX->setTextColor(fg, bg);
  GFX->setCursor(x, y);
  GFX->print(s);
}

static void drawTest() {
  Adafruit_GFX *g = GFX;
  const int w = 320;
  g->fillScreen(BLACK);
  char s[64];
  snprintf(s, sizeof(s), "%s INV=%d BGR=%d ROT=%d", panelType ? "ST7789" : "ILI9341", invertOn ? 1 : 0, bgrOn ? 1 : 0, rotation & 3);
  txt(2, 2, 2, WHITE, BLACK, s);
  // 1) แถบสี 8 สี ตัวอักษรบอกสีที่ควรเห็น
  const uint16_t bars[8] = {WHITE, YELLOW, CYAN, GREEN, MAGENTA, RED, BLUE, BLACK};
  const char names[8] = {'W', 'Y', 'C', 'G', 'M', 'R', 'B', 'K'};
  for (int i = 0; i < 8; i++) {
    g->fillRect(i * (w / 8), 20, w / 8, 36, bars[i]);
    char c[2] = {names[i], 0};
    txt(i * (w / 8) + 14, 30, 2, i == 7 ? WHITE : BLACK, bars[i], c);
  }
  // 2) คำเขียนด้วยสีของมันเอง
  txt(8, 62, 3, RED, BLACK, "RED");
  txt(112, 62, 3, GREEN, BLACK, "GREEN");
  txt(232, 62, 3, BLUE, BLACK, "BLUE");
  // 3) ไล่เฉดเทา 32 ระดับ + ไล่เฉดแดง เขียว น้ำเงิน
  for (int i = 0; i < 32; i++) {
    const uint16_t gray = (uint16_t)((i << 11) | ((i * 2) << 5) | i);
    g->fillRect(i * 10, 92, 10, 14, gray);
    g->fillRect(i * 10, 108, 10, 6, (uint16_t)(i << 11));
    g->fillRect(i * 10, 114, 10, 6, (uint16_t)((i * 2) << 5));
    g->fillRect(i * 10, 120, 10, 6, (uint16_t)i);
  }
  txt(2, 130, 1, WHITE, BLACK, "GRAY/R/G/B RAMPS: SMOOTH, NO WRONG-COLOR BANDS");
  // 4) ดำสนิทในกรอบขาว และช่องใกล้ดำ 8 ระดับ
  g->fillRect(4, 144, 92, 66, WHITE);
  g->fillRect(14, 154, 72, 46, BLACK);
  txt(18, 172, 1, WHITE, BLACK, "PURE BLACK");
  for (int i = 0; i < 8; i++) {
    const uint16_t v = (uint16_t)((i << 11) | ((i * 2) << 5) | i);   // เทา 0..7 จาก 31 ระดับ (ช่อง 0 = ดำสนิท)
    g->fillRect(104 + i * 26, 144, 24, 40, v);
    char c[2] = {(char)('0' + i), 0};
    txt(112 + i * 26, 188, 1, WHITE, BLACK, c);
  }
  txt(104, 200, 1, WHITE, BLACK, "BLACK STEPS 0-7: #0 MUST BE BLACK");
  txt(2, 216, 1, YELLOW, BLACK, "RIGHT = bars match letters, RED/GREEN/BLUE correct,");
  txt(2, 226, 1, YELLOW, BLACK, "black is black. BOOT=next set  HOLD=chip type");
}

static void panelInit() {
  delete ili; delete st; ili = nullptr; st = nullptr;
  if (panelType == 1) {
    st = new Adafruit_ST7789(&spiTft, PIN_CS, PIN_DC, -1);
    st->init(240, 320);
    st->setSPISpeed(SPI_HZ);
  } else {
    ili = new Adafruit_ILI9341(&spiTft, PIN_DC, PIN_CS, -1);
    ili->begin(SPI_HZ);
  }
  GFX->setTextWrap(false);
  applyPanelSettings();
  drawTest();
}

static void printState() {
  Serial.print(panelType ? "ST7789" : "ILI9341");
  Serial.print(" invert="); Serial.print(String((int)invertOn));
  Serial.print(" bgr="); Serial.print(String((int)bgrOn));
  Serial.print(" rot="); Serial.println(String((int)(rotation & 3)));
}

// ชุดถัดไป: ST7789 วน 4 ชุด (กลับสี x BGR) · ILI9341 วน 2 ชุด (กลับสีอย่างเดียว เพราะไลบรารีกำหนดลำดับสีเอง)
static void nextSet() {
  if (panelType == 1) {
    if (invertOn == bgrOn) invertOn = !invertOn; else bgrOn = !bgrOn;   // วนแบบเกรย์โค้ด: (0,0) -> (1,0) -> (1,1) -> (0,1) ทีละบิต
  } else {
    invertOn = !invertOn;
  }
  applyPanelSettings();
  drawTest();
  printState();
}

static void handleCommand(char c) {
  if (c == 'n') nextSet();
  else if (c == 'p') { panelType = panelType ? 0 : 1; panelInit(); printState(); }
  else if (c == 'i') { invertOn = !invertOn; applyPanelSettings(); drawTest(); printState(); }
  else if (c == 'b') { bgrOn = !bgrOn; applyPanelSettings(); drawTest(); printState(); }
  else if (c == 'r') { rotation = (rotation + 1) & 3; applyPanelSettings(); drawTest(); printState(); }
  else if (c == 'a') { autoMode = !autoMode; lastAuto = millis(); Serial.println(autoMode ? "auto ON" : "auto OFF"); }
  else if (c == 'k') { GFX->fillScreen(BLACK); Serial.println("full black"); }
  else if (c == 'w') { GFX->fillScreen(WHITE); Serial.println("full white"); }
  else if (c == '+' || c == '-') { int v = blPct + (c == '+' ? 10 : -10); if (v < 10) v = 10; if (v > 100) v = 100; backlight(v); Serial.print("backlight "); Serial.println(String(v)); }
  else if (c == 'x') { drawTest(); printState(); }
  else if (c == 'h') Serial.println("n next set | p chip | i invert | b bgr | r rotate | a auto | k black | w white | +/- backlight | x test page | BOOT short=next, long=chip");
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BOOT, INPUT_PULLUP);
  backlight(0);
  spiTft.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  panelInit();
  backlight(100);
  Serial.println("CYD-ColorTest " APP_VERSION);
  printState();
  handleCommand('h');
}

void loop() {
  while (Serial.available()) {
    const int c = Serial.read();
    if (c != '\r' && c != '\n') handleCommand((char)c);
  }
  if (autoMode && millis() - lastAuto >= 4000) { lastAuto = millis(); nextSet(); }
  if (digitalRead(PIN_BOOT) == LOW) {                  // สั้น = ชุดถัดไป · ค้าง 1 วินาที = สลับชิป
    const uint32_t t0 = millis();
    while (digitalRead(PIN_BOOT) == LOW && millis() - t0 < 1000) delay(10);
    if (millis() - t0 >= 1000) handleCommand('p'); else handleCommand('n');
    while (digitalRead(PIN_BOOT) == LOW) delay(10);
  }
  delay(5);
}
