#ifndef MACCHIATO_DEVICE_H
#define MACCHIATO_DEVICE_H

#include <errno.h>
#include <pthread.h>
#include <string.h>

#include "qemu/osdep.h"
#include "hw/pci/msi.h"
#include "hw/pci/pci.h"

#include "irq.h"
#include "macchiato.h"
#include "macchiato_log.h"
#include "mmio.h"

#define TYPE_MACCHIATO_DEVICE MACCHIATO_HW_NAME

/* this provides some niceties such as type-safe cast macros
 * https://elixir.bootlin.com/qemu/v10.1.0/source/include/qom/object.h#L220 */
OBJECT_DECLARE_TYPE(MacchiatoState, MacchiatoClass, MACCHIATO_DEVICE);

/* Class */
typedef struct MacchiatoClass {
    PCIDeviceClass parent_class; /* inherit */
} MacchiatoClass;

/* Object (State) */
typedef struct MacchiatoState {
    PCIDevice pci_dev; /* inherit */

    /* mmio */
    MemoryRegion bar[MACCHIATO_BAR_ID_CNT];
    uint64_t reg[MACCHIATO_REG_CNT];
    uint8_t *ram;

    /* raised interrupts vector */
    bool irq[MACCHIATO_IRQ_MAX_VECTORS];

} MacchiatoState;

#endif /* MACCHIATO_DEVICE_H */
