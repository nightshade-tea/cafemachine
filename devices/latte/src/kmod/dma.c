#include "dma.h"
#include "data.h"
#include "kmod.h"
#include "latte.h"

int latte_dma_enable(struct pci_dev *pdev)
{
	struct device *dev;
	struct latte_dev_data *data;

	dev = &pdev->dev;
	data = pci_get_drvdata(pdev);

	dma_set_mask_and_coherent(&pdev->dev, LATTE_DMA_BIT_MASK);

	return 0;
}
