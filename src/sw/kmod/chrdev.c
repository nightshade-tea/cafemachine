#include "chrdev.h"
#include "cafe.h"
#include "data.h"
#include "fops.h"

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

struct pci_dev *cafe_minor_to_dev(int minor) {
  return xa_load(&ctrl.minors_xa, minor);
}

int cafe_chrdev_create(struct pci_dev *pdev) {
  struct cafe_dev_data *data;
  struct device *dev;
  int err;

  dev = &pdev->dev;
  data = pci_get_drvdata(pdev);

  if ((err = allocate_minor(pdev))) {
    dev_err(dev, "allocate_minor() failed: %pe\n", ERR_PTR(err));
    goto err_allocate_minor;
  }

  if (IS_ERR(data->chrdev = device_create(ctrl.chrdev_class, dev, data->dev_num,
                                          NULL, CAFE_HW_NAME))) {
    dev_err(dev, "device_create() failed: %pe\n", data->chrdev);
    err = PTR_ERR(data->chrdev);
    goto err_device_create;
  }

  return 0;

err_device_create:
  free_minor(pdev);

err_allocate_minor:

  return err;
}

void cafe_chrdev_destroy(struct pci_dev *pdev) {
  struct cafe_dev_data *data;

  data = pci_get_drvdata(pdev);
  device_destroy(ctrl.chrdev_class, data->dev_num);
  free_minor(pdev);
}

int cafe_chrdev_init(void) {
  int err;

  xa_init_flags(&ctrl.minors_xa, XA_FLAGS_ALLOC);
  ctrl.minors_lim = MINORS_XA_LIMIT;

  cafe_init_fops(&ctrl.fops);

  /* If you pass a major number of 0 to register_chrdev, the return value
   * will be the dynamically allocated major number. */

  if ((ctrl.major = register_chrdev(0, CAFE_HW_NAME, &ctrl.fops)) < 0) {
    pr_err("register_chrdev() failed: %pe\n", ERR_PTR(ctrl.major));
    err = ctrl.major;
    goto err_register_chrdev;
  }

  if (IS_ERR(ctrl.chrdev_class = class_create(CAFE_HW_NAME))) {
    pr_err("class_create() failed: %pe\n", ctrl.chrdev_class);
    err = PTR_ERR(ctrl.chrdev_class);
    goto err_class_create;
  }

  return 0;

err_class_create:
  unregister_chrdev(ctrl.major, CAFE_HW_NAME);

err_register_chrdev:

  return err;
}

void cafe_chrdev_deinit(void) {
  class_destroy(ctrl.chrdev_class);
  unregister_chrdev(ctrl.major, CAFE_HW_NAME);
  xa_destroy(&ctrl.minors_xa);
}
