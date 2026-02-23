#ifndef _MACCHIATO_IRQ_H_
#define _MACCHIATO_IRQ_H_

#include "data.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

int macchiato_irq_enable(struct pci_dev *pdev);

void macchiato_irq_disable(struct pci_dev *pdev);

static inline void macchiato_sync(struct pci_dev *pdev)
{
	struct macchiato_dev_data *data = pci_get_drvdata(pdev);
	struct completion *c = &data->sync;

	wait_for_completion(c);
	reinit_completion(c);
}

#endif /* _MACCHIATO_IRQ_H_ */
