#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'USAGE'
usage: deploy_uf2loader.sh <pico|pico2w> [mountpoint]

Copies the CMake-built application UF2 to a mounted UF2 Loader SD card.

Targets:
  pico    -> build-ulisp-cmake/uf2/ulisp_picocalc_pico_rp2040.uf2       -> /pico1-apps/
  pico2w  -> build-ulisp-cmake/uf2/ulisp_picocalc_pico2w_rp2350.uf2    -> /pico2-apps/

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
REPO_ROOT=$(cd "$(dirname "$0")/.." && pwd)

case "$TARGET" in
  pico)
    UF2="$REPO_ROOT/build-ulisp-cmake/uf2/ulisp_picocalc_pico_rp2040.uf2"
    APP_DIR="pico1-apps"
    ;;
  pico2w|pico2|rp2350)
    UF2="$REPO_ROOT/build-ulisp-cmake/uf2/ulisp_picocalc_pico2w_rp2350.uf2"
    APP_DIR="pico2-apps"
    ;;
  *)
    usage
    exit 2
    ;;
esac

if [[ ! -f "$UF2" ]]; then
  echo "UF2 not found: $UF2" >&2
  echo "Build first: cmake --build build-ulisp-cmake --target firmware_all -j1" >&2
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
