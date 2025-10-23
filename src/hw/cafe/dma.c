#include "dma.h"
#include "cafe_log.h"

MemTxResult cafe_dma_read(CafeState *dev) {
  MemTxResult err;

  err = pci_dma_read(&dev->pci_dev, dev->reg[CAFE_DMA_SRC], dev->dma_buf,
                     dev->reg[CAFE_DMA_SZ]);

  if (err != MEMTX_OK) {
    cafe_log("failed dma read: sz=%#" PRIx64 " src_addr=%#" PRIx64 "\n",
             dev->reg[CAFE_DMA_SZ], dev->reg[CAFE_DMA_SRC]);
    memset(dev->dma_buf, 0, CAFE_DMA_BUF_SZ); /* zero dma_buf on error */
  }

  return err;
}

MemTxResult cafe_dma_write(CafeState *dev) {
  MemTxResult err;

  err = pci_dma_write(&dev->pci_dev, dev->reg[CAFE_DMA_DST], dev->dma_buf,
                      dev->reg[CAFE_DMA_SZ]);

  if (err != MEMTX_OK)
    cafe_log("failed dma write: sz=%#" PRIx64 " dst_addr=%#" PRIx64 "\n",
             dev->reg[CAFE_DMA_SZ], dev->reg[CAFE_DMA_DST]);

  return err;
}
