#include "irq.h"
#include "latte.h"
#include "latte_log.h"

void latte_irq_init(LatteState *dev, Error **errp)
{
    /* set device msi capability */
    if (msi_init(&dev->pci_dev, 0, LATTE_INT_VECTOR_CNT, true, false, errp)) {
        latte_log("failed to initialize msi\n");
    }
    latte_log("irq initialized\n");
}

void latte_irq_deinit(LatteState *dev)
{
    latte_irq_reset(dev);
    msi_uninit(&dev->pci_dev);
    latte_log("irq deinitialized\n");
}

void latte_irq_raise(LatteState *dev, unsigned int vector)
{
    if (vector >= LATTE_INT_VECTOR_CNT) {
        return;
    }

    dev->irq[vector] = true;
    msi_notify(&dev->pci_dev, vector);

    latte_log("irq %s raised\n", latte_irq_vec_str(vector));
}

void latte_nmi_raise(LatteState *dev, unsigned int vector)
{
    MSIMessage msg;

    msg = msi_get_message(&dev->pci_dev, vector);

    msg.data &= ~(0b011 << 8);
    msg.data |= 0b100 << 8;

    msi_send_message(dev, msg);
}

void latte_irq_lower(LatteState *dev, unsigned int vector)
{
    if (vector >= LATTE_INT_VECTOR_CNT) {
        return;
    }

    dev->irq[vector] = false;
    latte_log("irq %s lowered\n", latte_irq_vec_str(vector));
}

void latte_irq_reset(LatteState *dev)
{
    memset(&dev->irq, 0, sizeof dev->irq);
    latte_log("irq_reset: all irqs lowered\n");
}
