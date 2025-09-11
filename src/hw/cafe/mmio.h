#ifndef CAFE_MMIO_H
#define CAFE_MMIO_H

#include "cafe_device.h"

/* registers read/write callbacks and sets constraints */
extern const MemoryRegionOps cafe_mmio_ops;

/* initializes CafeState's mmio MemoryRegion and register BAR */
void cafe_mmio_init(CafeState *dev, Error **errp);

#endif /* CAFE_MMIO_H */
