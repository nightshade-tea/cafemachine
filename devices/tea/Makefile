DEVICE := tea
QEMU_ARGS := -device pcie-root-port,id=rp1,bus=pcie.0,chassis=1,slot=1
NET_OPTS := ,hostfwd=tcp::7735-:7735
GUEST_DIRS := src/lie-detector/ps-server
HOST_DIRS := src/client

include ../../scripts/device.mk
