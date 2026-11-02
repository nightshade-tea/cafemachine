#ifndef MACCHIATO_DMA_H
#define MACCHIATO_DMA_H

#include "macchiato_device.h"

int macchiato_dma_read(MacchiatoState *dev, hwaddr addr, hwaddr size);

int macchiato_dma_read_off(MacchiatoState *dev, size_t off, hwaddr addr,
                           hwaddr size);

MemTxResult macchiato_dma_write(MacchiatoState *dev, hwaddr dest, void *src,
                                size_t size);

#endif /* MACCHIATO_DMA_H */
