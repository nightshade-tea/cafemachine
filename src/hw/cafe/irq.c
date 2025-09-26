#include "irq.h"
#include "hw/cafe/cafe.h"

void cafe_irq_init(CafeState *dev, Error **errp) {
    if (msi_init(&dev->pci_dev, 0, CAFE_HW_MSI_VECTOR_CNT, true, false, errp))
        qemu_log("failed to initialize MSI\n");
}

void cafe_irq_deinit(CafeState *dev) {
    cafe_irq_reset(dev);
    msi_uninit(&dev->pci_dev);
}

void cafe_irq_raise(CafeState *dev, unsigned int vector) {
    if (vector >= CAFE_HW_MSI_VECTOR_CNT)
        return;

    dev->irq[vector] = true;
    msi_notify(&dev->pci_dev, vector);
}

void cafe_irq_lower(CafeState *dev, unsigned int vector) {
    if (vector >= CAFE_HW_MSI_VECTOR_CNT)
        return;

    dev->irq[vector] = false;
}

void cafe_irq_reset(CafeState *dev) {
    memset(&dev->irq, 0, sizeof dev->irq);
}
