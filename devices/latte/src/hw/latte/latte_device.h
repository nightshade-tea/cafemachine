#ifndef LATTE_DEVICE_H
#define LATTE_DEVICE_H

#include "qemu/osdep.h"
#include "hw/pci/msi.h"
#include "hw/pci/pci.h"
#include "latte.h"

#define TYPE_LATTE_DEVICE LATTE_HW_NAME

/* this provides some niceties such as type-safe cast macros
 * https://elixir.bootlin.com/qemu/v10.1.0/source/include/qom/object.h#L220 */
OBJECT_DECLARE_TYPE(LatteState, LatteClass, LATTE_DEVICE);

/* Class */
typedef struct LatteClass {
    PCIDeviceClass parent_class; /* inherit */
} LatteClass;

/* Object (State) */
typedef struct LatteState {
    PCIDevice pci_dev; /* inherit */

    /* mmio */
    MemoryRegion mmio;
    uint64_t reg[LATTE_REG_CNT];

    /* raised interrupts vector */
    bool irq[LATTE_IRQ_MAX_VECTORS];

    /* dma read/write buffer */
    uint8_t dma_buf[LATTE_DMA_BUF_SZ];
} LatteState;

#endif /* LATTE_DEVICE_H */
