#ifndef _LATTE_MMIO_H_
#define _LATTE_MMIO_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

int latte_mmio_enable(struct pci_dev *pdev);

void latte_mmio_disable(struct pci_dev *pdev);

#endif /* _LATTE_MMIO_H_ */
