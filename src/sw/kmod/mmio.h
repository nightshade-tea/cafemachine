#ifndef _CAFE_MMIO_H_
#define _CAFE_MMIO_H_

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>

int cafe_mmio_init(struct pci_dev *dev);

void cafe_mmio_deinit(struct pci_dev *dev);

#endif /* _CAFE_MMIO_H_ */
