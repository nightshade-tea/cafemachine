#include "dma.h"
#include "latte_log.h"

MemTxResult latte_dma_read(LatteState *dev)
{
    MemTxResult err;

    err = pci_dma_read(&dev->pci_dev, dev->reg[LATTE_DMA_SRC], dev->dma_buf,
                       dev->reg[LATTE_DMA_SZ]);

    if (err != MEMTX_OK) {
        latte_log("failed dma read: sz=%#" PRIx64 " src_addr=%#" PRIx64 "\n",
                  dev->reg[LATTE_DMA_SZ], dev->reg[LATTE_DMA_SRC]);
        memset(dev->dma_buf, 0, LATTE_DMA_BUF_SZ); /* zero dma_buf on error */
    }

    return err;
}

MemTxResult latte_dma_write(LatteState *dev)
{
    MemTxResult err;

    err = pci_dma_write(&dev->pci_dev, dev->reg[LATTE_DMA_DST], dev->dma_buf,
                        dev->reg[LATTE_DMA_SZ]);

    if (err != MEMTX_OK) {
        latte_log("failed dma write: sz=%#" PRIx64 " dst_addr=%#" PRIx64 "\n",
                  dev->reg[LATTE_DMA_SZ], dev->reg[LATTE_DMA_DST]);
    }

    return err;
}
