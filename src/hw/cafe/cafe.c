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

#define TYPE_CAFE_DEVICE   "cafe"
#define CAFE_DEVICE_DESC   "cafe pci device"
#define CAFE_HW_VENDOR_ID  0xcafe /* Chrysalis-ITS */
#define CAFE_HW_DEVICE_ID  0xcafe
#define CAFE_HW_REVISION   0x00

typedef PCIDeviceClass CafeClass;

typedef struct CafeState {
    PCIDevice pdev;
    uint32_t reg;
} CafeState;

static void cafe_class_init(ObjectClass *klass, const void *data) {
    DeviceClass *device_class = DEVICE_CLASS(klass);
    PCIDeviceClass *pci_device_class = PCI_DEVICE_CLASS(klass);

    pci_device_class->vendor_id = CAFE_HW_VENDOR_ID;
    pci_device_class->device_id = CAFE_HW_DEVICE_ID;
    pci_device_class->revision = CAFE_HW_REVISION;
    pci_device_class->class_id = PCI_CLASS_OTHERS;

    set_bit(DEVICE_CATEGORY_MISC, device_class->categories);
    device_class->desc = CAFE_DEVICE_DESC;
}

/* most devices in QEMU do not expect to be unrealized. Only devices which are
 * hot-unpluggable should be unrealized (as part of the unplugging process);
 * all other devices are expected to last for the life of the simulation and
 * should not be unrealized and freed. */

static const TypeInfo cafe_info = {
    .name = TYPE_CAFE_DEVICE,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(CafeState),
    .class_init = cafe_class_init,
    .interfaces =
        (InterfaceInfo[]){
            { INTERFACE_CONVENTIONAL_PCI_DEVICE },
            {}
        },
};

static void cafe_register_types(void) {
    type_register_static(&cafe_info);
}

type_init(cafe_register_types)
