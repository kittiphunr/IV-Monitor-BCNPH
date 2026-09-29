// ฝั่ง Host ของการทดสอบความเข้ากันได้ ต่อท้ายตัวขับของ host-rx-test (ตัด main() เดิมทิ้ง)
// ใช้: host <ฉาก> <ไฟล์แพ็กเก็ตจาก Station> <ไฟล์ Sync ขาออก>
// ป้อนแพ็กเก็ตที่ Station ตัวจริงส่ง ตามเวลาเดิม ให้ Host ตัดสินสัญญาณเตือนเองทุก 100 ms
// และส่ง Sync ทุก 1 วินาที เก็บ Sync ของเตียงนี้ไว้ป้อนกลับให้ Station
#include "scenarios.h"
#include <vector>

static FILE *g_scap = nullptr;
static unsigned long g_t0 = 0;
static int g_seen = 0;

void capture_send(const uint8_t *d, int n) {
  if (!g_scap || n != (int)sizeof(struct_host_sync)) return;
  struct_host_sync s; memcpy(&s, d, n);
  if (s.stationId != BED) return;
  fprintf(g_scap, "%lu", g_millis - g_t0);
  for (int i = 0; i < n; i++) fprintf(g_scap, " %02X", d[i]);
  fprintf(g_scap, "\n");
}

struct Pkt { unsigned long t; std::vector<uint8_t> b; };

int main(int argc, char **argv) {
  if (argc < 4) return 2;
  const Scenario &sc = findScenario(argv[1]);
  std::vector<Pkt> pk;
  FILE *f = fopen(argv[2], "r"); char line[512];
  while (f && fgets(line, sizeof(line), f)) {
    Pkt p; char *s = line; p.t = strtoul(s, &s, 10);
    while (*s) { char *e; unsigned long x = strtoul(s, &e, 16); if (e == s) break; p.b.push_back((uint8_t)x); s = e; }
    pk.push_back(p);
  }
  if (f) fclose(f);
  g_scap = fopen(argv[3], "w");
  g_millis = 100000; g_t0 = g_millis;
  activeStationCount = BED;
  for (int i = 0; i < MAX_SUPPORTED_STATIONS; i++) {
    stations[i].cfg.dropFactor = sc.df; stations[i].cfg.targetRateHr = sc.target;
    stations[i].cfg.planVolumeMl = sc.plan; stations[i].cfg.resetSeq = sc.resetSeq;
    stations[i].nearEndPct = 80;
  }
  uint8_t mac[6] = {0x24, 0x6F, 0x28, 0x00, 0x00, 0x33};
  size_t k = 0;
  for (unsigned long t = 0; t <= sc.dur; t += 10) {
    if (g_millis < g_t0 + t) g_millis = g_t0 + t;
    while (k < pk.size() && pk[k].t <= t) {
      esp_now_recv_info_t info = {}; wifi_pkt_rx_ctrl_t c = {}; c.rssi = -60;
      info.rx_ctrl = &c; info.src_addr = mac;
      OnDataRecv(&info, pk[k].b.data(), (int)pk[k].b.size());
      k++; g_seen++;
    }
    if (t % 100 == 0) evaluateClinicalAlerts();
    if (t % 1000 == 500) SYNC_ALL();
  }
  fclose(g_scap);
  StationData &s = stations[BED - 1];
  printf("h_pkts=%d h_online=%d h_drops=%lu h_rate=%.0f h_alert=%d h_vol=%.0f\n",
         g_seen, isStationOnline(BED - 1) ? 1 : 0, (unsigned long)s.totalDrops, s.flowRate_ml_hr,
         (int)s.alertCode, s.totalVolumeMl);
  return 0;
}
