// ฝั่ง Station ของการทดสอบความเข้ากันได้ ต่อท้ายตัวขับเรนเดอร์ของสายนั้น (ตัด main() เดิมทิ้ง)
// ใช้: st gen   <ฉาก> <ไฟล์แพ็กเก็ตขาออก>                 Sync จำลอง แล้วเก็บแพ็กเก็ตที่ Station ส่งจริง
//      st apply <ฉาก> <ไฟล์ Sync จาก Host> <ไฟล์แพ็กเก็ตขาออก>  ใช้ Sync ตัวจริงของ Host
// พิมพ์สถานะสุดท้ายของ Station เป็น key=value
#include "scenarios.h"
#include <vector>
#include <string>

static FILE *g_cap = nullptr;
static unsigned long g_t0 = 0;

void capture_send(const uint8_t *d, int n) {
  if (!g_cap || n != (int)sizeof(struct_message)) return;
  fprintf(g_cap, "%lu", NOW_MS() - g_t0);
  for (int i = 0; i < n; i++) fprintf(g_cap, " %02X", d[i]);
  fprintf(g_cap, "\n");
}

struct Pkt { unsigned long t; std::vector<uint8_t> b; };
static std::vector<Pkt> readPkts(const char *path) {
  std::vector<Pkt> v; FILE *f = fopen(path, "r"); if (!f) return v;
  char line[512];
  while (fgets(line, sizeof(line), f)) {
    Pkt p; char *s = line; p.t = strtoul(s, &s, 10);
    while (*s) { char *e; unsigned long x = strtoul(s, &e, 16); if (e == s) break; p.b.push_back((uint8_t)x); s = e; }
    v.push_back(p);
  }
  fclose(f); return v;
}

static void deliver(const uint8_t *b, int n) {
  esp_now_recv_info_t info = {}; wifi_pkt_rx_ctrl_t c = {}; c.rssi = -60; info.rx_ctrl = &c;
  uint8_t mac[6] = {0x24, 0x6F, 0x28, 0x00, 0x00, 0x01}; info.src_addr = mac;
  SYNC_CB(&info, b, n);
}

int main(int argc, char **argv) {
  if (argc < 4) return 2;
  bool apply = !strcmp(argv[1], "apply");
  const Scenario &sc = findScenario(argv[2]);
  std::vector<Pkt> syncs; if (apply) syncs = readPkts(argv[3]);
  g_ops = fopen("/dev/null", "w");
  STATION_BOOT();
  MY_ID = BED;
  // ยอดหยดที่กู้คืนหลังบูต (RTC/NVS) แบบเดียวกับ tallyRestore() ใน setup()
  totalDrops = (uint32_t)(sc.startMl * sc.df);
  g_cap = fopen(argv[apply ? 4 : 3], "w");
  g_t0 = NOW_MS();
  size_t si = 0; unsigned long nextDrop = sc.firstDrop;
  float probeRate = -1;
  for (unsigned long t = 0; t <= sc.dur; t += 10) {
    ADVANCE_TO(g_t0 + t);
    if (!apply && t % 1000 == 0) {
      struct_host_sync s; memset(&s, 0, sizeof(s));
      s.stationId = BED; s.targetRateHr = sc.target; s.totalPlanMl = sc.plan; s.dropFactor = sc.df;
      s.nearEndPct = 80; s.hostChannel = 1; s.flags = 0x06;
      s.resetSeq = (unsigned char)(sc.resetSeq + ((sc.newBagAt && t >= sc.newBagAt) ? 1 : 0));
      deliver((const uint8_t *)&s, sizeof(s));
    }
    while (apply && si < syncs.size() && syncs[si].t <= t) { deliver(syncs[si].b.data(), (int)syncs[si].b.size()); si++; }
    if (sc.gap && t >= nextDrop && t < sc.dropsUntil) {
      DROP();
      nextDrop += (sc.gap2From && t >= sc.gap2From) ? sc.gap2 : sc.gap;
    }
    TICK();
    if (sc.probeAt && t == sc.probeAt) probeRate = currentFlowRate_ml_hr;
  }
  fclose(g_cap);
  printf("st_drops=%lu st_target=%.0f st_plan=%.0f st_df=%d st_hostalert=%d st_link_ok=%d st_rate=%.0f\n",
         (unsigned long)totalDrops, targetRateHr, totalPlanMl, (int)dropFactor, (int)hostAlertCode,
         linkState() == LINK_OK ? 1 : 0, currentFlowRate_ml_hr);
  printf("st_probe=%.1f\n", probeRate);
  return 0;
}
