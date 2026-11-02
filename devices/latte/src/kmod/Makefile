obj-m := latte.o
latte-objs := kmod.o data.o mmio.o irq.o dma.o integrity.o
ccflags-y := -I$(src)/../../include

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

.PHONY: all clean
