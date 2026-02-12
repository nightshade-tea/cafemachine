#ifndef _LATTE_MMIO_H_
#define _LATTE_MMIO_H_

#include "data.h"
#include "latte.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

int latte_mmio_enable(struct pci_dev *pdev);

void latte_mmio_disable(struct pci_dev *pdev);

static inline void latte_reg_write(struct pci_dev *pdev, enum latte_reg reg,
				   u64 val)
{
	struct latte_dev_data *data = pci_get_drvdata(pdev);
	writeq(val, data->bar.mmio + reg * 8);
}

#endif /* _LATTE_MMIO_H_ */
