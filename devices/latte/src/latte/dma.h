#ifndef LATTE_DMA_H
#define LATTE_DMA_H

#include "latte_device.h"

/* the parameters for the dma operations are set in the device's control
 * registers, accessible via mmio.
 *
 * contents are read/written to LatteState.dma_buf */

MemTxResult latte_dma_read(LatteState *dev);

MemTxResult latte_dma_write(LatteState *dev);

#endif /* LATTE_DMA_H */
