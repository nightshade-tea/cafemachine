obj-m += sugar.o
sugar-objs += kmod.o
#ccflags-y := -I$(src)/../../include

build:
	$(MAKE) -j$(shell nproc) -C /lib/modules/$(shell uname -r)/build M=$(PWD) \
		modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

format:
	find $(SRC_DIR) -iname '*.[hc]' | xargs clang-format -i

.PHONY: all clean format
