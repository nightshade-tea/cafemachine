#ifndef CAFE_DEVICE_H
#define CAFE_DEVICE_H

#include "qemu/osdep.h"
#include "cafe.h"
#include "hw/pci/msi.h"
#include "hw/pci/pci.h"
#include "qemu/log.h"

#define TYPE_CAFE_DEVICE CAFE_HW_NAME

/* https://elixir.bootlin.com/qemu/v10.1.0/source/include/qom/object.h#L220 */
OBJECT_DECLARE_TYPE(CafeState, CafeClass, CAFE_DEVICE);

typedef struct CafeClass {
  PCIDeviceClass parent_class;
} CafeClass;

typedef struct CafeState {
  PCIDevice pci_dev;
  MemoryRegion mmio;
  bool irq[CAFE_IRQ_MAX_VECTORS];
  uint64_t r[REG_MAX_NUM];
  uint8_t dma_buf[CAFE_DMA_BUF_SZ];
} CafeState;

#endif /* CAFE_DEVICE_H */
