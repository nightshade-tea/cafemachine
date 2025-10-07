SRC_DIR := src/hw
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
	$(MAKE) -j$(shell nproc) -C $(BUILD_DIR)

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

run: build
	bash ./scripts/start.sh

clean:
	@if [ -d $(BUILD_DIR) ]; then \
		$(MAKE) -C $(BUILD_DIR) clean; \
	fi

prune:
	rm -rf $(BUILD_DIR)

deinit:
	git submodule deinit -f $(QEMU_DIR)

format:
	find $(SRC_DIR) -iname '*.[hc]' | xargs clang-format -i

.PHONY: build patch run clean prune deinit format
