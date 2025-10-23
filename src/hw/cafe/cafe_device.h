#ifndef CAFE_DEVICE_H
#define CAFE_DEVICE_H

#include "qemu/osdep.h"
#include "cafe.h"
#include "hw/pci/msi.h"
#include "hw/pci/pci.h"

#define TYPE_CAFE_DEVICE CAFE_HW_NAME

/* this provides some niceties such as type-safe cast macros
 * https://elixir.bootlin.com/qemu/v10.1.0/source/include/qom/object.h#L220 */
OBJECT_DECLARE_TYPE(CafeState, CafeClass, CAFE_DEVICE);

/* Class */
typedef struct CafeClass {
  PCIDeviceClass parent_class; /* inherit */
} CafeClass;

/* Object (State) */
typedef struct CafeState {
  PCIDevice pci_dev; /* inherit */

  /* mmio */
  MemoryRegion mmio;
  uint64_t reg[CAFE_REG_CNT];

  /* raised interrupts vector */
  bool irq[CAFE_IRQ_MAX_VECTORS];

  /* dma read/write buffer */
  uint8_t dma_buf[CAFE_DMA_BUF_SZ];
} CafeState;

#endif /* CAFE_DEVICE_H */
