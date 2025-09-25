#include "chrdev.h"
#include "data.h"
#include "hw/cafe.h"

#define MINORS_XA_LIMIT XA_LIMIT(0, 255)

static struct chrdev_ctrl {
    struct class *chrdev_class;
    struct file_operations fops;
    int major;
    struct xarray minors_xa;
    struct xa_limit minors_lim;
} ctrl;

static int allocate_minor(struct pci_dev *pdev) {
    struct cafe_dev_data *data;
    int minor;
    int err;

    data = pci_get_drvdata(pdev);

    if (!(err = xa_alloc(&ctrl.minors_xa, &minor, pdev, ctrl.minors_lim,
                    GFP_KERNEL)))
        data->dev_num = MKDEV(ctrl.major, minor);

    return err;
}

static void free_minor(struct pci_dev *pdev) {
    struct cafe_dev_data *data;

    data = pci_get_drvdata(pdev);
    xa_erase(&ctrl.minors_xa, MINOR(data->dev_num));
}

static struct pci_dev *cafe_minor_to_dev(int minor) {
    return xa_load(&ctrl.minors_xa, minor);
}

int cafe_chrdev_create(struct pci_dev *pdev) {
    struct cafe_dev_data *data;
    struct device *dev;
    int err;

    dev = &pdev->dev;
    data = pci_get_drvdata(pdev);

    if ((err = allocate_minor(pdev))) {
        dev_err (dev, "allocate_minor() failed: %d\n", err);
        return err;
    }

    data->chrdev = device_create(ctrl.chrdev_class, dev, data->dev_num, NULL,
            CAFE_HW_NAME);

    if (IS_ERR(data->chrdev)) {
        dev_err(dev, "device_create() failed!\n");
        return PTR_ERR(data->chrdev);
    }

    return 0;
}

void cafe_chrdev_destroy(struct pci_dev *pdev) {
    struct cafe_dev_data *data;

    data = pci_get_drvdata(pdev);
    device_destroy(ctrl.chrdev_class, data->dev_num);
    free_minor(pdev);
}

int cafe_chrdev_init(void) {

    xa_init(&ctrl.minors_xa);
    ctrl.minors_lim = MINORS_XA_LIMIT;

    /* If you pass a major number of 0 to register_chrdev, the return value
     * will be the dynamically allocated major number. */

    if ((ctrl.major = register_chrdev(0, CAFE_HW_NAME, &ctrl.fops)) < 0) {
        pr_err("register_chrdev() failed: %d\n", ctrl.major);
        return ctrl.major;
    }

    ctrl.chrdev_class = class_create(CAFE_HW_NAME);

    if (IS_ERR(ctrl.chrdev_class)) {
        pr_err("class_create() failed!\n");
        return PTR_ERR(ctrl.chrdev_class);
    }

    return 0;
}

void cafe_chrdev_deinit(void) {
    class_destroy(ctrl.chrdev_class);
    unregister_chrdev(ctrl.major, CAFE_HW_NAME);
    xa_destroy(&ctrl.minors_xa);
}
