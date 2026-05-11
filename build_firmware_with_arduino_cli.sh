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
  "$SOURCE_DIR/ulisp_config.h" \
  "$SOURCE_DIR/ulisp_types.h" \
  "$SOURCE_DIR/ulisp_state.h" \
  "$SOURCE_DIR/ulisp_state.cpp" \
  "$SOURCE_DIR/ulisp_messages.h" \
  "$SOURCE_DIR/ulisp_messages.cpp" \
  "$SOURCE_DIR/ulisp_entry.h" \
  "$SOURCE_DIR/ulisp_entry.cpp" \
  "$SOURCE_DIR/ulisp_error.h" \
  "$SOURCE_DIR/ulisp_error.cpp" \
  "$SOURCE_DIR/ulisp_memory.h" \
  "$SOURCE_DIR/ulisp_memory.cpp" \
  "$SOURCE_DIR/ulisp_persistence.cpp" \
  "$SOURCE_DIR/ulisp_streams.h" \
  "$SOURCE_DIR/ulisp_streams.cpp" \
  "$SOURCE_DIR/ulisp_print.h" \
  "$SOURCE_DIR/ulisp_print.cpp" \
  "$SOURCE_DIR/ulisp_platform.h" \
  "$SOURCE_DIR/ulisp_platform.cpp" \
  "$SOURCE_DIR/ulisp_pretty.h" \
  "$SOURCE_DIR/ulisp_pretty.cpp" \
  "$SOURCE_DIR/ulisp_terminal.h" \
  "$SOURCE_DIR/ulisp_terminal.cpp" \
  "$SOURCE_DIR/ulisp_reader.h" \
  "$SOURCE_DIR/ulisp_reader.cpp" \
  "$SOURCE_DIR/ulisp_tables.h" \
  "$SOURCE_DIR/ulisp_tables.cpp" \
  "$SOURCE_DIR/ulisp_eval.h" \
  "$SOURCE_DIR/ulisp_eval.cpp" \
  "$SOURCE_DIR/ulisp_gfx.h" \
  "$SOURCE_DIR/ulisp_gfx.cpp" \
  "$SOURCE_DIR/ulisp_arduino.h" \
  "$SOURCE_DIR/ulisp_arduino.cpp" \
  "$SOURCE_DIR/ulisp_persistence.h" \
  "$SOURCE_DIR/ulisp_picocalc.h" \
  "$SOURCE_DIR/ulisp_picocalc.cpp" \
  "$SOURCE_DIR/ulisp_runtime.h" \
  "$SOURCE_DIR/ulisp_runtime.cpp" \
  "$SOURCE_DIR/ulisp_runtime_symbols.cpp" \
  "$SOURCE_DIR/ulisp_runtime_math.cpp" \
  "$SOURCE_DIR/ulisp_runtime_data.cpp" \
  "$SOURCE_DIR/ulisp_runtime_env.cpp" \
  "$SOURCE_DIR/ulisp_builtins.h" \
  "$SOURCE_DIR/ulisp_builtins.cpp" \
  "$SOURCE_DIR/ulisp_builtins_control.cpp" \
  "$SOURCE_DIR/ulisp_builtins_core.cpp" \
  "$SOURCE_DIR/ulisp_builtins_numbers.cpp" \
  "$SOURCE_DIR/ulisp_builtins_strings.cpp" \
  "$SOURCE_DIR/ulisp_builtins_system.cpp" \
  "$SOURCE_DIR/ulisp_http.h" \
  "$SOURCE_DIR/ulisp_http.cpp" \
  "$SOURCE_DIR/dhcpserver.c" \
  "$SOURCE_DIR/dhcpserver.h" \
  "$SOURCE_DIR/dnsserver.c" \
  "$SOURCE_DIR/dnsserver.h" \
  "$SOURCE_DIR/ui/repl_window.h" \
  "$SOURCE_DIR/ui/repl_window.cpp" \
  "$SOURCE_DIR/ui/window.h" \
  "$SOURCE_DIR/ui/window.cpp" \
  "$SOURCE_DIR/ui/window_builtins.cpp" \
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
