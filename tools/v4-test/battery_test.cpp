// ทดสอบตัวตรวจระดับแบตเตอรี่ (battery_check.h) บน PC ด้วยนาฬิกาจำลอง
#include "battery_check.h"
#include <cstdio>

static int failures = 0;
static void check(bool ok, const char* what) {
  printf("  %-72s %s\n", what, ok ? "ผ่าน" : "ไม่ผ่าน");
  if (!ok) failures++;
}

// ป้อนแรงดันคงที่ทุก 1 วินาทีเป็นเวลา sec วินาที คืนระดับสุดท้าย
static uint8_t feed(BatteryMonitor& b, uint32_t& t, uint16_t mv, int sec) {
  uint8_t l = b.level;
  for (int i = 0; i < sec; i++) { t += 1000; l = b.update(t, mv); }
  return l;
}

int main() {
  uint32_t t = 100000;

  printf("1) ไม่มีวงจรวัด / ไฟ USB ไม่เตือน\n");
  { BatteryMonitor b; check(b.update(t, 0) == BAT_NONE, "0 mV = ไม่มีวงจรวัด");
    check(feed(b, t, 2400, 120) == BAT_NONE, "ต่ำกว่า 2.5 V ถือว่าไม่มีวงจร ไม่ใช่แบตหมด"); }

  printf("\n2) อ่านครั้งแรกตัดสินทันที\n");
  { BatteryMonitor b; check(b.update(t, 4100) == BAT_OK, "4.10 V = ปกติ");
    BatteryMonitor c; check(c.update(t, 3450) == BAT_LOW, "เปิดเครื่องที่ 3.45 V = ต่ำ ทันที ไม่ต้องรอ 30 วินาที");
    BatteryMonitor d; check(d.update(t, 3300) == BAT_CRIT, "เปิดเครื่องที่ 3.30 V = วิกฤต ทันที"); }

  printf("\n3) ลดระดับต้องต่อเนื่อง 30 วินาที\n");
  { BatteryMonitor b; t = 100000; b.update(t, 4000);
    check(feed(b, t, 3400, 29) == BAT_OK, "ต่ำ 29 วินาที ยังเป็นปกติ");
    check(feed(b, t, 3400, 2) == BAT_LOW, "ครบ 30 วินาที -> ต่ำ");
    check(feed(b, t, 3300, 29) == BAT_LOW, "วิกฤต 29 วินาที ยังเป็นต่ำ");
    check(feed(b, t, 3300, 2) == BAT_CRIT, "ครบ 30 วินาที -> วิกฤต"); }

  printf("\n4) แรงดันตกชั่วขณะ (บัซเซอร์ดัง) ไม่ทำให้เตือนผิด\n");
  { BatteryMonitor b; t = 100000; b.update(t, 3900);
    for (int k = 0; k < 20; k++) { feed(b, t, 3300, 5); feed(b, t, 3900, 5); }   // ตก 5 วินาที กลับ 5 วินาที สลับกัน 200 วินาที
    check(b.level == BAT_OK, "ตกสั้น ๆ สลับกลับ 200 วินาที ระดับยังเป็นปกติ"); }

  printf("\n5) ฮิสเทอรีซิส: แรงดันแกว่งรอบเส้นแบ่งไม่สลับระดับ\n");
  { BatteryMonitor b; t = 100000; b.update(t, 3400);                     // ต่ำ
    check(b.level == BAT_LOW, "เริ่มที่ต่ำ");
    check(feed(b, t, 3520, 120) == BAT_LOW, "3.52 V (เหนือเส้นต่ำแต่ไม่ถึง +50 mV) ยังเป็นต่ำ");
    check(feed(b, t, 3560, 31) == BAT_OK, "3.56 V (เกิน +50 mV) นาน 30 วินาที กลับเป็นปกติ");
    for (int k = 0; k < 60; k++) { feed(b, t, 3480, 1); feed(b, t, 3510, 1); }
    check(b.level == BAT_OK, "แกว่ง 3.48/3.51 ไม่ดึงกลับไปต่ำภายในเวลาสั้น (ต้องต่ำกว่าเส้นต่อเนื่อง 30 วินาที)"); }

  printf("\n6) ชาร์จกลับขึ้น\n");
  { BatteryMonitor b; t = 100000; b.update(t, 3300);
    check(b.level == BAT_CRIT, "เริ่มที่วิกฤต");
    check(feed(b, t, 3420, 31) == BAT_LOW, "3.42 V (เกินเส้นวิกฤต +50 mV) -> ต่ำ");
    check(feed(b, t, 4100, 31) == BAT_OK, "4.10 V -> ปกติ");
    check(feed(b, t, 0, 1) == BAT_NONE, "ถอดแบต/เสียบ USB -> ไม่มีวงจร ล้างการเตือนทันที"); }

  printf("\n7) เปอร์เซ็นต์และข้อความ\n");
  check(batteryPctFromMv(4200) == 100 && batteryPctFromMv(3200) == 0 && batteryPctFromMv(3700) == 50, "เปอร์เซ็นต์ตรงสมการของ Host");
  check(batteryPctFromMv(5000) == 100 && batteryPctFromMv(2900) == 0, "เปอร์เซ็นต์ถูกจำกัดช่วง");
  check(batteryPctFromMv(BAT_LOW_MV) == 30 && batteryPctFromMv(BAT_CRIT_MV) == 15, "เส้นต่ำ = 30% เส้นวิกฤต = 15%");
  check(batLevelWord(BAT_LOW)[0] == 'L' && batLevelWord(BAT_CRIT)[0] == 'B' && batLevelWord(BAT_OK)[0] == 0, "ข้อความระดับ");

  printf("\n%s\n", failures ? "มีข้อที่ไม่ผ่าน" : "ผ่านทุกข้อ");
  return failures ? 1 : 0;
}
