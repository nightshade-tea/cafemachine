#ifndef CAFE_IRQ_H
#define CAFE_IRQ_H

#include "qemu/osdep.h"
#include "cafe_device.h"
#include "hw/pci/pci.h"

void cafe_irq_init(CafeState *dev, Error **errp);

void cafe_irq_deinit(CafeState *dev);

void cafe_irq_raise(CafeState *dev, unsigned int vector);

void cafe_irq_lower(CafeState *dev, unsigned int vector);

void cafe_irq_reset(CafeState *dev);

#endif /* CAFE_IRQ_H */
