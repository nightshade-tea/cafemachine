#ifndef _MACCHIATO_IRQ_H_
#define _MACCHIATO_IRQ_H_

#include "data.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

int macchiato_irq_enable(struct pci_dev *pdev);

void macchiato_irq_disable(struct pci_dev *pdev);

#endif /* _MACCHIATO_IRQ_H_ */
