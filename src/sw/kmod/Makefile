obj-m += cafe.o
cafe-objs += cafe_kmod.o cafe_data.o
ccflags-y := -I$(src)/../../../include

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
