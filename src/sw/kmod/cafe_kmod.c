#include "cafe_kmod.h"
#include "hw/cafe.h"

MODULE_LICENSE("GPL");

/* specifies which devices the driver supports */
static struct pci_device_id cafe_id_table[] = {
    { PCI_DEVICE(CAFE_VENDOR_ID, CAFE_DEVICE_ID) },
    {},
};

/* The pci_device_id structure needs to be exported to user space to allow the
 * hotplug and module loading systems know what module works with what hardware
 * devices. The macro MODULE_DEVICE_TABLE accomplishes this. */
MODULE_DEVICE_TABLE(pci, cafe_id_table);

static int cafe_probe(struct pci_dev *dev, const struct pci_device_id *id) {
    return pci_enable_device(dev);
}

static void cafe_remove(struct pci_dev *dev) {
    pci_disable_device(dev);
}

static struct pci_driver cafe_pci_driver = {
    .name = CAFE_HW_NAME,
    .id_table = cafe_id_table,
    .probe = cafe_probe,
    .remove= cafe_remove,
};

static int __init cafe_init(void) {
    printk("Cafe driver loaded\n");
    return pci_register_driver(&cafe_pci_driver);
}

static void cafe_exit(void) {
    pci_unregister_driver(&cafe_pci_driver);
}

module_init(cafe_init);
module_exit(cafe_exit);
