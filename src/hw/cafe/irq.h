#ifndef CAFE_IRQ_H
#define CAFE_IRQ_H

#include "cafe_device.h"
#include "qemu/osdep.h"
#include "hw/pci/pci.h"

#define CAFE_IRQ_MAX_VECTORS 32

typedef struct IRQStatusMSI {
    bool state[CAFE_IRQ_MAX_VECTORS];
} IRQStatusMSI;

void cafe_irq_init(CafeState *dev, Error **errp);

void cafe_irq_deinit(CafeState *dev);

void cafe_irq_raise(CafeState *dev, unsigned int vector);

void cafe_irq_lower(CafeState *dev, unsigned int vector);

void cafe_irq_reset(CafeState *dev);

#endif /* CAFE_IRQ_H */
