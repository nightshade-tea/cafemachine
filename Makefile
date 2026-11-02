QEMU_DIR := qemu
BUILD_DIR := $(QEMU_DIR)/build
PATCH_MARKER := $(QEMU_DIR)/.patched
TARGETS := x86_64-softmmu
DEVICES := $(notdir $(wildcard devices/*))
NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu)

build: $(PATCH_MARKER)
	@test -d $(BUILD_DIR) || { mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && \
		../configure --target-list=$(TARGETS); }
	$(MAKE) -j$(NPROC) -C $(BUILD_DIR)

init: $(QEMU_DIR)/.git

$(QEMU_DIR)/.git:
	git submodule update --init $(QEMU_DIR)
	git config submodule.$(QEMU_DIR).ignore all

patch: $(PATCH_MARKER)

$(PATCH_MARKER): $(QEMU_DIR)/.git devices
	bash scripts/patch.sh
	touch $@

# run a device, e.g. make cafe DRIVE=path/to/image.qcow2
$(DEVICES):
	$(MAKE) -C devices/$@ run $(if $(DRIVE),DRIVE=$(abspath $(DRIVE)))

format:
	find devices -name '*.[ch]' | xargs clang-format -i

clean:
	test ! -d $(BUILD_DIR) || $(MAKE) -C $(BUILD_DIR) clean

prune:
	rm -rf $(BUILD_DIR)

deinit:
	git submodule deinit -f $(QEMU_DIR)

.PHONY: build init patch format clean prune deinit $(DEVICES)
