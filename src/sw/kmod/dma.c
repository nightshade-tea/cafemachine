#include "dma.h"
#include "kmod.h"

int cafe_dma_enable(struct pci_dev *pdev) {
    return dma_set_mask_and_coherent(&pdev->dev, CAFE_DMA_BIT_MASK);
}
