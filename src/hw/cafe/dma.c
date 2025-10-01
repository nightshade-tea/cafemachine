#include "dma.h"
#include "cafe_log.h"

MemTxResult cafe_dma_read(CafeState *dev, void *buf) {
   MemTxResult ret;
   cafe_log ("got read of size %lu from %lu \n", dev->r[CAFE_DMA_SZ], dev->r[CAFE_DMA_SRC]);

   ret = pci_dma_read (&dev->pci_dev, dev->r[CAFE_DMA_SRC], buf, dev->r[CAFE_DMA_SZ]);
   if (ret != MEMTX_OK) {
      cafe_log ("failed read\n");
      return ret;
   }

   cafe_log ("got this thing\n\n");

  for (int i = 0; i < dev->r[CAFE_DMA_SZ]; i++)
    qemu_log("%x ", ((uint8_t *)buf)[i]);

  qemu_log("\n");

   return ret;   
}

MemTxResult cafe_dma_write(CafeState *dev, void *buf) {
   return pci_dma_write (&dev->pci_dev, dev->r[CAFE_DMA_DST], buf, dev->r[CAFE_DMA_SZ]);   
}

