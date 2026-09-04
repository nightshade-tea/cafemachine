#include "irq.h"

void macchiato_irq_init(MacchiatoState *dev, Error **errp)
{
    /* create MemoryRegion and assign it to the bar */
    if (msix_init_exclusive_bar(&dev->pci_dev, MACCHIATO_IRQ_CNT,
                                MACCHIATO_BAR_ID_MSIX, errp)) {
        macchiato_log("failed to initialize msix\n");
        return;
    }

    for (int i = 0; i < MACCHIATO_IRQ_CNT; ++i) {
        msix_vector_use(&dev->pci_dev, i);
        dev->irq[i].raised = false;
        pthread_mutex_init(&dev->irq[i].lock, 0);
        pthread_cond_init(&dev->irq[i].handled, 0);
    }

    macchiato_log("irq initialized\n");
}

void macchiato_irq_deinit(MacchiatoState *dev)
{
    macchiato_irq_reset(dev);
    msix_uninit_exclusive_bar(&dev->pci_dev);
    /* deinit other stuff...  */
    macchiato_log("irq deinitialized\n");
}

void macchiato_irq_raise(MacchiatoState *dev, unsigned int vector)
{
    if (vector >= MACCHIATO_IRQ_CNT) {
        return;
    }

    pthread_mutex_lock(&dev->irq[vector].lock);

    if (!dev->irq[vector].raised) {
        dev->irq[vector].raised = true;
        msix_notify(&dev->pci_dev, vector);
        macchiato_log("irq %s raised\n", macchiato_irq_str(vector));
    }

    pthread_mutex_unlock(&dev->irq[vector].lock);
}

void macchiato_irq_raise_wait(MacchiatoState *dev, unsigned int vector)
{
    if (vector >= MACCHIATO_IRQ_CNT) {
        return;
    }

    pthread_mutex_lock(&dev->irq[vector].lock);

    if (dev->irq[vector].raised) {
        pthread_mutex_unlock(&dev->irq[vector].lock);
        return;
    }

    dev->irq[vector].raised = true;
    msix_notify(&dev->pci_dev, vector);
    macchiato_log("irq %s raised, waiting...\n", macchiato_irq_str(vector));

    while (dev->irq[vector].raised) {
        pthread_cond_wait(&dev->irq[vector].handled, &dev->irq[vector].lock);
    }

    macchiato_log("irq %s handled, continuing\n", macchiato_irq_str(vector));

    pthread_mutex_unlock(&dev->irq[vector].lock);
}

void macchiato_irq_lower(MacchiatoState *dev, unsigned int vector)
{
    if (vector >= MACCHIATO_IRQ_CNT) {
        return;
    }

    pthread_mutex_lock(&dev->irq[vector].lock);

    if (dev->irq[vector].raised) {
        dev->irq[vector].raised = false;
        pthread_cond_signal(&dev->irq[vector].handled);
        macchiato_log("irq %s lowered\n", macchiato_irq_str(vector));
    }

    pthread_mutex_unlock(&dev->irq[vector].lock);
}

void macchiato_irq_reset(MacchiatoState *dev)
{
    // memset(&dev->irq, 0, sizeof dev->irq);
    macchiato_log("irq_reset(): all irqs lowered\n");
}
