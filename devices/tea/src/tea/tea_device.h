#ifndef TEA_DEVICE_H
#define TEA_DEVICE_H

#include "qemu/osdep.h"
#include "hw/pci/msi.h"
#include "hw/pci/pci.h"
#include "tea.h"

#define TYPE_TEA_DEVICE TEA_HW_NAME

/* this provides some niceties such as type-safe cast macros
 * https://elixir.bootlin.com/qemu/v10.1.0/source/include/qom/object.h#L220 */
OBJECT_DECLARE_TYPE(TeaState, TeaClass, TEA_DEVICE);

/* Class */
typedef struct TeaClass {
    PCIDeviceClass parent_class; /* inherit */
} TeaClass;

/* Object (State) */
typedef struct TeaState {
    PCIDevice pci_dev; /* inherit */

    int listen_sock; /* listen for inconming connections */

    /* dma read/write buffer */
    uint8_t dma_buf[TEA_DMA_BUF_SZ];
} TeaState;

#endif /* TEA_DEVICE_H */
