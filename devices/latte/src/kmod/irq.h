#ifndef _LATTE_IRQ_H_
#define _LATTE_IRQ_H_

#include "data.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

int latte_irq_enable(struct pci_dev *pdev);

void latte_irq_disable(struct pci_dev *pdev);

static inline void latte_sync(struct pci_dev *pdev, enum latte_complete id)
{
	struct latte_dev_data *data = pci_get_drvdata(pdev);
	struct completion *c = &data->devop_done[id];

	wait_for_completion(c);
	reinit_completion(c);
}

#endif /* _LATTE_IRQ_H_ */
