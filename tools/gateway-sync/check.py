# -*- coding: utf-8 -*-
"""ตรวจว่าตรรกะที่ Host-Gateway ยกมาจาก Host-OLED ยังเหมือนต้นฉบับทุกบรรทัด

Host-Gateway แตกจาก Host-OLED จึงมีโค้ดสองสำเนา ถ้าแก้ตรรกะเตือนหรือการรับแพ็กเก็ตฝั่งเดียว
สองเครื่องจะตัดสินเตือนไม่เหมือนกันโดยไม่มีใครรู้ (เครื่องหนึ่งเตือน อีกเครื่องเงียบ)
ตัวตรวจนี้เทียบ "ตัวฟังก์ชัน" ที่ต้องเหมือนกัน และโครงสร้างแพ็กเก็ต/รหัสเตือน/ค่าคงที่ที่ใช้ตัดสินเตือน

ฟังก์ชันที่ Host-Gateway แก้ไว้ตั้งใจ (เครือข่าย เวลา คำสั่ง) ไม่อยู่ในรายการที่ต้องเหมือน
ใช้:  bash tools/gateway-sync/run.sh
"""
import io, os, re, sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'firmware')
OLED = os.path.join(ROOT, 'Host-OLED')
GW = os.path.join(ROOT, 'Host-Gateway')

# ฟังก์ชันที่ตัดสินเตือน รับ/ส่งแพ็กเก็ต หรือวาดจอ ต้องเหมือนต้นฉบับทุกบรรทัด
SHARED_INO = [
    'OnDataRecv', 'evaluateClinicalAlerts', 'sendSyncForStation', 'serviceSyncScheduler',
    'serviceLinkQuality', 'requestSyncNow', 'requestSyncAll', 'syncSlotIntervalMs',
    'isStationOnline', 'isStationLostLink', 'isStationNoSignal', 'hasIdConflict', 'anyIdConflict',
    'isStationOccluded', 'occlusionThresholdMs', 'heardStationId', 'heardBeyondBedCount',
    'safeDropFactor', 'safeNearPct', 'alertCodeForStation', 'isCriticalAlert', 'isSnoozed',
    'alertTextEn', 'handleBuzzerAlarm', 'handleNearEndChime', 'anyNearEndPending',
    'acknowledgeAllNearEnd', 'dripAnimPhase', 'anyBedDripping', 'bedNeedsAttention',
    'macTail', 'jsonEscape', 'readHostBattery', 'calculateBatteryPct',
    'checkPageButton', 'checkSmartphonePowerButton', 'loadBedConfigs', 'saveBedConfig',
]
# ใน HostScreen.h ฟังก์ชันที่ Host-Gateway แก้ไว้ตั้งใจ (ที่เหลือต้องเหมือน)
SCREEN_DIFFERS = {'updateHostOLED', 'drawTopBar'}

# บล็อกนิยามที่ต้องเหมือนกัน: (ชื่อ, regex ที่ใช้หาจุดเริ่ม, จุดสิ้นสุด)
SHARED_BLOCKS = [
    ('struct_message', r'typedef struct __attribute__\(\(packed\)\) struct_message \{', r'\} struct_message;'),
    ('struct_host_sync', r'typedef struct __attribute__\(\(packed\)\) struct_host_sync \{', r'\} struct_host_sync;'),
    ('BedConfig', r'struct BedConfig \{', r'^\};'),
]
SHARED_DEFINES = [
    'ALERT_NONE', 'ALERT_TOO_FAST', 'ALERT_TOO_SLOW', 'ALERT_NEAR_END', 'ALERT_COMPLETE',
    'ALERT_OCCLUSION', 'ALERT_NO_SIGNAL', 'ALERT_LOST_LINK', 'ESPNOW_CHANNEL', 'SYNC_INTERVAL_MS',
    'SYNC_SLOT_MIN_MS', 'ONLINE_TIMEOUT_MS', 'LOST_LINK_GRACE_MS', 'MAX_PLAUSIBLE_RATE_HR',
    'MAX_PLAUSIBLE_BATT_V', 'LINK_WINDOW_MS', 'LINK_WEAK_PCT', 'ID_CONFLICT_WINDOW_MS',
    'ID_CONFLICT_MIN_FLIPS', 'HEARD_REMEMBER_MS', 'MIN_OCCLUSION_MS', 'NO_SIGNAL_MS',
    'MAX_OCCLUSION_MS', 'RATE_DEVIATION_HOLD_MS', 'SNOOZE_MS', 'NEAR_END_REMIND_MS',
    'DEFAULT_NEAR_END_PCT', 'MAX_SUPPORTED_STATIONS', 'OLED_ANIM_INTERVAL_MS', 'OLED_IDLE_INTERVAL_MS',
    'DROP_FALL_MS', 'HOST_BAT_ADC_PIN', 'POWER_BTN_PIN', 'PAGE_BTN_PIN', 'BUZZER_PIN',
    'OLED_SDA_PIN', 'OLED_SCL_PIN', 'OLED_I2C_HZ',
]

# Host-TFT-Gateway ต้องเหมือน Host-TFT28 ในตรรกะเตือน/ปุ่ม/แบตเตอรี่/การตัดสินสถานะเตียง
TFT_SHARED_WITH_TFT28 = [
    'evaluateClinicalAlerts', 'isStationOnline', 'isStationLostLink', 'occlusionThresholdMs',
    'isStationOccluded', 'alertCodeForStation', 'isCriticalAlert', 'safeDropFactor', 'safeNearPct',
    'isSnoozed', 'alertTextEn', 'handleBuzzerAlarm', 'handleNearEndChime', 'anyNearEndPending',
    'acknowledgeAllNearEnd', 'bedUiStatus', 'bedPct', 'bedMinutesLeft', 'autoFocusBed',
    'currentFocusBed', 'readHostBattery', 'calculateBatteryPct', 'checkPageButton',
    'checkSmartphonePowerButton', 'loadBedConfigs', 'saveBedConfig', 'updateMainScreen',
    'updateHostDisplay', 'resetUiCaches',
]
# ส่วนลิงก์พีซีต้องเหมือน Host-Gateway (ข้อความและคำสั่งฝั่งพีซีใช้เซิร์ฟเวอร์ตัวเดียวกัน)
TFT_SHARED_WITH_GW = [
    'urlDecode', 'cmdArg', 'enqueueCmd', 'parsePcReply', 'applyStationConfig', 'applyStationAck',
    'applyPcTime', 'serviceCommands', 'pushToPc', 'uplinkTask', 'handlePortalRoot', 'handlePortalSave',
    'htmlEscape', 'startSetupPortal', 'startStaMode', 'onStaConnected', 'serviceWifi', 'forceRadioAwake',
    'attachEspNowPeer', 'loadWifiConfig', 'saveWifiConfig', 'savePcConfig', 'clearWifiConfig', 'jsonEscape',
    'gatewayLine', 'syncSlotIntervalMs', 'requestSyncNow', 'requestSyncAll',
    'serviceLinkQuality',
]

START = re.compile(r'^[A-Za-z_][\w:<>,\*& ]*?\b(\w+)\s*\([^;]*\)\s*\{\s*$')


def strip_comments(s):
    s = re.sub(r'/\*.*?\*/', '', s, flags=re.S)
    s = re.sub(r'//[^\n]*', '', s)
    return '\n'.join(l.rstrip() for l in s.split('\n') if l.strip())


def functions(text):
    """{ชื่อ: [เนื้อฟังก์ชัน, ...]} ฟังก์ชันระดับบนสุดที่ปีกกาเปิดท้ายบรรทัดและปีกกาปิดอยู่คอลัมน์แรก"""
    out, lines, i = {}, text.split('\n'), 0
    while i < len(lines):
        m = START.match(lines[i])
        if m and not lines[i].startswith((' ', '\t', '#', '//')):
            j = i
            while j < len(lines) and lines[j] != '}':
                j += 1
            out.setdefault(m.group(1), []).append(strip_comments('\n'.join(lines[i:j + 1])))
            i = j
        i += 1
    return out


def block(text, start, end):
    m = re.search(start, text, re.M)
    if not m:
        return None
    n = re.compile(end, re.M).search(text, m.end())
    return strip_comments(text[m.start():n.end()]) if n else None


def define(text, name):
    m = re.search(r'^#define\s+%s\s+(.+?)\s*(?://.*)?$' % re.escape(name), text, re.M)
    return m.group(1).strip() if m else None


def main():
    oled = io.open(os.path.join(OLED, 'Host-OLED.ino'), encoding='utf-8').read()
    gw = io.open(os.path.join(GW, 'Host-Gateway.ino'), encoding='utf-8').read()
    so = io.open(os.path.join(OLED, 'HostScreen.h'), encoding='utf-8').read()
    sg = io.open(os.path.join(GW, 'HostScreen.h'), encoding='utf-8').read()
    problems = 0

    fo, fg = functions(oled), functions(gw)
    for name in SHARED_INO:
        if name not in fo:
            print('  ไม่พบ %s ใน Host-OLED (รายการในตัวตรวจล้าสมัย?)' % name); problems += 1
        elif name not in fg:
            print('  ไม่พบ %s ใน Host-Gateway' % name); problems += 1
        elif fo[name] != fg[name]:
            print('  ** %s ต่างจาก Host-OLED **' % name); problems += 1
    print('ฟังก์ชันร่วมใน .ino: %d ตัว' % len(SHARED_INO))

    so_f, sg_f = functions(so), functions(sg)
    n = 0
    for name, bodies in so_f.items():
        if name in SCREEN_DIFFERS:
            continue
        n += 1
        if sg_f.get(name) != bodies:
            print('  ** HostScreen.h: %s ต่างจาก Host-OLED **' % name); problems += 1
    for name in sg_f:
        if name not in so_f:
            print('  ** HostScreen.h ของ Host-Gateway มีฟังก์ชันที่ Host-OLED ไม่มี: %s **' % name); problems += 1
    print('ฟังก์ชันวาดจอร่วม: %d ตัว (ยกเว้น %s)' % (n, ', '.join(sorted(SCREEN_DIFFERS))))

    for name, s, e in SHARED_BLOCKS:
        a, b = block(oled, s, e), block(gw, s, e)
        if a is None or a != b:
            print('  ** นิยาม %s ต่างจาก Host-OLED **' % name); problems += 1
    print('นิยามโครงสร้าง: %d รายการ' % len(SHARED_BLOCKS))

    for d in SHARED_DEFINES:
        a, b = define(oled, d), define(gw, d)
        if a is None or a != b:
            print('  ** #define %s ต่าง: OLED=%r Gateway=%r **' % (d, a, b)); problems += 1
    print('ค่าคงที่ร่วม: %d ตัว' % len(SHARED_DEFINES))

    # ---- Host-TFT-Gateway: ตรรกะเตือนต้องเหมือน Host-TFT28 และส่วนลิงก์พีซีต้องเหมือน Host-Gateway ----
    tg = io.open(os.path.join(ROOT, 'Host-TFT-Gateway', 'Host-TFT-Gateway.ino'), encoding='utf-8').read()
    t28 = io.open(os.path.join(ROOT, 'Host-TFT28', 'Host-TFT28.ino'), encoding='utf-8').read()
    ftg, ft28 = functions(tg), functions(t28)
    for name in TFT_SHARED_WITH_TFT28:
        if name not in ft28:
            print('  ไม่พบ %s ใน Host-TFT28 (รายการในตัวตรวจล้าสมัย?)' % name); problems += 1
        elif name not in ftg:
            print('  ไม่พบ %s ใน Host-TFT-Gateway' % name); problems += 1
        elif ft28[name] != ftg[name]:
            print('  ** Host-TFT-Gateway: %s ต่างจาก Host-TFT28 **' % name); problems += 1
    print('Host-TFT-Gateway: ฟังก์ชันร่วมกับ Host-TFT28 %d ตัว' % len(TFT_SHARED_WITH_TFT28))
    for name in TFT_SHARED_WITH_GW:
        if name not in fg:
            print('  ไม่พบ %s ใน Host-Gateway' % name); problems += 1
        elif name not in ftg:
            print('  ไม่พบ %s ใน Host-TFT-Gateway' % name); problems += 1
        elif fg[name] != ftg[name]:
            print('  ** Host-TFT-Gateway: %s ต่างจาก Host-Gateway **' % name); problems += 1
    print('Host-TFT-Gateway: ฟังก์ชันลิงก์พีซีร่วมกับ Host-Gateway %d ตัว' % len(TFT_SHARED_WITH_GW))
    s28 = functions(io.open(os.path.join(ROOT, 'Host-TFT28', 'HostScreen.h'), encoding='utf-8').read())
    stg = functions(io.open(os.path.join(ROOT, 'Host-TFT-Gateway', 'HostScreen.h'), encoding='utf-8').read())
    if s28 != stg:
        print('  ** HostScreen.h ของ Host-TFT-Gateway ต่างจาก Host-TFT28 (ต้องเหมือนทุกฟังก์ชัน) **'); problems += 1
    print('HostScreen.h (TFT): %d ฟังก์ชันเหมือน Host-TFT28' % len(s28))
    dd = [io.open(os.path.join(ROOT, d, 'drop_detector.h'), 'rb').read() for d in ('Station', 'Station-Thin')]
    if dd[0] != dd[1]:
        print('  ** drop_detector.h ของ Station-Thin ต่างจาก Station (ต้องเหมือนทุกไบต์) **'); problems += 1
    print('drop_detector.h: Station-Thin เหมือน Station ทุกไบต์' if dd[0] == dd[1] else '')
    p4 = [io.open(os.path.join(ROOT, d, 'protocol_v4.h'), 'rb').read() for d in ('Station-Thin', 'Host-TFT-Gateway')]
    if p4[0] != p4[1]:
        print('  ** protocol_v4.h สองฝั่งไม่เหมือนกัน **'); problems += 1
    print('protocol_v4.h: สองฝั่งเหมือนกันทุกไบต์' if p4[0] == p4[1] else '')

    print()
    if problems:
        print('พบความต่าง %d จุด — แก้ตรรกะที่ Host-OLED แล้วต้องแก้ Host-Gateway ให้ตรงกันด้วย' % problems)
        return 1
    print('ตรรกะที่ยกมาตรงกับ Host-OLED ทุกบรรทัด')
    return 0


if __name__ == '__main__':
    sys.exit(main())
