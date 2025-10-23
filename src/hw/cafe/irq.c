#include "irq.h"
#include "cafe.h"
#include "cafe_log.h"

void cafe_irq_init(CafeState *dev, Error **errp) {
  /* set device msi capability */
  if (msi_init(&dev->pci_dev, 0, CAFE_INT_VECTOR_CNT, true, false, errp))
    cafe_log("failed to initialize msi\n");
  cafe_log("irq initialized\n");
}

void cafe_irq_deinit(CafeState *dev) {
  cafe_irq_reset(dev);
  msi_uninit(&dev->pci_dev);
  cafe_log("irq deinitialized\n");
}

void cafe_irq_raise(CafeState *dev, unsigned int vector) {
  if (vector >= CAFE_INT_VECTOR_CNT)
    return;

  dev->irq[vector] = true;
  msi_notify(&dev->pci_dev, vector);

  cafe_log("irq %s raised\n", cafe_irq_vec_str(vector));
}

void cafe_irq_lower(CafeState *dev, unsigned int vector) {
  if (vector >= CAFE_INT_VECTOR_CNT)
    return;

  dev->irq[vector] = false;
  cafe_log("irq %s lowered\n", cafe_irq_vec_str(vector));
}

void cafe_irq_reset(CafeState *dev) {
  memset(&dev->irq, 0, sizeof dev->irq);
  cafe_log("irq_reset: all irqs lowered\n");
}
