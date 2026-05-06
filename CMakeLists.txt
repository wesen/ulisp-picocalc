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
      "${ULISP_SOURCE_DIR}/ulisp_fwd_decls.h"
      "${ULISP_SOURCE_DIR}/repl_window.h"
      "${ULISP_SOURCE_DIR}/repl_window.cpp"
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
