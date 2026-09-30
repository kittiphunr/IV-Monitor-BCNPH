#!/usr/bin/env python3
"""เตรียมภาพประกอบของคู่มือจากภาพหน้าจอที่เครื่องมือ preview สร้างไว้

ใช้: python3 make_images.py            (รันจากโฟลเดอร์นี้)
ภาพผังองค์ประกอบระบบ (diagram_system.png) วาดด้วยสคริปต์นี้เช่นกัน
"""
import os
from PIL import Image
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
OUT = os.path.join(os.path.dirname(__file__), 'manual-img')
os.makedirs(OUT, exist_ok=True)

def save(src, dst, crop=None, w=None):
    im = Image.open(os.path.join(ROOT, src)).convert('RGB')
    if crop: im = im.crop(crop)
    if w: im = im.resize((w, round(im.height * w / im.width)), Image.LANCZOS)
    im.save(os.path.join(OUT, dst))
    print(dst, im.size)

save('docs/screens-station/p1_bag.png',         'st_p1.png',   w=260)
save('docs/screens-station/p3_link.png',        'st_p3.png',   w=260)
save('docs/screens-station/emergency.png',      'st_emg.png',  w=260)
save('docs/screens-station/near_end.png',       'st_near.png', w=260)
save('docs/screens-station/config_id.png',      'st_cfg.png',  w=260)
save('docs/screens-oled/01_block_5beds.png',    'ol_block.png',  w=520)
save('docs/screens-oled/02_detail_bed1.png',    'ol_detail.png', w=520)
save('docs/screens-oled/14_link_diag_dup_id.png','ol_diag.png',  w=520)
save('docs/screens-station/sensor_check.png',   'st_check.png', w=300)
save('docs/screens-station/cal_1_noise.png',    'cal_1.png',   w=240)
save('docs/screens-station/cal_2_learn.png',    'cal_2.png',   w=240)
save('docs/screens-station/cal_3_result.png',   'cal_3.png',   w=240)
save('docs/screens-station/cal_4_verify.png',   'cal_4.png',   w=240)
# ---- ภาพของบทที่ ๙ ถึง ๑๕ (การใช้งานแยกตามรุ่น) ----
# สายหลัก (ภาพหน้าที่เหมือนกันใช้กับสายเสถียรด้วย)
for src, dst in [('p2_plan', 'st_p2'), ('p4_trend', 'st_p4'), ('p1_paused', 'st_paused'),
                 ('screensaver', 'st_saver'), ('p3_link_oneway', 'st_oneway')]:
    save('docs/screens-station/%s.png' % src, dst + '.png', w=260)
# รุ่นพิเศษ Fallout
for src, dst in [('p1_bag', 'fo_p1'), ('p2_plan', 'fo_p2'), ('p3_link', 'fo_p3'), ('p4_trend', 'fo_p4'),
                 ('config_id', 'fo_cfg'), ('sensor_check', 'fo_check'), ('p1_paused', 'fo_paused'),
                 ('p1_noflow', 'fo_noflow'), ('screensaver', 'fo_saver'), ('emergency', 'fo_emg'),
                 ('near_end', 'fo_near'), ('p3_link_oneway', 'fo_oneway')]:
    save('docs/screens-fallout/%s.png' % src, dst + '.png', w=260)
# บอร์ดเล็ก (ภาพต้นทางขยาย 8 เท่าแล้ว ย่อลงครึ่งหนึ่งยังคมอยู่)
for src, dst in [('02_rate_normal', 'c3_rate'), ('06_gtt', 'c3_gtt'), ('07_volume', 'c3_vol'),
                 ('08_status', 'c3_status'), ('09_no_link', 'c3_nolink'), ('10_paused', 'c3_paused'),
                 ('11_alert_no_flow', 'c3_noflow'), ('14_alert_near_end', 'c3_near'),
                 ('17_set_bed_id', 'c3_setid'), ('18_screensaver', 'c3_saver'),
                 ('20_cal1_countdown', 'c3_cal1'), ('23_cal3_learn', 'c3_cal3'),
                 ('25_cal_done', 'c3_caldone'), ('26_cal_failed', 'c3_calfail'), ('27_link_oneway', 'c3_oneway')]:
    save('docs/screens-c3/%s.png' % src, dst + '.png', w=288)
# เครื่องศูนย์กลางจอ OLED
for src, dst in [('06_block_1bed', 'ol_1bed'), ('08_block_8beds', 'ol_8beds'), ('05_block_near_end', 'ol_nearend'),
                 ('03_detail_bed5_nosignal', 'ol_nosig'), ('17_block_extra_bed_warning', 'ol_extra'),
                 ('09_screensaver', 'ol_saver')]:
    save('docs/screens-oled/%s.png' % src, dst + '.png', w=520)
# เครื่องศูนย์กลางจอสี 2.8 นิ้ว
for src, dst in [('main_normal', 'tf_main'), ('main_alert', 'tf_alert'), ('main_nearend', 'tf_near'),
                 ('alarm_fullscreen', 'tf_alarm'), ('screensaver', 'tf_saver'), ('poweroff', 'tf_off')]:
    save('docs/screens-host/%s.png' % src, dst + '.png', w=480)
# เครื่องศูนย์กลางจอสัมผัส 2.4 นิ้ว
for src, dst in [('home_normal', 'tc_home'), ('home_alert', 'tc_alert'), ('home_nearend', 'tc_near'),
                 ('bed_detail', 'tc_bed'), ('bed_settings', 'tc_set'), ('numpad', 'tc_num'),
                 ('system', 'tc_sys'), ('alarm', 'tc_alarm'), ('calibration', 'tc_cal')]:
    save('docs/screens-touch/%s.png' % src, dst + '.png', w=360)
# ภาพหน้าเว็บ (web_*.png) เป็นหน้าที่ของ crop_web_images.py ที่เดียว
# เดิมไฟล์นี้ก็สร้าง web_top.png และ web_card.png ด้วยกรอบตัดคนละแบบ ใครรันทีหลังก็ทับกัน
# คู่มือที่เก็บไว้ใช้รุ่นของ crop_web_images.py จึงตัดสองบรรทัดนั้นออกจากที่นี่
print('หมายเหตุ: diagram_system.png และ diagram_sensor_pos.png เก็บไว้ในที่เก็บโดยตรง ไม่ได้สร้างใหม่จากสคริปต์นี้')
