#!/bin/bash

ROOT=$(git rev-parse --show-toplevel)
QEMU_DIR=$ROOT/qemu
QEMU_BIN=$QEMU_DIR/build/qemu-system-x86_64

DRIVE=$ROOT/images/disks/cafe-debian-amd64.qcow2
CDROM=$ROOT/images/isos/debian-13.1.0-amd64-netinst.iso
SHARED=$ROOT

$QEMU_BIN                                                   \
  -accel kvm                                                \
  -smp 8                                                    \
  -m 8G                                                     \
  -boot c                                                   \
  -drive file=$DRIVE,format=qcow2,if=virtio,cache=writeback \
  -netdev user,id=net0                                      \
  -device virtio-net-pci,netdev=net0                        \
  -virtfs local,path=$SHARED,mount_tag=share9p,security_model=mapped-xattr,id=shared9p  \
  -device cafe                                              \
  -nographic                                                \
  -serial mon:stdio
