#ifndef LATTE_MMIO_H
#define LATTE_MMIO_H

#include "latte_device.h"

/* defines read/write callbacks and sets access constraints */
extern const MemoryRegionOps latte_mmio_ops;

/* initializes LatteState's mmio MemoryRegion and register BAR */
void latte_mmio_init(LatteState *dev, Error **errp);

#endif /* LATTE_MMIO_H */
