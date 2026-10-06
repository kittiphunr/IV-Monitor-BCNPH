#!/usr/bin/env bash
# สร้างเอกสารแบตเตอรี่ LiPo 802540 จากต้นฉบับ Markdown ใน docs/battery/
#   ภาพประกอบ (PNG) -> .docx (pandoc) -> .pdf (LibreOffice)
# ต้องมี python3 + matplotlib + python-docx, pandoc, libreoffice และฟอนต์ TH Sarabun
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DOCS="$ROOT/docs/battery"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT

bash "$ROOT/tools/manual-build/install_fonts.sh" >/dev/null
python3 "$ROOT/tools/battery-docs/make_figures.py" "$DOCS/img"

pandoc -o "$TMP/default.docx" --print-default-data-file reference.docx
python3 "$ROOT/tools/battery-docs/make_reference_docx.py" "$TMP/default.docx" "$TMP/reference.docx"

for md in "$DOCS"/*.md; do
  base="$(basename "${md%.md}")"
  ( cd "$DOCS" && pandoc "$md" -o "$DOCS/$base.docx" \
      --reference-doc="$TMP/reference.docx" \
      --resource-path="$DOCS" )
  python3 "$ROOT/tools/battery-docs/postprocess_docx.py" "$DOCS/$base.docx"
  soffice --headless --convert-to pdf --outdir "$DOCS" "$DOCS/$base.docx" >/dev/null 2>&1
  echo "สร้างแล้ว: docs/battery/$base.{docx,pdf}"
done
