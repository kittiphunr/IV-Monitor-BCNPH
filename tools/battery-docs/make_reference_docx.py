#!/usr/bin/env python3
"""สร้าง reference.docx ให้ pandoc: ฟอนต์ TH SarabunPSK 16 pt ตามรูปแบบเอกสารราชการ"""
import sys
from docx import Document
from docx.shared import Pt, Cm
from docx.oxml.ns import qn

src, dst = sys.argv[1], sys.argv[2]
doc = Document(src)
FONT = "TH SarabunPSK"
SIZES = {"Normal": 16, "Body Text": 16, "First Paragraph": 16, "Compact": 16,
         "Title": 26, "Subtitle": 18, "Date": 16, "Heading 1": 20, "Heading 2": 18,
         "Heading 3": 16, "Source Code": 12, "Block Text": 16, "Image Caption": 14,
         "Table Caption": 14}
for st in doc.styles:
    if st.type != 1 and st.name not in ("Default Paragraph Font", "Verbatim Char"):
        continue
    try:
        f = st.font
    except AttributeError:
        continue
    if st.name in ("Source Code", "Verbatim Char"):
        # [ภาษาไทยในโค้ด] อักษรละตินใช้ฟอนต์ความกว้างคงที่ ส่วนอักษรไทย (complex script) ใช้ Sarabun
        rpr = st.element.get_or_add_rPr()
        rf = rpr.find(qn("w:rFonts"))
        if rf is None:
            rf = rpr.makeelement(qn("w:rFonts"), {}); rpr.append(rf)
        for a in ("w:ascii", "w:hAnsi", "w:eastAsia"):
            rf.set(qn(a), "DejaVu Sans Mono")
        rf.set(qn("w:cs"), FONT)
        st.font.size = Pt(11)
        szcs = rpr.find(qn("w:szCs"))
        if szcs is None:
            szcs = rpr.makeelement(qn("w:szCs"), {}); rpr.append(szcs)
        szcs.set(qn("w:val"), "30")
        continue
    f.name = FONT
    rpr = st.element.get_or_add_rPr()
    rf = rpr.find(qn("w:rFonts"))
    if rf is None:
        rf = rpr.makeelement(qn("w:rFonts"), {}); rpr.append(rf)
    for a in ("w:ascii", "w:hAnsi", "w:cs", "w:eastAsia"):
        rf.set(qn(a), FONT)
    for t in ("w:asciiTheme", "w:hAnsiTheme", "w:cstheme", "w:eastAsiaTheme"):
        if rf.get(qn(t)) is not None:
            del rf.attrib[qn(t)]
    if st.name in SIZES:
        f.size = Pt(SIZES[st.name])
        szcs = rpr.find(qn("w:szCs"))
        if szcs is None:
            szcs = rpr.makeelement(qn("w:szCs"), {}); rpr.append(szcs)
        szcs.set(qn("w:val"), str(SIZES[st.name] * 2))
    if st.name.startswith("Heading") or st.name == "Title":
        f.bold = True
        f.color.rgb = None
        bcs = rpr.makeelement(qn("w:bCs"), {}); rpr.append(bcs)
for s in doc.sections:
    s.page_width, s.page_height = Cm(21.0), Cm(29.7)
    s.left_margin = Cm(3.0); s.right_margin = Cm(2.0)
    s.top_margin = Cm(2.5); s.bottom_margin = Cm(2.0)
doc.save(dst)
