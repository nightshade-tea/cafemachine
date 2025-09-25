#include "kmod.h"
#include "data.h"
#include "mmio.h"
#include "chrdev.h"
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

static int cafe_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
    struct cafe_dev_data *data;
    struct device *dev;
    int err;

    dev = &pdev->dev;

    dev_info(dev, "Found cafe device\n");

    if (!(data = cafe_dev_data_alloc())) {
        dev_err (dev, "cafe_dev_data_alloc() failed!");
        return -ENOMEM;
    }

    pci_set_drvdata (pdev, data);

    if ((err = pci_enable_device(pdev))) {
        dev_err (dev, "pci_enable_device() failed: %d\n", err);
        return err;
    }

    /* The driver can only determine MMIO and IO Port resource availability
     * _after_ calling pci_enable_device(). */

    if ((err = cafe_mmio_init(pdev))) {
        dev_err (dev, "cafe_mmio_init() failed: %d\n", err);
        return err;
    }

    if ((err = cafe_chrdev_create(pdev))) {
        dev_err (dev, "cafe_chrdev_create() failed: %d\n", err);
        return err;
    }

    dev_info(dev, "Initialized cafe device\n");
    return 0;
}

static void cafe_remove(struct pci_dev *pdev) {
    struct device *dev;

    dev = &pdev->dev;

    /* drivers should call pci_release_region() AFTER calling
     * pci_disable_device(). The idea is to prevent two devices colliding on
     * the same address range. */

    pci_disable_device(pdev);
    cafe_chrdev_destroy(pdev);
    cafe_mmio_deinit(pdev);

    cafe_dev_data_free(pci_get_drvdata(pdev));
    pci_set_drvdata(pdev, NULL);

    dev_info(dev, "Removed cafe device\n");
}

static struct pci_driver cafe_pci_driver = {
    .name = CAFE_HW_NAME,
    .id_table = cafe_id_table,
    .probe = cafe_probe,
    .remove= cafe_remove,
};

static int __init cafe_init(void) {
    int err;

    if ((err = cafe_chrdev_init()) < 0) {
        pr_err("cafe_chrdev_init() failed: %d\n", err);
        return err;
    }

    if ((err = pci_register_driver(&cafe_pci_driver))) {
        pr_err("pci_register_driver() failed: %d\n", err);
        return err;
    }

    printk("Cafe driver loaded\n");
    return 0;
}

static void cafe_exit(void) {
    pci_unregister_driver(&cafe_pci_driver);
    cafe_chrdev_deinit();
    printk("Cafe driver unloaded\n");
}

module_init(cafe_init);
module_exit(cafe_exit);
