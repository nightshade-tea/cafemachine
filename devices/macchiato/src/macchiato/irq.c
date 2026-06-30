#include "irq.h"

void macchiato_irq_init(MacchiatoState *dev, Error **errp)
{
    /* create MemoryRegion and asign to the bar */
    int ret = msix_init_exclusive_bar(&dev->pci_dev, MACCHIATO_IRQ_CNT,
                                      MACCHIATO_BAR_ID_MSIX, errp);

    if (ret) {
        macchiato_log("failed to initialize msix\n");
        return;
    }

    for (uint32_t i = 0; i < MACCHIATO_IRQ_CNT; i++) {
        msix_vector_use(&dev->pci_dev, i);
        pthread_mutex_init(&dev->irq_locks[i], NULL);
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

    macchiato_log("grabbing lock for irq %s\n", macchiato_irq_str(vector));
    pthread_mutex_lock(&dev->irq_locks[vector]);

    dev->irq[vector] = true;
    msix_notify(&dev->pci_dev, vector);
    macchiato_log("irq %s raised\n", macchiato_irq_str(vector));
}

void macchiato_irq_lower(MacchiatoState *dev, unsigned int vector)
{
    if (vector >= MACCHIATO_IRQ_CNT) {
        return;
    }

    dev->irq[vector] = false;

    pthread_mutex_unlock(&dev->irq_locks[vector]);

    macchiato_log("irq %s lowered\n", macchiato_irq_str(vector));
}

void macchiato_irq_reset(MacchiatoState *dev)
{
    memset(&dev->irq, 0, sizeof dev->irq);
    macchiato_log("irq_reset(): all irqs lowered\n");
}

/* may be an issue  */
void wait_for_irq(MacchiatoState *dev, int vector)
{
    macchiato_log("waiting for irq lower vector %s\n",
                  macchiato_irq_str(vector));
    pthread_mutex_lock(&dev->irq_locks[vector]);
}
