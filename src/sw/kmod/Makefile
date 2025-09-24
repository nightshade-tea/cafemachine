obj-m += cafe.o
cafe-objs += kmod.o data.o mmio.o chrdev.o
ccflags-y := -I$(src)/../../../include

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
