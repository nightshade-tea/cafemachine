#include "cafe_device.h"
#include "cafe_log.h"
#include "irq.h"
#include "mmio.h"

/* instantiate a cafe device object (CafeState) */
static void cafe_realize(PCIDevice *pci_dev, Error **errp) {
  CafeState *dev = CAFE_DEVICE(pci_dev);
  uint16_t cmd, wmask, cmask;

  /* set pci express capability on pci configuration space */
  if (pci_bus_is_express(pci_get_bus(pci_dev)) &&
      pcie_endpoint_cap_init(pci_dev, 0) >= 0)
    cafe_log("express capability set\n");

  cafe_mmio_init(dev, errp);
  cafe_irq_init(dev, errp);

  /* enable bus mastering */
  memory_region_set_enabled(&pci_dev->bus_master_enable_region, true);
  pci_dev->is_master = true;

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

  cafe_log("device realized\n");
}

/* initialize cafe device class (CafeClass) methods and static attributes */
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

/* register cafe device type on qemu (Class + State) */
static const TypeInfo cafe_info = {
    .name = TYPE_CAFE_DEVICE,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(CafeState),
    .class_init = cafe_class_init,
    .interfaces = (InterfaceInfo[]){{INTERFACE_PCIE_DEVICE}, {}},
};

static void cafe_register_types(void) { type_register_static(&cafe_info); }

type_init(cafe_register_types)
