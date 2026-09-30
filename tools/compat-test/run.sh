#!/usr/bin/env bash
# ทดสอบความเข้ากันได้ของทั้งระบบ: เฟิร์มแวร์ Station ทุกสาย x Host ทุกรุ่น ด้วยโค้ดตัวจริงทั้งสองฝั่ง
#
# ต่อจาก protocol-test ซึ่งตรวจแค่ว่าโครงสร้างแพ็กเก็ตตรงกันทุกไบต์ ที่นี่ตรวจ "ความหมาย"
#   1) Station ตัวจริงเล่นฉาก (หยดผ่าน onDropDetected() และ loop() ตัวจริง) เก็บทุกไบต์ที่ส่ง
#   2) Host ตัวจริงรับไบต์เหล่านั้นผ่าน OnDataRecv() ตัดสินสัญญาณเตือนเอง แล้วเก็บ Sync ที่ส่งกลับ
#   3) Station ตัวจริงรับ Sync ตัวจริงของ Host นั้น แล้วดูว่าเข้าใจตรงกันไหม
# ทุกคู่ Station x Host ต้องผ่านทุกฉาก
set -uo pipefail
cd "$(dirname "$0")"
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
STATIONS="Station Station-Stable Station-Fallout Station-C3-OLED"
HOSTS="Host-OLED Host-TFT28 Host-Touch24"
rc=0

build_station() {   # $1 = โฟลเดอร์สเก็ตช์
  local name="$1" dir="../../firmware/$1" d="$W/src_st_$1"; mkdir -p "$d"
  for h in "$dir"/*.h; do cp "$h" "$d/"; done
  if [ "$name" = "Station-C3-OLED" ]; then
    cp "$dir/$name.ino" "$d/station_c3.cpp"
    sed '/^int main(/,$d' ../c3-preview/driver.cpp > "$d/drv.cpp"
    cat >> "$d/drv.cpp" <<'CPP'
#define NOW_MS()        (g_micros / 1000UL)
#define ADVANCE_TO(ms)  (g_micros = (unsigned long)(ms) * 1000UL)
#define STATION_BOOT()  do { g_adc = BASE_ADC; g_micros = 3000UL * 1000UL; setup(); } while (0)
#define MY_ID           stationId
#define SYNC_CB         OnHostSyncRecv
#define TICK()          do { g_adc = BASE_ADC; loop(); } while (0)
#define DROP()          onDropDetected()
CPP
  else
    cp "$dir/$name.ino" "$d/station.cpp"
    sed '/^int main(/,$d' ../station-preview/driver.cpp > "$d/drv.cpp"
    cat >> "$d/drv.cpp" <<'CPP'
#define NOW_MS()        (g_millis)
#define ADVANCE_TO(ms)  (g_millis = (ms))
#define STATION_BOOT()  setup()
#define MY_ID           currentStationId
#define SYNC_CB         OnTimeSyncRecv
#define TICK()          loop()
CPP
    # สายเสถียรใช้อัลกอริทึมเดิม ฟังก์ชันรับหยดชื่อ registerDrop(เวลา)
    if [ "$name" = "Station-Stable" ]; then echo '#define DROP() registerDrop(g_millis)' >> "$d/drv.cpp"
    else echo '#define DROP() onDropDetected()' >> "$d/drv.cpp"; fi
  fi
  # เก็บทุกแพ็กเก็ตที่ส่งออก แทน esp_now_send() เปล่า ๆ ของตัวขับเรนเดอร์
  sed -i 's/^int esp_now_send(const uint8_t\*,const uint8_t\*,int){return 0;}/void capture_send(const uint8_t*,int); int esp_now_send(const uint8_t*,const uint8_t* d,int n){capture_send(d,n);return 0;}/' "$d/drv.cpp"
  cat station_main.cpp >> "$d/drv.cpp"
  cp scenarios.h "$d/"
  g++ -std=gnu++17 -I../screen-preview -I"$d" -DESP_ARDUINO_VERSION_MAJOR=3 -w -o "$W/st_$name" "$d/drv.cpp" || { echo "คอมไพล์ $name ไม่ผ่าน"; exit 1; }
}

build_host() {      # $1 = โฟลเดอร์สเก็ตช์
  local name="$1" dir="../../firmware/$1" d="$W/src_h_$1"; mkdir -p "$d"
  for h in "$dir"/*.h; do cp "$h" "$d/"; done
  cp "$dir/$name.ino" "$d/host_oled.cpp"
  { echo 'void capture_send(const unsigned char*, int);'
    sed '/^int main() {/,$d' ../host-rx-test/driver.cpp | sed 's/^  g_sendCount++;/  g_sendCount++; capture_send(d, n);/'
    if [ "$name" = "Host-TFT28" ]; then echo '#define SYNC_ALL() broadcastSyncToNodes()'
    else echo '#define SYNC_ALL() do { for (int i_ = 0; i_ < activeStationCount; i_++) sendSyncForStation(i_); } while (0)'; fi
    cat host_main.cpp; } > "$d/drv.cpp"
  cp scenarios.h "$d/"
  g++ -std=gnu++17 -I../screen-preview -I"$d" -DESP_ARDUINO_VERSION_MAJOR=3 -w -o "$W/h_$name" "$d/drv.cpp" || { echo "คอมไพล์ $name ไม่ผ่าน"; exit 1; }
}

for s in $STATIONS; do build_station "$s"; done
for h in $HOSTS; do build_host "$h"; done

val() { echo "$1" | tr ' ' '\n' | sed -n "s/^$2=//p"; }
ALERT=(NONE TOO_FAST TOO_SLOW NEAR_END COMPLETE OCCLUSION NO_SIGNAL LOST_LINK)

# ---- ฉากที่ต้องมีทั้งสองฝั่ง ----
for sc in normal fast maxrate nearend; do
  case $sc in
    normal)  want=0; what="ไหลปกติ 100 mL/h: ไม่มีเตือน ทั้งสองฝั่งเห็นกันและกัน" ;;
    fast)    want=1; what="ไหลอิสระ 3600 mL/h (DF10): ต้องเตือน TOO FAST ทั้งสองฝั่ง" ;;
    maxrate) want=1; what="หยดห่าง 70 ms (DF10) 5143 mL/h ใกล้ขีดที่ Station นับได้: ยังต้องเตือน TOO FAST" ;;
    nearend) want=3; what="เกิน 80% ของแผน: ต้องเตือน NEAR END ทั้งสองฝั่ง" ;;
  esac
  echo "================ ฉาก $sc — $what"
  printf "  %-16s %-13s %-34s %-34s %s\n" "Station" "Host" "Host เห็น" "Station เห็น" "ผล"
  for s in $STATIONS; do
    "$W/st_$s" gen "$sc" "$W/pk_$s.txt" >/dev/null
    for h in $HOSTS; do
      hr=$("$W/h_$h" "$sc" "$W/pk_$s.txt" "$W/sy_${s}_${h}.txt")
      sr=$("$W/st_$s" apply "$sc" "$W/sy_${s}_${h}.txt" "$W/pk2.txt")
      ho=$(val "$hr" h_online); ha=$(val "$hr" h_alert); hrate=$(val "$hr" h_rate)
      sl=$(val "$sr" st_link_ok); sa=$(val "$sr" st_hostalert); st=$(val "$sr" st_target)
      ok=1
      [ "$ho" = 1 ] || ok=0; [ "$ha" = "$want" ] || ok=0
      [ "$sl" = 1 ] || ok=0; [ "$sa" = "$want" ] || ok=0; [ "$st" = 100 ] || ok=0
      hv="$( [ "$ho" = 1 ] && echo ONLINE || echo OFFLINE ) ${ALERT[$ha]} ${hrate}mL/h"
      sv="$( [ "$sl" = 1 ] && echo LINK_OK || echo NOT_OK ) ${ALERT[$sa]} SET ${st}"
      printf "  %-16s %-13s %-34s %-34s %s\n" "$s" "$h" "$hv" "$sv" "$( [ $ok = 1 ] && echo ผ่าน || echo ไม่ผ่าน )"
      [ $ok = 1 ] || rc=1
    done
  done
done

# ---- ฉากฝั่ง Station อย่างเดียว: บูตใหม่ระหว่างให้น้ำเกลือ ----
echo "================ ฉาก reboot — Station บูตใหม่ ยอดหยดที่กู้คืนต้องไม่ถูกล้าง"
echo "  (Host เก็บ resetSeq ลง NVS หลังกดเริ่มถุงใหม่ครั้งแรก ค่าจึงไม่ใช่ 0 ตลอดไป)"
for s in $STATIONS; do
  sr=$("$W/st_$s" gen reboot "$W/pk_r.txt")
  d=$(val "$sr" st_drops)
  if [ "$d" -ge 12400 ]; then r=ผ่าน; else r=ไม่ผ่าน; rc=1; fi
  printf "  %-16s กู้คืน 12400 หยด  หลังรับ Sync เหลือ %-6s %s\n" "$s" "$d" "$r"
done

echo "================ ฉาก newbag — กด \"เริ่มถุงใหม่\" ที่ Host กลางฉาก ตัวนับต้องเริ่มใหม่จริง"
for s in $STATIONS; do
  sr=$("$W/st_$s" gen newbag "$W/pk_r.txt")
  d=$(val "$sr" st_drops)
  if [ "$d" -lt 100 ]; then r=ผ่าน; else r=ไม่ผ่าน; rc=1; fi
  printf "  %-16s ก่อนกด 12400 หยด  หลังกดเหลือ %-6s %s\n" "$s" "$d" "$r"
done

echo "================ ฉาก slowdown / speedup — หมุนตัวปรับหยด 4 เท่า จอต้องขึ้นอัตราใหม่ทันทีหลังช่วงแรก"
for sc in slowdown speedup; do
  case $sc in slowdown) want=25 ;; speedup) want=400 ;; esac
  for s in $STATIONS; do
    sr=$("$W/st_$s" gen $sc "$W/pk_r.txt")
    r=$(val "$sr" st_probe)
    ok=$(python3 -c "print(1 if abs($r-$want) <= $want*0.1 else 0)")
    [ "$ok" = 1 ] && res=ผ่าน || { res=ไม่ผ่าน; rc=1; }
    printf "  %-9s %-16s อัตราจริง %4d  จอขึ้น %7s mL/h  %s\n" "$sc" "$s" "$want" "$r" "$res"
  done
done

echo
[ $rc = 0 ] && echo "เข้ากันได้ทุกคู่ ทุกฉาก" || echo "*** มีคู่ที่ไม่ผ่าน ***"
exit $rc
