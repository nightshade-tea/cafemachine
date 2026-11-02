#ifndef _CAFE_MMIO_H_
#define _CAFE_MMIO_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

int cafe_mmio_enable(struct pci_dev *pdev);

void cafe_mmio_disable(struct pci_dev *pdev);

#endif /* _CAFE_MMIO_H_ */
