#include "cafe_kmod.h"
#include "cafe_data.h"
#include "cafe_mmio.h"
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
    int err;

    dev_info(&dev->dev, "Found cafe device\n");

    if (!(dev->dev.driver_data = cafe_dev_data_alloc())) {
        dev_err (&dev->dev, "cafe_dev_data_alloc() failed!");
        return -ENOMEM;
    }

    if ((err = pci_enable_device(dev))) {
        dev_err (&dev->dev, "pci_enable_device() failed: %d\n", err);
        return err;
    }

    if ((err = cafe_mmio_init(dev))) {
        dev_err (&dev->dev, "cafe_mmio_init() failed: %d\n", err);
        return err;
    }

    dev_info(&dev->dev, "Initialized cafe device\n");
    return 0;
}

static void cafe_remove(struct pci_dev *dev) {
    pci_disable_device(dev);

    cafe_mmio_deinit(dev);
    cafe_dev_data_free(dev->dev.driver_data);
    dev->dev.driver_data = NULL;

    dev_info(&dev->dev, "Removed cafe device\n");
}

static struct pci_driver cafe_pci_driver = {
    .name = CAFE_HW_NAME,
    .id_table = cafe_id_table,
    .probe = cafe_probe,
    .remove= cafe_remove,
};

static int __init cafe_init(void) {
    int err;

    if ((err = pci_register_driver(&cafe_pci_driver))) {
        pr_err("pci_register_driver() failed: %d\n", err);
        return err;
    }

    printk("Cafe driver loaded\n");
    return 0;
}

static void cafe_exit(void) {
    pci_unregister_driver(&cafe_pci_driver);
    printk("Cafe driver unloaded\n");
}

module_init(cafe_init);
module_exit(cafe_exit);
