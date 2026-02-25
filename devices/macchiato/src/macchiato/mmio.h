#ifndef MACCHIATO_MMIO_H
#define MACCHIATO_MMIO_H

#include "irq.h"
#include "macchiato_device.h"
#include "macchiato_log.h"
#include <sys/param.h>

extern const MemoryRegionOps macchiato_reg_ops;

/* initializes MacchiatoState's mmio MemoryRegion and register BAR */
void macchiato_mmio_init(MacchiatoState *dev, Error **errp);

#endif /* MACCHIATO_MMIO_H */
