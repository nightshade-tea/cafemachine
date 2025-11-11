ROOT := $(shell git rev-parse --show-toplevel)
SRC_DIR := include/ src/
QEMU_BIN := $(ROOT)/qemu-10.0.3/build/qemu-system-x86_64
CORES := 16
RAM := 2G
DEV_NAME := tea

build:
	$(MAKE) -C $(ROOT)

clean:
	$(MAKE) -C $(ROOT) clean

run:
	$(QEMU_BIN)	                                               	\
		-M q35                                                    \
		-accel kvm                                                \
		-smp $(CORES)                                             \
		-m $(RAM)                                                 \
		-boot c                                                   \
		-drive file=$(DRIVE),format=qcow2,if=virtio,cache=writeback \
		-netdev user,id=net0,hostfwd=tcp::7735-:7735               \
		-device virtio-net-pci,netdev=net0                        \
		-virtfs local,path=$(ROOT),mount_tag=share9p,security_model=mapped-xattr,id=shared9p \
		-device pcie-root-port,id=rp1,bus=pcie.0,chassis=1,slot=1 \
		-nographic                                                \
		-serial mon:stdio                                         \
		-D $(ROOT)/qemu-debug.log $(OPT)

format:
	find $(SRC_DIR) -iname '*.[hc]' | xargs clang-format -i
