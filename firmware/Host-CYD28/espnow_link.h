/**
 * @file      espnow_link.h
 * @brief     ESP-NOW: รับแพ็กเก็ตจาก Station เข้าคิว และส่ง Sync แบบกระจาย
 * @version   2.0.3-CYD
 * @date      2026-11-27
 * @author    นายกิตติพันธ์ รัตนคร <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * วิทยาลัยพยาบาลบรมราชชนนี แพร่ คณะพยาบาลศาสตร์ สถาบันพระบรมราชชนก
 * หอผู้ป่วยอายุรกรรม โรงพยาบาลสูงเม่น จังหวัดแพร่
 *
 * @par Description
 * callback ของ ESP-NOW ทำงานบนงานของ Wi-Fi จึงแค่ใส่คิววงแหวน (ผู้เขียนคนเดียว ผู้อ่านคนเดียว) แล้วให้ loop() ประมวลผลเอง ไม่แชร์โครงสร้างข้อมูลข้ามงาน ช่องสัญญาณตั้งตายตัวที่ CYD_ESPNOW_CHANNEL (Station ค้นหาช่องเอง) ส่งด้านเดียว ไม่มี ACK ไม่เข้ารหัส เหมือน Protocol v3
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 2.0.3-CYD | 2026-11-27 | linkBegin(withAp) เปิดจุดเชื่อมต่อ Wi-Fi ของตัวเองบนช่องเดียวกับ ESP-NOW เพื่อให้หน้าเว็บใช้งานได้ |
 * | 2.0.1-CYD | 2026-11-27 | เขียนใหม่ด่านที่ 2 (แกนเครือข่าย) ไม่นำโค้ดรุ่น 1.x มาปรับ |
 *
 * @warning  ยังไม่ได้ทดสอบบนฮาร์ดแวร์จริง ตรวจด้วยเครื่องมือจำลองบนเครื่อง PC เท่านั้น
 */
#pragma once
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>
#include "protocol.h"
#include "board.h"

#ifndef CYD_ESPNOW_CHANNEL
#define CYD_ESPNOW_CHANNEL 1
#endif

struct RxPacket {
  struct_message m;
  int8_t   rssi;
  uint8_t  mac[6];
};

constexpr uint8_t RX_QUEUE_LEN = 16;                 // ต้องเป็นยกกำลังสอง
static RxPacket rxQueue[RX_QUEUE_LEN];
static volatile uint8_t rxHead = 0, rxTail = 0;
static volatile uint32_t rxDropped = 0, rxWrongLen = 0;
static const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint32_t syncSendFail = 0;

static void rxPush(const uint8_t *src, const uint8_t *data, int len, int8_t rssi) {
  if (len != (int)sizeof(struct_message)) { rxWrongLen++; return; }   // แพ็กเก็ต Sync ของ Host อื่นหรือของแปลกปลอม
  const uint8_t next = (uint8_t)((rxHead + 1) & (RX_QUEUE_LEN - 1));
  if (next == rxTail) { rxDropped++; return; }                          // คิวเต็ม ทิ้งใบใหม่ (ใบถัดไปมาในอีก 1 วินาที)
  RxPacket &p = rxQueue[rxHead];
  memcpy(&p.m, data, sizeof(struct_message));
  p.rssi = rssi;
  if (src) memcpy(p.mac, src, 6); else memset(p.mac, 0, 6);
  rxHead = next;
}

#if defined(ESP_IDF_VERSION) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  rxPush(info ? info->src_addr : nullptr, data, len, (info && info->rx_ctrl) ? (int8_t)info->rx_ctrl->rssi : (int8_t)-60);
}
#else
static void onEspNowRecv(const uint8_t *mac, const uint8_t *data, int len) {
  rxPush(mac, data, len, -60);                          // core รุ่นเก่าไม่ส่ง RSSI มาให้ใน callback
}
#endif

// ดึงแพ็กเก็ตถัดไปออกจากคิว (เรียกจาก loop() เท่านั้น)
static bool rxPop(RxPacket &out) {
  if (rxTail == rxHead) return false;
  out = rxQueue[rxTail];
  rxTail = (uint8_t)((rxTail + 1) & (RX_QUEUE_LEN - 1));
  return true;
}

// withAp = true: เปิดจุดเชื่อมต่อ Wi-Fi ของ Host (ใช้กับหน้าเว็บ) ช่องของจุดเชื่อมต่อเป็นตัวกำหนดช่อง ESP-NOW
static bool linkBegin(bool withAp = false) {
  WiFi.mode(withAp ? WIFI_AP_STA : WIFI_STA);
  WiFi.disconnect();
  WiFi.setSleep(false);
  if (withAp) WiFi.softAP(CYD_AP_SSID, CYD_AP_PASS, CYD_ESPNOW_CHANNEL, 0, 4);
  esp_wifi_set_channel(CYD_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(esp_now_recv_cb_t(onEspNowRecv));
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BROADCAST_MAC, 6);
  peer.channel = 0;                                     // 0 = ใช้ช่องปัจจุบัน
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  return esp_now_add_peer(&peer) == ESP_OK;
}

static uint8_t linkChannel() {
  uint8_t ch = CYD_ESPNOW_CHANNEL;
  wifi_second_chan_t sc;
  esp_wifi_get_channel(&ch, &sc);
  return ch;
}

static void linkSendSync(const struct_host_sync &s) {
  if (esp_now_send(BROADCAST_MAC, (const uint8_t *)&s, sizeof(s)) != ESP_OK) syncSendFail++;   // ไม่ delay/ไม่ส่งซ้ำ รอบถัดไปมาเอง
}
