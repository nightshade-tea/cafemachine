#ifndef _MACCHIATO_MMIO_H_
#define _MACCHIATO_MMIO_H_

#include "data.h"
#include "macchiato.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

int macchiato_mmio_enable(struct pci_dev *pdev);

void macchiato_mmio_disable(struct pci_dev *pdev);

static inline u64 macchiato_reg_read(struct pci_dev *pdev,
				     enum macchiato_reg reg)
{
	struct macchiato_dev_data *data = pci_get_drvdata(pdev);
	return readq(data->bar[MACCHIATO_BAR_ID_CTL].mmio + reg * 8);
}

static inline void macchiato_reg_write(struct pci_dev *pdev,
				       enum macchiato_reg reg, u64 val)
{
	struct macchiato_dev_data *data = pci_get_drvdata(pdev);
	writeq(val, data->bar[MACCHIATO_BAR_ID_CTL].mmio + reg * 8);
}

#endif /* _MACCHIATO_MMIO_H_ */
