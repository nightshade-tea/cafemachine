#!/bin/bash

# die on errors
set -eEuo pipefail

ROOT=$(git rev-parse --show-toplevel)
QEMU_DIR=$ROOT/qemu

append() {
  grep -qxF "$1" "$2" || echo "$1" >> "$2"
}

cd "$ROOT/devices"

for name in */; do
  name=${name%/}

  # link our device source inside qemu
  ln -sfn "../../devices/$name/src/$name" "$QEMU_DIR/hw/$name"
  for header in "$name"/include/*.h; do
    ln -sf "../../devices/$header" "$QEMU_DIR/include/"
  done

  # include it in the build steps
  append "source $name/Kconfig" "$QEMU_DIR/hw/Kconfig"
  append "subdir('$name')" "$QEMU_DIR/hw/meson.build"
done
