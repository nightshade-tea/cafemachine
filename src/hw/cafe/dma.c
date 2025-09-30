#include "dma.h"

MemTxResult cafe_dma_read(CafeState *dev, void *buf) {
   return pci_dma_read (&dev->pci_dev, dev->mem.dma_src, buf, dev->mem.dma_sz);   
}

MemTxResult cafe_dma_write(CafeState *dev, void *buf) {
   return pci_dma_write (&dev->pci_dev, dev->mem.dma_dst, buf, dev->mem.dma_sz);   
}

