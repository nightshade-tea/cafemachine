DEVICES := $(notdir $(wildcard devices/*))
NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu)
QEMU_DIR := qemu
BUILD_DIR := $(QEMU_DIR)/build
PATCH_MARKER := $(QEMU_DIR)/.patched
BUILD_TARGETS := x86_64-softmmu

build: patch format
	@if [ ! -d $(BUILD_DIR) ]; then \
		mkdir -p $(BUILD_DIR) && \
		cd $(BUILD_DIR) && \
		../configure --target-list=$(BUILD_TARGETS); \
	fi
	$(MAKE) -j$(NPROC) -C $(BUILD_DIR)

patch:
	@if [ ! -f $(PATCH_MARKER) ]; then \
		git submodule update --init $(QEMU_DIR) && \
		git config submodule.$(QEMU_DIR).ignore all && \
		bash ./scripts/patch.sh && \
		touch $(PATCH_MARKER) && \
		echo "QEMU patched"; \
	else \
		echo "patch already applied ($(PATCH_MARKER) exists)"; \
	fi

# run a device, e.g. make cafe DRIVE=path/to/image.qcow2
$(DEVICES): build
ifndef DRIVE
	$(error set DRIVE to a qcow2 image, e.g. make $@ DRIVE=path/to/image.qcow2)
endif
	$(MAKE) DRIVE=$(abspath $(DRIVE)) -C devices/$@ run

clean:
	@if [ -d $(BUILD_DIR) ]; then \
		$(MAKE) -C $(BUILD_DIR) clean; \
	fi

prune:
	rm -rf $(BUILD_DIR)

deinit:
	git submodule deinit -f $(QEMU_DIR)

format:
	find devices -iname '*.[hc]' | xargs clang-format -i

.PHONY: build patch clean prune deinit format $(DEVICES)
