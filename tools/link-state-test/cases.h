// กรณีทดสอบไฟสถานะการเชื่อมต่อแบบสองทาง — ใช้ร่วมกันทั้ง Station / Station-Stable / C3
//
// ป้อน Sync ของ Host เข้าคอลแบ็กตัวจริงของเฟิร์มแวร์ แล้วถาม linkState() ว่าจอจะบอกอะไร
// กรณีที่ 3 คืออาการในภาพถ่ายของผู้ใช้ตรงตัว: เตียงได้ยิน Host แต่ Host ไม่ได้ยินเตียง
//
// ตัวขับของแต่ละรุ่นต้องนิยามก่อน include ไฟล์นี้
//   MY_ID            ตัวแปรเลขเตียงของเฟิร์มแวร์นั้น
//   SYNC_CB          คอลแบ็กรับ Sync
//   ADVANCE(ms)      เดินนาฬิกาจำลอง
//   TICK()           ให้เฟิร์มแวร์เดินหนึ่งรอบ (Station ต้องเรียก loop() เพื่อตัดสิน timeout)
//   RESET_LINK()     ล้างสถานะการเชื่อมต่อให้เหมือนเพิ่งเปิดเครื่อง
#include <cstring>

static int g_fail = 0;

static const char* linkName(uint8_t s) {
  switch (s) {
    case LINK_NONE:       return "NO LINK";
    case LINK_OK:         return "ONLINE";
    case LINK_NOT_HEARD:  return "ONE-WAY (host can't hear)";
    case LINK_NOT_LISTED: return "ONE-WAY (bed not on host)";
    default:              return "?";
  }
}

static void expect(const char* what, uint8_t want) {
  uint8_t got = linkState();
  bool ok = (got == want);
  if (!ok) g_fail++;
  printf("  %-52s %-26s %s\n", what, linkName(got), ok ? "ผ่าน" : "*** ไม่ผ่าน ***");
}

static void feedSync(uint8_t id, uint8_t flags) {
  struct_host_sync s;
  memset(&s, 0, sizeof(s));
  s.stationId  = id;
  s.flags      = flags;
  s.dropFactor = 20;
  s.nearEndPct = 80;
  s.hostChannel = 1;
  esp_now_recv_info_t info;
  memset(&info, 0, sizeof(info));
  SYNC_CB(&info, (const uint8_t*)&s, (int)sizeof(s));
}

// Host จำลอง: วนส่ง Sync ให้เตียง 1..beds ทุก 1 วินาที (กระจายเป็นช่องละ 125 ms)
struct FakeHost {
  bool   on       = true;
  int    beds     = 7;
  bool   modern   = true;   // false = Host รุ่นเก่า ไม่ส่งบิตรายงาน
  bool   hearsMe  = true;
};

static void runFor(FakeHost &h, unsigned long ms) {
  for (unsigned long t = 0; t < ms; t += 125) {
    static int slot = 0;
    if (h.on) {
      int id = (slot % 8) + 1;
      if (id <= h.beds) {
        uint8_t f = 0;
        if (h.modern) {
          f |= 0x04;
          // Host ตัดสิน "ได้ยิน" ของแต่ละเตียงแยกกัน เตียงอื่นได้ยินเสมอในการทดสอบนี้
          if (id != MY_ID || h.hearsMe) f |= 0x02;
        }
        feedSync((uint8_t)id, f);
      }
    }
    slot++;
    ADVANCE(125);
    TICK();
  }
}

static int runCases() {
  MY_ID = 2;   // เตียง 2 เหมือนในภาพถ่าย

  printf("\nกรณี 1: ยังไม่ได้ยิน Host เลย\n");
  RESET_LINK();
  expect("เพิ่งเปิดเครื่อง", LINK_NONE);

  printf("\nกรณี 2: Host รุ่นใหม่ ได้ยินกันสองทาง\n");
  { FakeHost h; RESET_LINK(); runFor(h, 12000); expect("หลัง 12 วินาที", LINK_OK); }

  printf("\nกรณี 3: เตียงได้ยิน Host แต่ Host ไม่ได้ยินเตียง (อาการในภาพ)\n");
  { FakeHost h; h.hearsMe = false; RESET_LINK();
    runFor(h, 2000);  expect("2 วินาทีแรก ยังอยู่ในช่วงผ่อนผัน", LINK_OK);
    runFor(h, 10000); expect("หลัง 12 วินาที", LINK_NOT_HEARD);
    h.hearsMe = true;
    runFor(h, 2000);  expect("Host กลับมาได้ยินแล้ว", LINK_OK); }

  printf("\nกรณี 4: Host รุ่นเก่า (ไม่มีบิตรายงาน) ต้องไม่เตือนผิด\n");
  { FakeHost h; h.modern = false; RESET_LINK(); runFor(h, 12000); expect("หลัง 12 วินาที", LINK_OK); }

  printf("\nกรณี 5: Host เปิดไว้แค่ 1 เตียง แต่เครื่องนี้ตั้งเป็นเตียง 2\n");
  { FakeHost h; h.beds = 1; RESET_LINK(); runFor(h, 12000); expect("หลัง 12 วินาที", LINK_NOT_LISTED); }

  printf("\nกรณี 6: Host ดับไปทั้งเครื่อง\n");
  { FakeHost h; RESET_LINK(); runFor(h, 6000); h.on = false; runFor(h, 12000);
    expect("หลัง Host ดับ 12 วินาที", LINK_NONE); }

  printf("\n%s (%d ข้อที่ไม่ผ่าน)\n", g_fail ? "มีข้อที่ไม่ผ่าน" : "ผ่านทั้งหมด", g_fail);
  return g_fail ? 1 : 0;
}
