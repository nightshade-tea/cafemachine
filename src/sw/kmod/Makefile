obj-m += cafe.o
cafe-objs += kmod.o data.o mmio.o chrdev.o fops.o irq.o dma.o utils.o
ccflags-y := -I$(src)/../../../include

all: format
	$(MAKE) -j$(shell nproc) -C /lib/modules/$(shell uname -r)/build M=$(PWD) \
		modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

format:
	find $(SRC_DIR) -iname '*.[hc]' | xargs clang-format -i

.PHONY: all clean format
