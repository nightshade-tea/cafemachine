#include "macchiato_device.h"
#include "irq.h"
#include "macchiato_log.h"
#include "mmio.h"
#include <pthread.h>
#include <yara.h>

void *mmon_thread(void *pci_dev)
{
    MacchiatoState *dev = MACCHIATO_DEVICE(pci_dev);

    while (1) {
        sleep(10);
        macchiato_irq_raise(dev, 0);
        macchiato_log("raised\n");
    }

    return NULL;
}

/* instantiate a macchiato device object (MacchiatoState) */
static void macchiato_realize(PCIDevice *pci_dev, Error **errp)
{
    MacchiatoState *dev = MACCHIATO_DEVICE(pci_dev);

    /* set pci express capability on pci configuration space */
    if (pci_bus_is_express(pci_get_bus(pci_dev)) &&
        pcie_endpoint_cap_init(pci_dev, 0) >= 0) {
        macchiato_log("express capability set\n");
    }

    macchiato_mmio_init(dev, errp);
    macchiato_irq_init(dev, errp);

    if (yr_compiler_create(&dev->compiler)) {
        macchiato_log("failed to create yara compiler\n");
    }

    // macchiato_irq_raise(dev, 0);

    pthread_t latte_thread;
    int err;
    if ((err = pthread_create(&latte_thread, NULL, mmon_thread, dev))) {
        macchiato_log("failed to create monitor thread: %s\n", strerror(err));
    }

    macchiato_log("device realized\n");
}

/* initialize macchiato device class (MacchiatoClass) methods and static
 * attributes */
static void macchiato_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *device_class = DEVICE_CLASS(klass);
    PCIDeviceClass *pci_device_class = PCI_DEVICE_CLASS(klass);

    pci_device_class->realize = macchiato_realize;
    pci_device_class->vendor_id = MACCHIATO_VENDOR_ID;
    pci_device_class->device_id = MACCHIATO_DEVICE_ID;
    pci_device_class->class_id = PCI_CLASS_OTHERS;
    set_bit(DEVICE_CATEGORY_MISC, device_class->categories);

    if (yr_initialize()) {
        macchiato_log("failed to initialize yara\n");
    }

    macchiato_log("class initialized\n");
}

/* register macchiato device type on qemu (Class + State) */
static const TypeInfo macchiato_info = {
    .name = TYPE_MACCHIATO_DEVICE,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(MacchiatoState),
    .class_init = macchiato_class_init,
    .interfaces = (InterfaceInfo[]){{INTERFACE_PCIE_DEVICE}, {}},
};

static void macchiato_register_types(void)
{
    type_register_static(&macchiato_info);
}

type_init(macchiato_register_types)
