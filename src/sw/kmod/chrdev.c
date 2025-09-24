#include "chrdev.h"
#include "data.h"
#include "hw/cafe.h"

static struct chrdev_ctrl {
    struct class *class;
    struct file_operations fops;
    int major;
} ctrl;

int cafe_chrdev_create(struct pci_dev *pdev) {
    struct cafe_dev_data *data;
    struct device *dev;

    dev = &pdev->dev;

    if (!(data = pci_get_drvdata(pdev))) {
        dev_err (dev, "pci_get_drvdata() returned NULL!");
        return -ENOMEM;
    }

    data->dev_num = MKDEV(ctrl.major, 0/* minor */);

    data->chrdev = device_create(ctrl.class, dev, data->dev_num, NULL,
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
    device_destroy(ctrl.class, data->dev_num);
}

int cafe_chrdev_init(void) {

    /* If you pass a major number of 0 to register_chrdev, the return value
     * will be the dynamically allocated major number. */

    if ((ctrl.major = register_chrdev(0, CAFE_HW_NAME, &ctrl.fops)) < 0) {
        pr_err("register_chrdev() failed: %d\n", ctrl.major);
        return ctrl.major;
    }

    ctrl.class = class_create(CAFE_HW_NAME);

    if (IS_ERR(ctrl.class)) {
        pr_err("class_create() failed!\n");
        return PTR_ERR(ctrl.class);
    }

    return 0;
}

void cafe_chrdev_deinit(void) {
    class_destroy(ctrl.class);
    unregister_chrdev(ctrl.major, CAFE_HW_NAME);
}
