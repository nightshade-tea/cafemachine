#include "irq.h"

void macchiato_irq_init(MacchiatoState *dev, Error **errp)
{
    /* set device msi capability */
    if (msi_init(&dev->pci_dev, 0, MACCHIATO_IRQ_CNT, true, false, errp)) {
        macchiato_log("failed to initialize msi\n");
    }
    macchiato_log("irq initialized\n");
}

void macchiato_irq_deinit(MacchiatoState *dev)
{
    macchiato_irq_reset(dev);
    msi_uninit(&dev->pci_dev);
    macchiato_log("irq deinitialized\n");
}

void macchiato_irq_raise(MacchiatoState *dev, unsigned int vector)
{
    if (vector >= MACCHIATO_IRQ_CNT) {
        return;
    }

    dev->irq[vector] = true;
    msi_notify(&dev->pci_dev, vector);

    macchiato_log("irq %u raised\n", vector);
}

void macchiato_nmi_raise(MacchiatoState *dev, unsigned int vector)
{
    MSIMessage msg;

    msg = msi_get_message(&dev->pci_dev, vector);

    msg.data &= ~(0b011 << 8);
    msg.data |= 0b100 << 8;

    msi_send_message(&dev->pci_dev, msg);
}

void macchiato_irq_lower(MacchiatoState *dev, unsigned int vector)
{
    if (vector >= MACCHIATO_IRQ_CNT) {
        return;
    }

    dev->irq[vector] = false;
    macchiato_log("irq %u lowered\n", vector);
}

void macchiato_irq_reset(MacchiatoState *dev)
{
    memset(&dev->irq, 0, sizeof dev->irq);
    macchiato_log("irq_reset: all irqs lowered\n");
}
