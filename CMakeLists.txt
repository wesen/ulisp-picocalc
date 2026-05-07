cmake_minimum_required(VERSION 3.20)

# This is the first CMake-oriented build stage for the PicoCalc uLisp split.
# It keeps the Arduino-Pico core as the compiler/linker provider while moving
# the firmware source out of .ino preprocessing and into explicit side-by-side
# .cpp/.h files.
project(ulisp_picocalc_firmware NONE)

set(ULISP_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(ULISP_OUTPUT_DIR "${CMAKE_BINARY_DIR}/uf2")
set(ULISP_BUILD_SCRIPT "${ULISP_SOURCE_DIR}/build_firmware_with_arduino_cli.sh")

find_program(ARDUINO_CLI arduino-cli REQUIRED)

function(add_ulisp_arduino_target target_name fqbn output_name)
  set(output_uf2 "${ULISP_OUTPUT_DIR}/${output_name}.uf2")

  add_custom_command(
    OUTPUT "${output_uf2}"
    COMMAND "${CMAKE_COMMAND}" -E env "PATH=$ENV{PATH}"
            "${ULISP_BUILD_SCRIPT}"
            "${ULISP_SOURCE_DIR}"
            "${CMAKE_BINARY_DIR}"
            "${fqbn}"
            "${target_name}"
            "${output_uf2}"
    DEPENDS
      "${ULISP_BUILD_SCRIPT}"
      "${ULISP_SOURCE_DIR}/ulisp_core.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_config.h"
      "${ULISP_SOURCE_DIR}/ulisp_types.h"
      "${ULISP_SOURCE_DIR}/ulisp_state.h"
      "${ULISP_SOURCE_DIR}/ulisp_state.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_messages.h"
      "${ULISP_SOURCE_DIR}/ulisp_messages.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_entry.h"
      "${ULISP_SOURCE_DIR}/ulisp_entry.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_error.h"
      "${ULISP_SOURCE_DIR}/ulisp_error.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_memory.h"
      "${ULISP_SOURCE_DIR}/ulisp_memory.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_persistence.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_streams.h"
      "${ULISP_SOURCE_DIR}/ulisp_streams.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_print.h"
      "${ULISP_SOURCE_DIR}/ulisp_print.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_platform.h"
      "${ULISP_SOURCE_DIR}/ulisp_platform.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_pretty.h"
      "${ULISP_SOURCE_DIR}/ulisp_pretty.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_terminal.h"
      "${ULISP_SOURCE_DIR}/ulisp_terminal.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_reader.h"
      "${ULISP_SOURCE_DIR}/ulisp_reader.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_tables.h"
      "${ULISP_SOURCE_DIR}/ulisp_tables.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_eval.h"
      "${ULISP_SOURCE_DIR}/ulisp_eval.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_gfx.h"
      "${ULISP_SOURCE_DIR}/ulisp_gfx.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_arduino.h"
      "${ULISP_SOURCE_DIR}/ulisp_arduino.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_persistence.h"
      "${ULISP_SOURCE_DIR}/ulisp_picocalc.h"
      "${ULISP_SOURCE_DIR}/ulisp_picocalc.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_runtime.h"
      "${ULISP_SOURCE_DIR}/ulisp_runtime.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_runtime_symbols.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_runtime_math.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_runtime_data.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_runtime_env.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_builtins.h"
      "${ULISP_SOURCE_DIR}/ulisp_builtins.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_builtins_control.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_builtins_core.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_builtins_numbers.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_builtins_strings.cpp"
      "${ULISP_SOURCE_DIR}/ulisp_builtins_system.cpp"
      "${ULISP_SOURCE_DIR}/ui/repl_window.h"
      "${ULISP_SOURCE_DIR}/ui/repl_window.cpp"
      "${ULISP_SOURCE_DIR}/Setup60_RP2040_ILI9488.h"
    VERBATIM
    COMMENT "Building ${output_name}.uf2 with ${fqbn}"
  )

  add_custom_target(${target_name} DEPENDS "${output_uf2}")
  add_dependencies(firmware_all ${target_name})
endfunction()

add_custom_target(firmware_all)

# RP2040 / Pico 1 family: install app UF2s under /pico1-apps/ for UF2 Loader.
add_ulisp_arduino_target(
  ulisp_picocalc_pico_rp2040
  "rp2040:rp2040:rpipico"
  "ulisp_picocalc_pico_rp2040"
)

# Current hardware: Pico 2W / RP2350. Install app UF2s under /pico2-apps/.
add_ulisp_arduino_target(
  ulisp_picocalc_pico2w_rp2350
  "rp2040:rp2040:rpipico2w"
  "ulisp_picocalc_pico2w_rp2350"
)

message(STATUS "UF2 outputs will be written to: ${ULISP_OUTPUT_DIR}")
message(STATUS "RP2040 UF2 Loader destination: /pico1-apps/ulisp_picocalc_pico_rp2040.uf2")
message(STATUS "RP2350 UF2 Loader destination: /pico2-apps/ulisp_picocalc_pico2w_rp2350.uf2")
