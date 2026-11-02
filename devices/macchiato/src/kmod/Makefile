obj-m += macchiato.o
macchiato-objs += kmod.o data.o mmio.o irq.o chardev.o
ccflags-y := -I$(src)/../../include

.PHONY: build clean format

build:
	$(MAKE) -j$(shell nproc) -C /lib/modules/$(shell uname -r)/build M=$(PWD) \
		modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

format:
	find $(SRC_DIR) -iname '*.[hc]' | xargs clang-format -i
