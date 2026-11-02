obj-m := cafe.o
cafe-objs := kmod.o data.o mmio.o chrdev.o fops.o irq.o dma.o utils.o
ccflags-y := -I$(src)/../../include

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

.PHONY: all clean
