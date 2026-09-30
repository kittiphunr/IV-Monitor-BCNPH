#!/usr/bin/env bash
# ตรวจสูตรแรงดันและ % แบตเตอรี่ ด้วยฟังก์ชันตัวจริงของ Host ทั้งสามรุ่นและเครื่องประจำเตียงบอร์ดเล็ก
#
# ที่มา: สูตรเดิมคิด raw / 4095 x 3.3 V แต่ ADC ที่ลดทอน 11 dB ของ ESP32-S3 วัดได้เต็มสเกลราว 3.1 V
# และของ ESP32-C3 ราว 2.5 V แบตเต็มจึงอ่านเกินจริง 6.5% (Host) ถึง 32% (C3) แล้วยังแปลงเป็น %
# แบบเส้นตรง แบตที่เกือบหมดจึงขึ้นว่ายังเหลือมาก ตอนนี้อ่านด้วย analogReadMilliVolts() และ
# แปลงด้วยตารางแรงดันของแบตลิเธียม ชุดนี้ตรวจว่า
#   1) แรงดันที่อ่านได้ = มิลลิโวลต์ที่ขา ADC x 2 (ตัวแบ่งแรงดัน 100k:100k) ไม่ขึ้นกับช่วงเต็มสเกลของชิป
#   2) % ตรงตามตารางที่จุดสำคัญ และไม่ลดลงเมื่อแรงดันเพิ่ม
set -uo pipefail
cd "$(dirname "$0")"
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
rc=0

cat > "$W/check.h" <<'CPP'
#include <cmath>
static int g_fail = 0;
static void expect(bool ok, const char *what) { printf("  %-54s %s\n", what, ok ? "ผ่าน" : "ไม่ผ่าน"); if (!ok) g_fail++; }
struct Pt { float v; int pct; };
static const Pt PTS[] = {{3.30f, 0}, {3.50f, 5}, {3.60f, 10}, {3.70f, 30}, {3.80f, 50}, {3.92f, 70}, {4.00f, 80}, {4.20f, 100}};
CPP

# ---- Host ทั้งสามรุ่น: ใช้ตัวขับของ host-rx-test แต่ให้ analogRead() คืนค่าที่ทดสอบได้ ----
for h in Host-OLED Host-TFT28 Host-Touch24; do
  echo "================ $h"
  d="$W/$h"; mkdir -p "$d"
  cp ../../firmware/$h/*.h "$d/"; cp ../../firmware/$h/$h.ino "$d/host_oled.cpp"
  sed '/^int main() {/,$d' ../host-rx-test/driver.cpp \
    | sed 's/^int analogRead(int){ return 2400; }/int g_raw = 2400; int analogRead(int){ return g_raw; }/' > "$d/drv.cpp"
  cat "$W/check.h" >> "$d/drv.cpp"
  cat >> "$d/drv.cpp" <<'CPP'
int main() {
  char what[96];
  for (int raw : {1500, 2400, 2700}) {
    g_raw = raw;
    float want = analogReadMilliVolts(0) / 1000.0f * 2.0f;
    float got = readHostBattery();
    snprintf(what, sizeof(what), "ADC %4d = %4u mV ที่ขา -> แบต %.3f V", raw, (unsigned)analogReadMilliVolts(0), got);
    expect(fabsf(got - (want < 2.5f ? 0.0f : (want > 4.25f ? 4.2f : want))) < 0.002f, what);
  }
  for (const Pt &p : PTS) {
    snprintf(what, sizeof(what), "%.2f V -> %d%% (ตาราง %d%%)", p.v, calculateBatteryPct(p.v), p.pct);
    expect(abs(calculateBatteryPct(p.v) - p.pct) <= 1, what);
  }
  expect(calculateBatteryPct(0.0f) == 100, "ไม่มีแบต (0 V ใช้ไฟ USB) -> 100%");
  int prev = -1; bool mono = true;
  for (float v = 3.0f; v <= 4.3f; v += 0.01f) { int p = calculateBatteryPct(v); if (p < prev) mono = false; prev = p; }
  expect(mono, "% ไม่ลดลงเมื่อแรงดันเพิ่ม (3.00-4.30 V)");
  return g_fail ? 1 : 0;
}
CPP
  g++ -std=gnu++17 -I../screen-preview -I"$d" -DESP_ARDUINO_VERSION_MAJOR=3 -w -o "$d/t" "$d/drv.cpp" && "$d/t" || rc=1
done

# ---- เครื่องประจำเตียงบอร์ดเล็ก: ฟังก์ชันของ C3 ใช้ตัวแปร batteryVolts ร่วม ----
echo "================ Station-C3-OLED"
d="$W/c3"; mkdir -p "$d"
cp ../../firmware/Station-C3-OLED/*.h "$d/"; cp ../../firmware/Station-C3-OLED/Station-C3-OLED.ino "$d/station_c3.cpp"
sed '/^int main(/,$d' ../c3-preview/driver.cpp > "$d/drv.cpp"
cat "$W/check.h" >> "$d/drv.cpp"
cat >> "$d/drv.cpp" <<'CPP'
int main() {
  char what[96];
  float got = readBattery();                     // ตัวขับให้ขาแบตคืน ADC 2450
  float want = analogReadMilliVolts(0) / 1000.0f * 2.0f;
  snprintf(what, sizeof(what), "ADC 2450 = %u mV ที่ขา -> แบต %.3f V", (unsigned)analogReadMilliVolts(0), got);
  expect(fabsf(got - want) < 0.002f, what);
  for (const Pt &p : PTS) {
    batteryVolts = p.v;
    snprintf(what, sizeof(what), "%.2f V -> %d%% (ตาราง %d%%)", p.v, (int)batteryPct(), p.pct);
    expect(abs((int)batteryPct() - p.pct) <= 1, what);
  }
  batteryVolts = 0.0f;
  expect(batteryPct() == 0, "ไม่มีแบต (เสียบ USB) -> 0% ตามเดิม");
  return g_fail ? 1 : 0;
}
CPP
g++ -std=gnu++17 -I../screen-preview -I"$d" -DESP_ARDUINO_VERSION_MAJOR=3 -w -o "$d/t" "$d/drv.cpp" && "$d/t" || rc=1

echo
[ $rc = 0 ] && echo "สูตรแบตเตอรี่ถูกต้องทุกเครื่อง" || echo "*** มีเครื่องที่ไม่ผ่าน ***"
exit $rc
