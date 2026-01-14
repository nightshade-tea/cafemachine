#ifndef LATTE_IRQ_H
#define LATTE_IRQ_H

#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "latte_device.h"

void latte_irq_init(LatteState *dev, Error **errp);

void latte_irq_deinit(LatteState *dev);

void latte_irq_raise(LatteState *dev, unsigned int vector);

void latte_irq_lower(LatteState *dev, unsigned int vector);

void latte_irq_reset(LatteState *dev);

#endif /* LATTE_IRQ_H */
