#pragma once
// ตัวแทนของ esp_task_wdt.h สำหรับการจำลองบนเครื่อง PC
// บนเครื่อง PC ไม่มีสุนัขเฝ้าบ้าน ทุกฟังก์ชันจึงเป็นการเรียกเปล่า
// แต่ต้องมีให้ครบ เพื่อให้โค้ดชุดเดียวกับที่ลงบอร์ดจริงคอมไพล์ผ่านโดยไม่ต้องมี #ifdef
typedef struct {
  unsigned int timeout_ms;
  unsigned int idle_core_mask;
  bool trigger_panic;
} esp_task_wdt_config_t;

#ifndef ESP_ERR_INVALID_STATE
#define ESP_ERR_INVALID_STATE 0x103
#endif

// ตัวนับสำหรับชุดทดสอบ (ไม่กระทบโค้ดที่ไม่สนใจ): นับจำนวนครั้งที่สมัคร/ถอน/ป้อน
// และจับช่วงเวลาที่นานที่สุดที่ไม่มีการป้อน (ตามนาฬิกาจำลอง millis()) เพื่อพิสูจน์ว่าไม่เกิน timeout
unsigned long millis();
struct WdtStats {
  int adds = 0, deletes = 0, resets = 0, inits = 0;
  bool registered = false;
  unsigned long lastFeedMs = 0, maxGapMs = 0;
};
inline WdtStats& wdtStats() { static WdtStats s; return s; }
inline void wdtStatsReset() { wdtStats() = WdtStats(); }
inline void wdtNoteFeed_() {
  WdtStats& s = wdtStats(); unsigned long now = millis();
  if (s.registered && now - s.lastFeedMs > s.maxGapMs) s.maxGapMs = now - s.lastFeedMs;
  s.lastFeedMs = now;
}
// ช่วงที่นานที่สุดนับถึง "ตอนนี้" ด้วย เรียกหลัง setup() จบเพื่อรวมช่วงท้าย
inline unsigned long wdtMaxGapNow() {
  WdtStats& s = wdtStats(); unsigned long tail = s.registered ? millis() - s.lastFeedMs : 0;
  return tail > s.maxGapMs ? tail : s.maxGapMs;
}

inline int esp_task_wdt_init(const esp_task_wdt_config_t*) { wdtStats().inits++; return 0; }
inline int esp_task_wdt_init(unsigned int, bool)           { wdtStats().inits++; return 0; }
inline int esp_task_wdt_reconfigure(const esp_task_wdt_config_t*) { return 0; }
inline int esp_task_wdt_add(void*)    { wdtStats().adds++; wdtNoteFeed_(); wdtStats().registered = true; wdtStats().lastFeedMs = millis(); return 0; }
inline int esp_task_wdt_delete(void*) { wdtStats().deletes++; wdtStats().registered = false; return 0; }
inline int esp_task_wdt_reset()       { wdtStats().resets++; wdtNoteFeed_(); return 0; }
