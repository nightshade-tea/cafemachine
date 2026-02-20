#ifndef MACCHIATO_MMIO_H
#define MACCHIATO_MMIO_H

#include <sys/param.h>

#include "irq.h"
#include "macchiato_device.h"
#include "macchiato_log.h"

/* initializes MacchiatoState's mmio MemoryRegion and register BAR */
void macchiato_mmio_init(MacchiatoState *dev, Error **errp);

#endif /* MACCHIATO_MMIO_H */
