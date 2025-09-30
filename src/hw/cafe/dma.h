#ifndef CAFE_DMA_H
#define CAFE_DMA_H

#include "cafe_device.h"

MemTxResult cafe_dma_read(CafeState *dev, void *buf);

MemTxResult cafe_dma_write(CafeState *dev, void *buf);

#endif /* CAFE_DMA_H */
