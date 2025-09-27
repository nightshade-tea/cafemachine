#ifndef _CAFE_IRQ_H_
#define _CAFE_IRQ_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

int cafe_irq_enable(struct pci_dev *pdev);

void cafe_irq_disable(struct pci_dev *pdev);

#endif /* _CAFE_IRQ_H_ */
