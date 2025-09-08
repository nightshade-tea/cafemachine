#!/bin/bash

ROOT=$(git rev-parse --show-toplevel)
QEMU_BIN=$ROOT/build/qemu-system-x86_64-unsigned

DRIVE=$ROOT/images/disks/cafe-debian-amd64.qcow2
CDROM=$ROOT/images/isos/debian-13.0.0-amd64-netinst.iso
SHARED=$ROOT

$QEMU_BIN                                                   \
  -smp 2                                                    \
  -m 2G                                                     \
  -boot c                                                   \
  -drive file=$DRIVE,format=qcow2,if=virtio,cache=writeback \
  -device virtio-net-pci,netdev=net0                        \
  -virtfs local,path=$SHARED,mount_tag=share9p,security_model=mapped-xattr,id=shared9p  \
  -device cafe                                              \
  -nographic                                                \
  -serial mon:stdio
