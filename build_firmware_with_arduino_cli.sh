#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 5 ]]; then
  echo "usage: $0 <source-dir> <build-dir> <fqbn> <target-name> <output-uf2>" >&2
  exit 2
fi

SOURCE_DIR=$1
BUILD_DIR=$2
FQBN=$3
TARGET_NAME=$4
OUTPUT_UF2=$5

SKETCH_DIR="$BUILD_DIR/sketch-$TARGET_NAME"
ARDUINO_BUILD_DIR="$BUILD_DIR/arduino-$TARGET_NAME"

rm -rf "$SKETCH_DIR" "$ARDUINO_BUILD_DIR"
mkdir -p "$SKETCH_DIR" "$ARDUINO_BUILD_DIR" "$(dirname "$OUTPUT_UF2")"

# Arduino CLI still expects a sketch directory with a same-named .ino file.
# The actual firmware entry points live in ulisp_core.cpp; this .ino is empty.
SKETCH_BASENAME=$(basename "$SKETCH_DIR")
: > "$SKETCH_DIR/$SKETCH_BASENAME.ino"

cp \
  "$SOURCE_DIR/ulisp_core.cpp" \
  "$SOURCE_DIR/ulisp_fwd_decls.h" \
  "$SOURCE_DIR/repl_window.h" \
  "$SOURCE_DIR/repl_window.cpp" \
  "$SOURCE_DIR/Setup60_RP2040_ILI9488.h" \
  "$SKETCH_DIR/"

arduino-cli compile \
  --fqbn "$FQBN" \
  --build-path "$ARDUINO_BUILD_DIR" \
  --libraries "$SOURCE_DIR/../arduino_picocalc_kbd" \
  --libraries "$SOURCE_DIR/.." \
  "$SKETCH_DIR"

UF2_PATH="$ARDUINO_BUILD_DIR/$SKETCH_BASENAME.ino.uf2"
if [[ ! -f "$UF2_PATH" ]]; then
  echo "expected UF2 not found: $UF2_PATH" >&2
  find "$ARDUINO_BUILD_DIR" -maxdepth 2 -type f -name '*.uf2' -print >&2 || true
  exit 1
fi

cp "$UF2_PATH" "$OUTPUT_UF2"
ls -lh "$OUTPUT_UF2"
