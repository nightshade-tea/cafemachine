#ifndef CAFE_DEVICE_H
#define CAFE_DEVICE_H

/* todo: remove unneeded includes */
#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/units.h"
#include "hw/pci/pci.h"
#include "hw/pci/msi.h"
#include "qemu/timer.h"
#include "qom/object.h"
#include "qemu/main-loop.h" /* iothread mutex */
#include "qemu/module.h"
#include "qapi/visitor.h"
#include "hw/cafe/cafe.h"

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
    uint64_t buf[CAFE_MMIO_SIZE / 8];
} CafeState;

#endif /* CAFE_DEVICE_H */
