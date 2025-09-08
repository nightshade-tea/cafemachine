#!/bin/bash

ROOT=$(git rev-parse --show-toplevel)
HW_NAME=cafe

# link our device source inside qemu
ln -s $ROOT/src/hw/$HW_NAME $ROOT/qemu/hw/

# include it in the build steps
echo "source $HW_NAME/Kconfig" >> $ROOT/qemu/hw/Kconfig
echo "subdir('$HW_NAME')" >> qemu/hw/meson.build
