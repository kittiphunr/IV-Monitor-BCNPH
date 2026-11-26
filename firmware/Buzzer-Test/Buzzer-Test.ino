/**
 * @file      Buzzer-Test.ino
 * @brief     เครื่องมือหาความถี่ที่บัซเซอร์ Passive ดังที่สุด (กวาดความถี่ และเล่นความถี่ที่ระบุ)
 * @version   S1.0.0
 * @date      2026-11-26
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 * อาจารย์ที่ปรึกษา ดร.กรรณิการ์ กาศสมบูรณ์
 *
 * @par Hardware
 * ESP32-S3 Super Mini + Passive Buzzer ที่ GPIO 3 (ขาเดียวกับเครื่องประจำเตียง) แก้ BUZZER_PIN ได้ถ้าใช้บอร์ดอื่น
 * (Host ใช้ GPIO 7, บอร์ด CYD ใช้ GPIO 26)
 *
 * @par Description
 * บัซเซอร์ Passive แต่ละตัวมีความถี่ที่ดังที่สุด (เรโซแนนซ์) ต่างกัน ปกติราว 2.5 ถึง 4 kHz เสียงที่ได้ยินเบา
 * มักเพราะเฟิร์มแวร์ใช้โทนไกลจากจุดนั้น (เครื่องประจำเตียงใช้ 1000 ถึง 2600 Hz) เครื่องมือนี้กวาดความถี่
 * ให้ฟังด้วยหู แล้วบอกว่าความถี่ไหนดังที่สุด เพื่อนำค่าไปปรับในเฟิร์มแวร์ต่อ
 *
 * วิธีใช้: อัปโหลด เปิด Serial Monitor 115200 (ตั้งท้ายบรรทัดเป็น Newline)
 *   s        กวาดความถี่ 500 ถึง 5000 Hz ทีละ 100 Hz ค้างความถี่ละ 0.5 วินาที พิมพ์ความถี่ที่กำลังเล่น
 *   ตัวเลข   เช่น 2700 เล่นความถี่นั้นค้าง 3 วินาที (ใช้ยืนยันความถี่ที่ดังที่สุดทีละค่าให้ละเอียดขึ้น)
 *   x        หยุด
 * ฟังแล้วจดความถี่ที่ดังที่สุด (ลองกวาดซ้ำและเล่นค่าละเอียดรอบ ๆ จุดนั้น) แล้วแจ้งผู้พัฒนา
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | S1.0.0 | 2026-11-26 | สร้างเครื่องมือกวาดความถี่บัซเซอร์ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 * @warning  ปรับระดับเสียงด้วยหูเท่านั้น ไม่ได้วัดเป็นเดซิเบล สภาพห้องและตำแหน่งตัวบัซเซอร์ (รูเสียงในกล่อง) มีผลมาก
 * @note     ไฟล์นี้ไม่มีโครงสร้างแพ็กเก็ต Protocol v3 จึงไม่อยู่ในขอบเขตของ `tools/protocol-test/run.sh`
 */

#define APP_VERSION   "S1.0.0"
#define BUZZER_PIN    3          // Station = 3 · Host = 7 · CYD = 26

#define SWEEP_MIN_HZ   500
#define SWEEP_MAX_HZ   5000
#define SWEEP_STEP_HZ  100
#define SWEEP_HOLD_MS  500
#define SINGLE_HOLD_MS 3000

enum Mode { MODE_IDLE, MODE_SWEEP, MODE_SINGLE };
Mode mode = MODE_IDLE;
int  currentHz = 0;
unsigned long modeUntil = 0;
unsigned long nextStepAt = 0;
String lineBuf = "";

void playHz(int hz) {
  currentHz = hz;
  tone(BUZZER_PIN, hz);
  Serial.printf("เล่น %d Hz\n", hz);
}

void stopAll() {
  noTone(BUZZER_PIN);
  mode = MODE_IDLE;
  currentHz = 0;
}

void handleCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;
  if (cmd == "x" || cmd == "X") {
    stopAll();
    Serial.println("หยุด");
  } else if (cmd == "s" || cmd == "S") {
    mode = MODE_SWEEP;
    nextStepAt = 0;
    currentHz = SWEEP_MIN_HZ - SWEEP_STEP_HZ;
    Serial.println("เริ่มกวาดความถี่ 500 ถึง 5000 Hz จดความถี่ที่ดังที่สุด");
  } else {
    int hz = cmd.toInt();
    if (hz >= 100 && hz <= 20000) {
      mode = MODE_SINGLE;
      playHz(hz);
      modeUntil = millis() + SINGLE_HOLD_MS;
    } else {
      Serial.println("ไม่เข้าใจคำสั่ง: พิมพ์ s (กวาด), ตัวเลข 100-20000 (เล่นความถี่), x (หยุด)");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);
  Serial.println("\nBuzzer-Test " APP_VERSION);
  Serial.printf("บัซเซอร์ที่ GPIO %d  พิมพ์ s = กวาดความถี่, ตัวเลข = เล่นความถี่นั้น, x = หยุด\n", BUZZER_PIN);
}

void loop() {
  while (Serial.available()) {
    int c = Serial.read();
    if (c == '\n' || c == '\r') { handleCommand(lineBuf); lineBuf = ""; }
    else if (c >= 0) lineBuf += (char)c;
  }

  unsigned long now = millis();
  if (mode == MODE_SWEEP && now >= nextStepAt) {
    int next = currentHz + SWEEP_STEP_HZ;
    if (next > SWEEP_MAX_HZ) {
      stopAll();
      Serial.println("กวาดเสร็จแล้ว พิมพ์ s เพื่อกวาดซ้ำ หรือพิมพ์ความถี่ที่ดังที่สุดเพื่อฟังซ้ำ");
    } else {
      playHz(next);
      nextStepAt = now + SWEEP_HOLD_MS;
    }
  } else if (mode == MODE_SINGLE && now >= modeUntil) {
    stopAll();
    Serial.println("จบ");
  }
  delay(5);
}
