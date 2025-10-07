#ifndef _CAFE_DMA_H_
#define _CAFE_DMA_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

int cafe_dma_enable(struct pci_dev *pdev);

#endif /* _CAFE_DMA_H_ */
