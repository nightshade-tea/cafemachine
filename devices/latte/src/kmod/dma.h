#ifndef _LATTE_DMA_H_
#define _LATTE_DMA_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

/* set dma access mask */
int latte_dma_enable(struct pci_dev *pdev);

#endif /* _LATTE_DMA_H_ */
