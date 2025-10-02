#include "cafe_device.h"
#include "cafe_log.h"
#include "mmio.h"
#include "irq.h"

static void cafe_realize(PCIDevice *pci_dev, Error **errp) {
    CafeState *dev = CAFE_DEVICE(pci_dev);

    if (pci_bus_is_express(pci_get_bus(pci_dev))) {
      cafe_log("we are an espresso\n");
      if (pcie_endpoint_cap_init (pci_dev, 0) < 0)
        cafe_log("we failed to be an endpointers :c\n");
    }

    cafe_mmio_init(dev, errp);
    cafe_irq_init(dev, errp);
    cafe_log("device realized\n");
}

static void cafe_class_init(ObjectClass *klass, const void *data) {
    DeviceClass *device_class = DEVICE_CLASS(klass);
    PCIDeviceClass *pci_device_class = PCI_DEVICE_CLASS(klass);

    pci_device_class->realize = cafe_realize;
    pci_device_class->vendor_id = CAFE_VENDOR_ID;
    pci_device_class->device_id = CAFE_DEVICE_ID;
    pci_device_class->class_id = PCI_CLASS_OTHERS;

    set_bit(DEVICE_CATEGORY_MISC, device_class->categories);
    cafe_log("class initialized\n");
}

static const TypeInfo cafe_info = {
    .name = TYPE_CAFE_DEVICE,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(CafeState),
    .class_init = cafe_class_init,
    .interfaces =
      (InterfaceInfo[]){
        { INTERFACE_PCIE_DEVICE },
        {}
      },
};

static void cafe_register_types(void) {
    type_register_static(&cafe_info);
}

type_init(cafe_register_types)
