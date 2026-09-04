#ifndef _MACCHIATO_MMIO_H_
#define _MACCHIATO_MMIO_H_

#include "data.h"
#include "macchiato.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

int macchiato_mmio_enable(struct pci_dev *pdev);

void macchiato_mmio_disable(struct pci_dev *pdev);

static inline u64 macchiato_reg_read_d(struct macchiato_dev_data *data,
				       enum macchiato_reg reg)
{
	return readq((char *)data->bar[MACCHIATO_BAR_ID_CTL].mmio + reg * 8);
}

static inline void macchiato_reg_write_d(struct macchiato_dev_data *data,
					 enum macchiato_reg reg, u64 val)
{
	writeq(val, (char *)data->bar[MACCHIATO_BAR_ID_CTL].mmio + reg * 8);
}

static inline u64 macchiato_reg_read(struct pci_dev *pdev,
				     enum macchiato_reg reg)
{
	return macchiato_reg_read_d(pci_get_drvdata(pdev), reg);
}

static inline void macchiato_reg_write(struct pci_dev *pdev,
				       enum macchiato_reg reg, u64 val)
{
	macchiato_reg_write_d(pci_get_drvdata(pdev), reg, val);
}

#endif /* _MACCHIATO_MMIO_H_ */
