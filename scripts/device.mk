# shared rules for devices/<name>/Makefile
#
# a device makefile sets:
#   DEVICE      device name, as registered in qemu
#   QEMU_ARGS   extra qemu arguments (default: -device $(DEVICE))
#   NET_OPTS    extra options for the user netdev, e.g. ,hostfwd=...
#   GUEST_DIRS  directories to build inside the guest (kernel modules, tests)
#   HOST_DIRS   directories to build on the host (tools)

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
QEMU_BIN = $(firstword $(wildcard $(ROOT)/qemu/build/qemu-system-x86_64 \
                                  $(ROOT)/qemu/build/qemu-system-x86_64-unsigned))

ACCEL ?= kvm
SMP ?= 4
RAM ?= 2G
QEMU_ARGS ?= -device $(DEVICE)

run: check-drive qemu
	$(QEMU_BIN) \
		-M q35 \
		-accel $(ACCEL) \
		-smp $(SMP) \
		-m $(RAM) \
		-boot c \
		-drive file=$(DRIVE),format=qcow2,if=virtio,cache=writeback \
		-netdev user,id=net0$(NET_OPTS) \
		-device virtio-net-pci,netdev=net0 \
		-virtfs local,path=$(ROOT),mount_tag=share9p,security_model=mapped-xattr,id=shared9p \
		$(QEMU_ARGS) \
		-nographic \
		-serial mon:stdio \
		-D $(ROOT)/qemu-debug.log \
		$(OPT)

# host: tools that run on this machine
# guest: code that runs inside the vm, build it there
build: qemu host

qemu:
	$(MAKE) -C $(ROOT) build

guest:
	@for d in $(GUEST_DIRS); do $(MAKE) -C $$d || exit 1; done

host:
	@for d in $(HOST_DIRS); do $(MAKE) -C $$d || exit 1; done

clean:
	@for d in $(GUEST_DIRS) $(HOST_DIRS); do $(MAKE) -C $$d clean || exit 1; done

check-drive:
	@test -n "$(DRIVE)" || { echo "set DRIVE to a qcow2 image, e.g. make run DRIVE=path/to/image.qcow2"; exit 1; }

.PHONY: run build qemu guest host clean check-drive
