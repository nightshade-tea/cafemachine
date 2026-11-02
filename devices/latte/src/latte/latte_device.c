#include "latte_device.h"
#include "irq.h"
#include "latte_log.h"
#include "mmio.h"
#include "mon.h"
#include <errno.h>
#include <pthread.h>
#include <string.h>

/* instantiate a latte device object (LatteState) */
static void latte_realize(PCIDevice *pci_dev, Error **errp)
{
    LatteState *dev = LATTE_DEVICE(pci_dev);
    uint16_t cmd, wmask, cmask;
    pthread_t latte_thread;
    int err;

    /* set pci express capability on pci configuration space */
    if (pci_bus_is_express(pci_get_bus(pci_dev)) &&
        pcie_endpoint_cap_init(pci_dev, 0) >= 0) {
        latte_log("express capability set\n");
    }

    latte_mmio_init(dev, errp);
    latte_irq_init(dev, errp);

    /* enable bus mastering */
    memory_region_set_enabled(&pci_dev->bus_master_enable_region, true);

    /* mask bus master bit to avoid OS overwrite */
    cmd = pci_get_word(pci_dev->config + PCI_COMMAND);
    cmd |= PCI_COMMAND_MASTER;
    pci_set_word(pci_dev->config + PCI_COMMAND, cmd);

    cmask = pci_get_word(pci_dev->cmask + PCI_COMMAND);
    cmask |= PCI_COMMAND_MASTER;
    pci_set_word(pci_dev->cmask + PCI_COMMAND, cmask);

    wmask = pci_get_word(pci_dev->wmask + PCI_COMMAND);
    wmask &= ~PCI_COMMAND_MASTER;
    pci_set_word(pci_dev->wmask + PCI_COMMAND, wmask);

    pthread_mutex_lock(&lock);
    if ((err = pthread_create(&latte_thread, NULL, mon_thread, dev))) {
        latte_log("failed to create monitor thread: %s\n", strerror(err));
    }

    while (!ready_to_surveil) {
        latte_log("preso\n");
    }
    pthread_mutex_unlock(&lock);

    latte_log("device realized\n");
}

/* initialize latte device class (LatteClass) methods and static attributes */
static void latte_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *device_class = DEVICE_CLASS(klass);
    PCIDeviceClass *pci_device_class = PCI_DEVICE_CLASS(klass);

    pci_device_class->realize = latte_realize;
    pci_device_class->vendor_id = LATTE_VENDOR_ID;
    pci_device_class->device_id = LATTE_DEVICE_ID;
    pci_device_class->class_id = PCI_CLASS_OTHERS;
    set_bit(DEVICE_CATEGORY_MISC, device_class->categories);

    latte_log("class initialized\n");
}

/* register latte device type on qemu (Class + State) */
static const TypeInfo latte_info = {
    .name = TYPE_LATTE_DEVICE,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(LatteState),
    .class_init = latte_class_init,
    .interfaces = (InterfaceInfo[]){{INTERFACE_PCIE_DEVICE}, {}},
};

static void latte_register_types(void)
{
    type_register_static(&latte_info);
}

type_init(latte_register_types)
