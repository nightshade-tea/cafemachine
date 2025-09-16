#ifndef _CAFE_KMOD_H_
#define _CAFE_KMOD_H_

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>

#define CAFE_KMOD_NAME "cafe_kmod"

struct CafeDeviceData {
  void __iomem *mmio;
};

#endif /* _CAFE_KMOD_H_ */
