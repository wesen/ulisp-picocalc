#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'USAGE'
usage: deploy_uf2loader.sh <pico|pico2w> [mountpoint]

Copies the CMake-built application UF2 to a mounted UF2 Loader SD card.

Targets:
  pico    -> build-ulisp-cmake/uf2/ulisp_picocalc_pico_rp2040.uf2       -> /pico1-apps/
  pico2w  -> build-ulisp-cmake/uf2/ulisp_picocalc_pico2w_rp2350.uf2    -> /pico2-apps/

UF2 lookup checks BUILD_DIR first when set, then build-ulisp-cmake under the
current directory, this script directory, and this script directory's parent.

If mountpoint is omitted, the script tries to find a mounted /dev/sd* vfat drive
under /media/$USER and checks for BOOT2040.UF2 or BOOT2350.UF2.
USAGE
}

if [[ $# -lt 1 || $# -gt 2 ]]; then
  usage
  exit 2
fi

TARGET=$1
MOUNTPOINT=${2:-}
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

case "$TARGET" in
  pico)
    UF2_BASENAME="ulisp_picocalc_pico_rp2040.uf2"
    APP_DIR="pico1-apps"
    ;;
  pico2w|pico2|rp2350)
    UF2_BASENAME="ulisp_picocalc_pico2w_rp2350.uf2"
    APP_DIR="pico2-apps"
    ;;
  *)
    usage
    exit 2
    ;;
esac

BUILD_DIR_CANDIDATES=()
if [[ -n "${BUILD_DIR:-}" ]]; then
  BUILD_DIR_CANDIDATES+=("$BUILD_DIR")
fi
BUILD_DIR_CANDIDATES+=(
  "$PWD/build-ulisp-cmake"
  "$SCRIPT_DIR/build-ulisp-cmake"
  "$SCRIPT_DIR/../build-ulisp-cmake"
)

UF2=""
for build_dir in "${BUILD_DIR_CANDIDATES[@]}"; do
  candidate="$build_dir/uf2/$UF2_BASENAME"
  if [[ -f "$candidate" ]]; then
    UF2=$(cd "$(dirname "$candidate")" && pwd)/$(basename "$candidate")
    break
  fi
done

if [[ -z "$UF2" ]]; then
  echo "UF2 not found: $UF2_BASENAME" >&2
  echo "Looked under:" >&2
  for build_dir in "${BUILD_DIR_CANDIDATES[@]}"; do
    echo "  $build_dir/uf2/$UF2_BASENAME" >&2
  done
  echo "Build first: cmake --build build-ulisp-cmake --target firmware_all -j1" >&2
  echo "Or pass BUILD_DIR=/path/to/build-ulisp-cmake" >&2
  exit 1
fi

if [[ -z "$MOUNTPOINT" ]]; then
  while read -r target source fstype; do
    if [[ "$fstype" == "vfat" && "$target" == /media/$USER/* ]]; then
      if [[ -f "$target/BOOT2040.UF2" || -f "$target/BOOT2350.UF2" ]]; then
        MOUNTPOINT=$target
        break
      fi
    fi
  done < <(findmnt -rn -o TARGET,SOURCE,FSTYPE)
fi

if [[ -z "$MOUNTPOINT" || ! -d "$MOUNTPOINT" ]]; then
  echo "Could not find UF2 Loader mountpoint. Pass it explicitly, e.g.:" >&2
  echo "  $0 $TARGET /media/$USER/1216-8671" >&2
  exit 1
fi

DEST_DIR="$MOUNTPOINT/$APP_DIR"
mkdir -p "$DEST_DIR"
DEST="$DEST_DIR/$(basename "$UF2")"

cp -v "$UF2" "$DEST"
sync
ls -lh "$DEST"

echo "Copied $TARGET UF2 to $DEST"
echo "Unmount manually when ready, e.g.: udisksctl unmount -b /dev/sda1"
