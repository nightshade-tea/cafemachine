#include "irq.h"
#include "cafe_log.h"
#include "cafe.h"

void cafe_irq_init(CafeState *dev, Error **errp) {
    if (msi_init(&dev->pci_dev, 0, CAFE_HW_MSI_VECTOR_CNT, true, false, errp))
        cafe_log("failed to initialize msi\n");
    cafe_log("irq initialized\n");
}

void cafe_irq_deinit(CafeState *dev) {
    cafe_irq_reset(dev);
    msi_uninit(&dev->pci_dev);
    cafe_log("irq deinitialized\n");
}

void cafe_irq_raise(CafeState *dev, unsigned int vector) {
    if (vector >= CAFE_HW_MSI_VECTOR_CNT)
        return;

    dev->irq[vector] = true;
    msi_notify(&dev->pci_dev, vector);

    cafe_log("irq raised\n");
}

void cafe_irq_lower(CafeState *dev, unsigned int vector) {
    if (vector >= CAFE_HW_MSI_VECTOR_CNT)
        return;

    dev->irq[vector] = false;
    cafe_log("irq lowered\n");
}

void cafe_irq_reset(CafeState *dev) {
    memset(&dev->irq, 0, sizeof dev->irq);
    cafe_log("irq reset\n");
}
