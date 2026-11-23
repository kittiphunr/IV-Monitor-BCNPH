/**
 * @file      CYD-ScreenTest.ino
 * @brief     เครื่องมือทดสอบจอของบอร์ด ESP32-2432S028 (CYD) หาว่าจอเป็นชิปอะไรและตั้งค่าแบบไหนถึงแสดงถูก
 * @version   S1.2.0
 * @date      2026-11-23
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 * อาจารย์ที่ปรึกษา ดร.กรรณิการ์ กาศสมบูรณ์
 *
 * @par Hardware
 * ESP32-2432S028 (ESP32-WROOM-32 + จอ 2.8" 240x320) ขาจอ SCK14 MOSI13 MISO12 CS15 DC2 BL21
 *
 * @par Description
 * ใช้เมื่อเฟิร์มแวร์ Host บนบอร์ด CYD แสดงผลเพี้ยน (สีซีด ภาพไม่เต็มจอ สีกลับด้าน)
 * สเก็ตช์นี้ทดลองสั่งจอสี่แบบสลับกัน ไดรเวอร์ ILI9341 / ST7789 อย่างละสองโหมดกลับสี
 * แล้ววาดแถบสี 8 สี แถบเทา กรอบขอบจอ และป้ายมุมทั้งสี่ เพื่อดูด้วยตาว่าแบบไหนถูก
 * รุ่น S1.1.0 ลองทุกทิศทางการหมุน (rotation 0-3) ต่อทุกโหมดด้วย รวม 16 ชุด ใช้หาชุดที่ภาพเต็มจอและไม่กลับด้าน
 * พร้อมอ่านรหัสชิปจอส่งออกทาง Serial Monitor (115200) เพื่อส่งให้ผู้พัฒนาวิเคราะห์ต่อ
 *
 * วิธีใช้: อัปโหลด เปิด Serial Monitor 115200 กดปุ่ม BOOT (หรือพิมพ์อะไรก็ได้ใน Serial)
 * เพื่อเปลี่ยนชุด (16 ชุด = 4 โหมด x 4 ทิศทาง) ทีละชุดดูว่า (1) สีเรียงถูกไหม ขาว เหลือง ฟ้า เขียว ม่วง แดง น้ำเงิน ดำ
 * (2) กรอบขาวติดครบสี่ด้านและเห็นป้ายมุมทั้งสี่ไหม (3) สีสดไม่ซีด แล้วบอกเลขโหมดที่ถูกต้อง
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | S1.2.0 | 2026-11-23 | โหมด ST7789 ตรงกับค่า cyd2usb ของ OttoMeister/ESP32-2432S028R (ปิดกลับสี + ลำดับสี BGR) |
 * | S1.1.0 | 2026-11-23 | ลองครบทุกทิศทางการหมุนจอ (0-3) ต่อทุกโหมด รวม 16 ชุด พิมพ์ชื่อชุดบนจอและทาง Serial |
 * | S1.0.0 | 2026-11-23 | สร้างเครื่องมือทดสอบจอสำหรับบอร์ด CYD |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 * @warning  ห้ามกดค้างปุ่ม BOOT ตอนจ่ายไฟหรือกด RST บอร์ดจะเข้าโหมดโปรแกรม
 *
 * @note     ไฟล์นี้ไม่มีโครงสร้างแพ็กเก็ต Protocol v3 จึงไม่อยู่ในขอบเขตของ `tools/protocol-test/run.sh`
 *           ต้องติดตั้งไลบรารี Adafruit GFX, Adafruit ILI9341 และ Adafruit ST7735 and ST7789 Library
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_ST7789.h>

#define APP_VERSION   "S1.2.0"

#define PIN_SCLK      14
#define PIN_MOSI      13
#define PIN_MISO      12
#define PIN_CS        15
#define PIN_DC        2
#define PIN_BL        21
#define PIN_BOOT      0

SPIClass spiTft(HSPI);
Adafruit_ILI9341 ili(&spiTft, PIN_DC, PIN_CS, -1);     // ILI9341: DC ก่อน CS
Adafruit_ST7789  st(&spiTft, PIN_CS, PIN_DC, -1);      // ST7789 : CS ก่อน DC

const int MODE_COUNT = 16;       // 4 โหมดชิป/กลับสี x 4 ทิศทางการหมุน
int mode = -1;
bool bootWasDown = false;

const char* modeName(int m) {
  switch (m & 3) {
    case 0:  return "0: ILI9341  invert OFF";
    case 1:  return "1: ILI9341  invert ON";
    case 2:  return "2: ST7789 BGR inv OFF (cyd2usb ref)";
    default: return "3: ST7789 BGR inv ON";
  }
}

// อ่านรหัสชิปจอผ่านขา MISO (ILI9341 แท้ตอบ ID4 = 00 93 41) แล้วพิมพ์ลง Serial
void printChipInfo() {
  Serial.println("---- display registers (ILI9341 read commands) ----");
  Serial.printf("RDDID  0x04: %02X %02X %02X\n", ili.readcommand8(0x04, 0), ili.readcommand8(0x04, 1), ili.readcommand8(0x04, 2));
  Serial.printf("ID4    0xD3: %02X %02X %02X\n", ili.readcommand8(0xD3, 1), ili.readcommand8(0xD3, 2), ili.readcommand8(0xD3, 3));
  Serial.printf("RDDST  0x09: %02X %02X %02X %02X\n", ili.readcommand8(0x09, 1), ili.readcommand8(0x09, 2),
                ili.readcommand8(0x09, 3), ili.readcommand8(0x09, 4));
  Serial.printf("RDMODE 0x0A: %02X  RDMADCTL 0x0B: %02X  RDPIXFMT 0x0C: %02X  RDSELFDIAG 0x0F: %02X\n",
                ili.readcommand8(0x0A), ili.readcommand8(0x0B), ili.readcommand8(0x0C), ili.readcommand8(0x0F));
  Serial.println("(ILI9341 แท้: ID4 = 00 93 41, RDPIXFMT = 55  ถ้าเป็น FF FF FF หรือ 00 00 00 ทั้งหมด แปลว่าอ่านค่าจากจอไม่ได้)");
}

int rotOf(int m) { return m >> 2; }   // ชุดที่ 0-3 = rotation 0, 4-7 = rotation 1, ...

void drawPattern(Adafruit_GFX &g, const char* label) {
  const uint16_t bars[8] = { 0xFFFF, 0xFFE0, 0x07FF, 0x07E0, 0xF81F, 0xF800, 0x001F, 0x0000 };
  int w = g.width(), h = g.height();
  g.fillScreen(0x0000);
  int bw = w / 8, barH = (h * 2) / 3;
  for (int i = 0; i < 8; i++) g.fillRect(i * bw, 0, bw, barH, bars[i]);
  for (int x = 0; x < w; x++) {                       // แถบเทาไล่ระดับ
    uint8_t v = (uint8_t)((x * 255L) / (w - 1));
    uint16_t c = ((v >> 3) << 11) | ((v >> 2) << 5) | (v >> 3);
    g.fillRect(x, barH, 1, h / 6, c);
  }
  g.fillRect(0, barH + h / 6, w, h - barH - h / 6, 0x0000);
  g.drawRect(0, 0, w, h, 0xFFFF);                     // กรอบขอบจอ ต้องเห็นครบสี่ด้าน
  g.drawRect(1, 1, w - 2, h - 2, 0xF800);
  g.setTextSize(2);
  g.setTextColor(0x0000, 0xFFFF); g.setCursor(4, 4);          g.print("TL");
  g.setTextColor(0xFFFF, 0xF800); g.setCursor(w - 30, 4);     g.print("TR");
  g.setTextColor(0xFFFF, 0x001F); g.setCursor(4, h - 20);     g.print("BL");
  g.setTextColor(0x0000, 0x07E0); g.setCursor(w - 30, h - 20); g.print("BR");
  g.setTextSize(1);
  g.setTextColor(0xFFFF, 0x0000);
  g.setCursor(30, barH + h / 6 + 8);  g.print(label);
  char buf[40];
  snprintf(buf, sizeof(buf), "size %d x %d   rotation %d   set %d/16", w, h, rotOf(mode), mode + 1);
  g.setCursor(30, barH + h / 6 + 24); g.print(buf);
  g.setCursor(30, barH + h / 6 + 40); g.print("BOOT = next mode");
}

void applyMode(int m) {
  mode = m;
  Serial.printf("\n==== SET %d/16  MODE %s  ROTATION %d ====\n", m + 1, modeName(m), rotOf(m));
  if ((m & 3) <= 1) {
    ili.begin(40000000);
    ili.setRotation(rotOf(m));
    ili.invertDisplay((m & 3) == 1);
    drawPattern(ili, modeName(m));
    Serial.printf("drawn %d x %d\n", ili.width(), ili.height());
  } else {
    st.init(240, 320);
    st.setSPISpeed(40000000);
    st.setRotation(rotOf(m));
    st.invertDisplay((m & 3) == 3);
    {   // ลำดับสี BGR (บิต 3 ของ MADCTL) คงบิตหมุนจอเดิมของไลบรารี: 0=MX|MY 1=MY|MV 2=ไม่มี 3=MX|MV
      const uint8_t madctlBgr[4] = { 0xC8, 0xA8, 0x08, 0x68 };
      uint8_t v = madctlBgr[rotOf(m) & 3];
      st.sendCommand(0x36, &v, 1);
    }
    drawPattern(st, modeName(m));
    Serial.printf("drawn %d x %d\n", st.width(), st.height());
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(PIN_BOOT, INPUT_PULLUP);
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH);
  spiTft.begin(PIN_SCLK, PIN_MISO, PIN_MOSI, PIN_CS);
  Serial.println("\nCYD-ScreenTest " APP_VERSION);
  ili.begin(40000000);
  printChipInfo();
  applyMode(0);
}

void loop() {
  bool down = (digitalRead(PIN_BOOT) == LOW);
  bool serialKey = false;
  while (Serial.available()) { Serial.read(); serialKey = true; }
  if ((down && !bootWasDown) || serialKey) {
    applyMode((mode + 1) % MODE_COUNT);
    if (mode == 0) printChipInfo();
  }
  bootWasDown = down;
  delay(20);
}
