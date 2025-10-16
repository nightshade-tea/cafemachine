#include "dma.h"
#include "cafe_log.h"

MemTxResult cafe_dma_read(CafeState *dev) {
  MemTxResult err;

  err = pci_dma_read(&dev->pci_dev, dev->reg[CAFE_DMA_SRC], dev->dma_buf,
                     dev->reg[CAFE_DMA_SZ]);

  /* zero dma_buf on error */
  if (err != MEMTX_OK) {
    cafe_log("failed read\n");
    memset(dev->dma_buf, 0, CAFE_DMA_BUF_SZ);
  }

  return err;
}

MemTxResult cafe_dma_write(CafeState *dev) {
  return pci_dma_write(&dev->pci_dev, dev->reg[CAFE_DMA_DST], dev->dma_buf,
                       dev->reg[CAFE_DMA_SZ]);
}
