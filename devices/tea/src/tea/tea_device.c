#include "tea_device.h"
#include "tea_log.h"

/* instantiate a tea device object (TeaState) */
static void tea_realize(PCIDevice *pci_dev, Error **errp)
{
    uint16_t cmd, wmask, cmask;

    /* set pci express capability on pci configuration space */
    if (pci_bus_is_express(pci_get_bus(pci_dev)) &&
        pcie_endpoint_cap_init(pci_dev, 0) >= 0) {
        tea_log("express capability set\n");
    }

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

    tea_log("device realized\n");
}

/* initialize tea device class (TeaClass) methods and static attributes */
static void tea_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *device_class = DEVICE_CLASS(klass);
    PCIDeviceClass *pci_device_class = PCI_DEVICE_CLASS(klass);

    pci_device_class->realize = tea_realize;
    pci_device_class->vendor_id = TEA_VENDOR_ID;
    pci_device_class->device_id = TEA_DEVICE_ID;
    pci_device_class->class_id = PCI_CLASS_OTHERS;
    set_bit(DEVICE_CATEGORY_MISC, device_class->categories);

    tea_log("class initialized\n");
}

/* register tea device type on qemu (Class + State) */
static const TypeInfo tea_info = {
    .name = TYPE_TEA_DEVICE,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(TeaState),
    .class_init = tea_class_init,
    .interfaces = (InterfaceInfo[]){{INTERFACE_PCIE_DEVICE}, {}},
};

static void tea_register_types(void)
{
    type_register_static(&tea_info);
}

type_init(tea_register_types)
