#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SVG_DIR="$SCRIPT_DIR/svg"
PNG_DIR="$SCRIPT_DIR/png"
THUMB_DIR="$SCRIPT_DIR/.quicklook"

mkdir -p "$PNG_DIR" "$THUMB_DIR"

for svg in "$SVG_DIR"/*.svg; do
  name=$(basename "$svg")
  dimensions=$(sed -n 's/.*data-output-width="\([0-9][0-9]*\)" data-output-height="\([0-9][0-9]*\)".*/\1 \2/p' "$svg" | head -n 1)
  width=${dimensions% *}
  height=${dimensions#* }
  size=$(sed -n 's/.*<svg[^>]* width="\([0-9][0-9]*\)".*/\1/p' "$svg" | head -n 1)
  qlmanage -t -s "$size" -o "$THUMB_DIR" "$svg" >/dev/null
  source_png="$THUMB_DIR/$name.png"
  output_png="$PNG_DIR/${name%.svg}.png"
  sips -c "$height" "$width" "$source_png" --out "$output_png" >/dev/null
done

rm -rf "$THUMB_DIR"
echo "Rendered PNG previews in $PNG_DIR"
