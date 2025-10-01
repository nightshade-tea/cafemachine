#!/bin/bash

# die on errors
set -eEuo pipefail

ROOT=$(git rev-parse --show-toplevel)
QEMU_DIR=$ROOT/qemu
HW_NAME=cafe

# link our device source inside qemu
ln -s $ROOT/src/hw/$HW_NAME $QEMU_DIR/hw/
ln -s $ROOT/include/cafe.h $ROOT/qemu/include/

# include it in the build steps
echo "source $HW_NAME/Kconfig" >> $QEMU_DIR/hw/Kconfig
echo "subdir('$HW_NAME')" >> $QEMU_DIR/hw/meson.build
