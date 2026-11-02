#ifndef MACCHIATO_IRQ_H
#define MACCHIATO_IRQ_H

#include "qemu/osdep.h"
#include "hw/pci/msix.h"
#include "hw/pci/pci.h"
#include "macchiato.h"
#include "macchiato_device.h"
#include "macchiato_log.h"

void macchiato_irq_init(MacchiatoState *dev, Error **errp);

void macchiato_irq_deinit(MacchiatoState *dev);

void macchiato_irq_raise(MacchiatoState *dev, unsigned int vector);
void macchiato_irq_raise_wait(MacchiatoState *dev, unsigned int vector);

void macchiato_nmi_raise(MacchiatoState *dev, unsigned int vector);

void macchiato_irq_lower(MacchiatoState *dev, unsigned int vector);

void macchiato_irq_reset(MacchiatoState *dev);

void wait_for_irq(MacchiatoState *dev, int vector);

#endif /* MACCHIATO_IRQ_H */
