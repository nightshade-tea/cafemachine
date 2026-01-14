#ifndef _LATTE_IRQ_H_
#define _LATTE_IRQ_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

int latte_irq_enable(struct pci_dev *pdev);

void latte_irq_disable(struct pci_dev *pdev);

#endif /* _LATTE_IRQ_H_ */
