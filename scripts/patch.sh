#!/bin/bash

# die on errors
set -eEuo pipefail

ROOT=$(git rev-parse --show-toplevel)
QEMU_DIR=$ROOT/qemu

for dev in "$ROOT"/devices/*/; do
  name=$(basename "$dev")

  # link our device source inside qemu
  ln -s "$dev/src/$name" "$QEMU_DIR/hw/"
  ln -s "$dev"/include/*.h "$QEMU_DIR/include/"

  # include it in the build steps
  echo "source $name/Kconfig" >> "$QEMU_DIR/hw/Kconfig"
  echo "subdir('$name')" >> "$QEMU_DIR/hw/meson.build"
done
