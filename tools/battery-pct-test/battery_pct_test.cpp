// ส่วนทดสอบ ต่อท้ายตารางและฟังก์ชันที่ run.sh ตัดมาจาก Host-OLED.ino
struct Case { float v; int want; const char *why; };

int main() {
  const Case cases[] = {
    { 0.00f, 100, "ไม่มีวงจรวัด (0 V) ถือว่าเสียบ USB" },
    { 0.50f, 100, "ขอบของกรณีไม่มีวงจรวัด" },
    { 2.80f,   0, "ต่ำกว่าปลายตาราง" },
    { 3.00f,   0, "ปลายล่างของตาราง" },
    { 3.20f,   2, "ระหว่าง 3.00 กับ 3.45 V" },
    { 3.45f,   5, "จุดในตาราง" },
    { 3.70f,  13, "สูตรเส้นตรงเดิมให้ 50 %" },
    { 3.82f,  50, "จุดกึ่งกลางตาราง" },
    { 3.98f,  80, "จุดในตาราง" },
    { 4.13f,  95, "ระหว่าง 4.06 กับ 4.20 V" },
    { 4.20f, 100, "ชาร์จเต็ม" },
    { 4.25f, 100, "เกินชาร์จเต็ม" },
  };
  int fail = 0;
  for (const Case &c : cases) {
    int got = calculateBatteryPct(c.v);
    bool ok = got == c.want;
    printf("%s  %.2f V -> %3d %% (ต้องได้ %3d)  %s\n", ok ? "ok  " : "FAIL", c.v, got, c.want, c.why);
    if (!ok) fail++;
  }
  // ไม่ลดลงเมื่อแรงดันเพิ่ม และอยู่ใน 0-100 ตลอดช่วงที่ ADC อ่านได้
  int prev = 0;
  for (int mv = 2500; mv <= 4300; mv++) {
    int p = calculateBatteryPct(mv / 1000.0f);
    if (p < prev || p < 0 || p > 100) { printf("FAIL  ไม่เป็นทางเดียวที่ %d mV (%d -> %d)\n", mv, prev, p); fail++; break; }
    prev = p;
  }
  // สถานะเตือนแบตต่ำ: ลงทันทีที่ถึงเกณฑ์ ขึ้นต้องเกินเกณฑ์ + 0.05 V
  struct Step { float v; uint8_t want; const char *why; };
  const Step steps[] = {
    { 0.00f, BATT_STATE_OK,   "เสียบ USB ไม่มีแบต ไม่เตือน" },
    { 3.90f, BATT_STATE_OK,   "แบตปกติ" },
    { 3.71f, BATT_STATE_OK,   "ยังเหนือเกณฑ์ต่ำ" },
    { 3.70f, BATT_STATE_LOW,  "ถึงเกณฑ์ต่ำ 3.70 V" },
    { 3.74f, BATT_STATE_LOW,  "แกว่งขึ้นยังไม่พ้นค่าหน่วง (ต้องเกิน 3.75)" },
    { 3.76f, BATT_STATE_OK,   "พ้นค่าหน่วง หายเตือน" },
    { 3.68f, BATT_STATE_LOW,  "ต่ำอีกครั้ง" },
    { 3.50f, BATT_STATE_CRIT, "ถึงเกณฑ์วิกฤต 3.50 V" },
    { 3.54f, BATT_STATE_CRIT, "วิกฤตยังไม่พ้นค่าหน่วง (ต้องเกิน 3.55)" },
    { 3.60f, BATT_STATE_LOW,  "พ้นวิกฤตแต่ยังต่ำ" },
    { 3.40f, BATT_STATE_CRIT, "ต่ำลงอีก" },
    { 4.10f, BATT_STATE_OK,   "เสียบชาร์จ แรงดันขึ้น หายเตือนทันที" },
    { 3.40f, BATT_STATE_CRIT, "จากปกติตกลงวิกฤตในรอบเดียว" },
    { 0.00f, BATT_STATE_OK,   "ถอดแบตออก" },
  };
  uint8_t st = BATT_STATE_OK;
  for (const Step &k : steps) {
    st = batteryAlertState(st, k.v);
    bool ok = st == k.want;
    printf("%s  %.2f V -> state %d (ต้องได้ %d)  %s\n", ok ? "ok  " : "FAIL", k.v, st, k.want, k.why);
    if (!ok) fail++;
  }

  if (fail) { printf("ไม่ผ่าน %d รายการ\n", fail); return 1; }
  printf("ผ่านทั้งหมด\n");
  return 0;
}
