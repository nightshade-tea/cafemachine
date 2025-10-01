#include "dma.h"
#include "kmod.h"
#include "data.h"
#include "cafe.h"

int cafe_dma_enable(struct pci_dev *pdev) {
    struct device *dev;
    struct cafe_dev_data *data;
    char *buf;

    dev = &pdev->dev;
    data = pci_get_drvdata(pdev);

    dma_set_mask_and_coherent(&pdev->dev, CAFE_DMA_BIT_MASK);

    buf = kzalloc(512, GFP_KERNEL);

    for (int i = 0 ; i < 256 ; i += 2) {
        buf[i] = 0xca;
        buf[i + 1] = 0xfe;
    }

    writeq(256, data->bar.mmio + CAFE_DMA_SZ * 8);
    writeq(virt_to_phys(buf), data->bar.mmio + CAFE_DMA_SRC * 8);
    writeq(CAFE_DMA_READ, data->bar.mmio + CAFE_CMD * 8);

    writeq(256, data->bar.mmio + CAFE_DMA_SZ * 8);
    writeq(virt_to_phys(buf + 256), data->bar.mmio + CAFE_DMA_DST * 8);
    writeq(CAFE_DMA_WRITE, data->bar.mmio + CAFE_CMD * 8);

    writeq(512, data->bar.mmio + CAFE_DMA_SZ * 8);
    writeq(virt_to_phys(buf), data->bar.mmio + CAFE_DMA_SRC * 8);
    writeq(CAFE_DMA_READ, data->bar.mmio + CAFE_CMD * 8);

    kfree(buf);
    return 0;
}
