#!/usr/bin/env python3
"""สร้างภาพประกอบเอกสารแบตเตอรี่ LiPo 802540 (docs/battery/img/*.png)

ตัวเลขทุกตัวในไฟล์นี้ยกมาจากตารางในเอกสาร ถ้าแก้ตารางต้องแก้ที่นี่ด้วย
"""
import os, sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib import font_manager

OUT = sys.argv[1] if len(sys.argv) > 1 else "docs/battery/img"
os.makedirs(OUT, exist_ok=True)

for f in font_manager.findSystemFonts():
    if "Sarabun" in f:
        font_manager.fontManager.addfont(f)
plt.rcParams.update({
    "font.family": "TH Sarabun New", "font.size": 16,
    "axes.edgecolor": "#c3c2b7", "axes.labelcolor": "#52514e",
    "xtick.color": "#52514e", "ytick.color": "#52514e",
    "axes.spines.top": False, "axes.spines.right": False,
    "figure.facecolor": "#fcfcfb", "axes.facecolor": "#fcfcfb",
    "savefig.dpi": 200,
})
BLUE, ORANGE, INK, MUTED, GRID = "#2a78d6", "#eb6834", "#0b0b0b", "#52514e", "#e6e5e0"

# ---- รูปที่ 1: แรงดันวงจรเปิด (OCV) เทียบสูตรเส้นตรงในเฟิร์มแวร์ Host ----
# ค่าประมาณทั่วไปของเซลล์ LiCoO2/กราไฟต์ ขณะพัก 25 °C (ตาราง 3 ในเอกสารเทคนิค)
ocv = [(3.00, 0), (3.45, 5), (3.68, 10), (3.74, 20), (3.77, 30), (3.79, 40),
       (3.82, 50), (3.87, 60), (3.92, 70), (3.98, 80), (4.06, 90), (4.20, 100)]
fig, ax = plt.subplots(figsize=(8, 4.6))
ax.grid(True, color=GRID, linewidth=0.8); ax.set_axisbelow(True)
v = [p[0] for p in ocv]; s = [p[1] for p in ocv]
ax.plot(v, s, color=BLUE, linewidth=2, marker="o", markersize=5,
        markeredgecolor="#fcfcfb", markeredgewidth=1.5, label="SOC จริงโดยประมาณ (OCV ขณะพัก)")
lin_v = [3.0, 3.2, 4.2]; lin_s = [0, 0, 100]
ax.plot(lin_v, lin_s, color=ORANGE, linewidth=2, linestyle=(0, (5, 3)),
        label="สูตรเส้นตรง 3.2–4.2 V ใน calculateBatteryPct()")
ax.annotate("ที่ 3.70 V\nเฟิร์มแวร์แสดง 50 %\nแต่ของจริงเหลือราว 13 %",
            xy=(3.70, 50), xytext=(3.86, 6), color=INK, fontsize=15,
            arrowprops=dict(arrowstyle="-", color=MUTED, linewidth=1))
ax.plot([3.70, 3.70], [13, 50], color=MUTED, linewidth=1, linestyle=":")
ax.set_xlim(3.0, 4.25); ax.set_ylim(0, 105)
ax.set_xlabel("แรงดันขั้วแบตเตอรี่ (V)"); ax.set_ylabel("สถานะประจุ (%)")
ax.legend(loc="upper left", frameon=False, fontsize=14)
fig.tight_layout(); fig.savefig(os.path.join(OUT, "fig1_soc_vs_voltage.png")); plt.close(fig)

# ---- รูปที่ 2: เวลาใช้งานต่อการชาร์จหนึ่งครั้ง (ตาราง 4 ในรายงาน) ----
rows = [("Station (ESP32-S3 + TFT)", 3.3, 4.1, 4.6),
        ("Station-C3-OLED (ESP32-C3)", 4.3, 4.9, 5.7),
        ("Host-OLED (ESP32-S3 + AP)", 3.6, 4.3, 5.1)]
fig, ax = plt.subplots(figsize=(8, 3.4))
ax.grid(True, axis="x", color=GRID, linewidth=0.8); ax.set_axisbelow(True)
y = list(range(len(rows)))[::-1]
for yi, (name, lo, mid, hi) in zip(y, rows):
    ax.barh(yi, mid, height=0.5, color=BLUE)
    ax.plot([lo, hi], [yi, yi], color=INK, linewidth=1.2)
    ax.plot([lo, lo], [yi - .12, yi + .12], color=INK, linewidth=1.2)
    ax.plot([hi, hi], [yi - .12, yi + .12], color=INK, linewidth=1.2)
    ax.text(hi + 0.15, yi, f"≈ {mid:.1f} ชม.", va="center", color=INK, fontsize=15)
ax.axvline(8, color=MUTED, linewidth=1, linestyle=":")
ax.set_ylim(-0.5, max(y) + 0.9)
ax.text(7.9, max(y) + 0.6, "เวรพยาบาล 8 ชม.", color=MUTED, fontsize=14, va="center", ha="right")
ax.set_yticks(y); ax.set_yticklabels([r[0] for r in rows])
ax.set_xlim(0, 10); ax.set_xlabel("ชั่วโมงต่อการชาร์จเต็มหนึ่งครั้ง (600 mAh)")
fig.tight_layout(); fig.savefig(os.path.join(OUT, "fig2_runtime.png")); plt.close(fig)
print("เขียนภาพไว้ที่", OUT)
