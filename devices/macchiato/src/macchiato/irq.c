#include "irq.h"

void macchiato_irq_init(MacchiatoState *dev, Error **errp)
{
    /* create MemoryRegion and asign to the bar */
    int ret = msix_init_exclusive_bar(&dev->pci_dev, MACCHIATO_IRQ_CNT,
                                      MACCHIATO_BAR_ID_MSIX, errp);

    if (!ret) {
        macchiato_log("failed to initialize msix\n");
        return;
    }

    for (uint32_t i = 0; i < MACCHIATO_IRQ_CNT; i++) {
        msix_vector_use(&dev->pci_dev, i);
    }

    macchiato_log("irq initialized\n");
}

void macchiato_irq_deinit(MacchiatoState *dev)
{
    macchiato_irq_reset(dev);
    msix_uninit_exclusive_bar(&dev->pci_dev);
    macchiato_log("irq deinitialized\n");
}

void macchiato_irq_raise(MacchiatoState *dev, unsigned int vector)
{
    if (vector >= MACCHIATO_IRQ_CNT) {
        return;
    }

    dev->irq[vector] = true;
    msix_notify(&dev->pci_dev, vector);

    macchiato_log("irq %u raised\n", vector);
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
