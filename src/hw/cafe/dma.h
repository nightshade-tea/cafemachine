#ifndef CAFE_DMA_H
#define CAFE_DMA_H

#include "cafe_device.h"

/* the parameters for the dma operations are set in the device's control
 * registers, accessible via mmio.
 *
 * contents are read/written to CafeState.dma_buf */

MemTxResult cafe_dma_read(CafeState *dev);

MemTxResult cafe_dma_write(CafeState *dev);

#endif /* CAFE_DMA_H */
