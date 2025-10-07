#include "dma.h"
#include "cafe.h"
#include "data.h"
#include "kmod.h"

int cafe_dma_enable(struct pci_dev *pdev) {
  struct device *dev;
  struct cafe_dev_data *data;

  dev = &pdev->dev;
  data = pci_get_drvdata(pdev);

  dma_set_mask_and_coherent(&pdev->dev, CAFE_DMA_BIT_MASK);

  return 0;
}
