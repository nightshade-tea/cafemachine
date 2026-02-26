#include "chardev.h"
#include "data.h"
#include "macchiato.h"
#include <linux/cdev.h>

static struct class *class;
static dev_t base_devt;
static DEFINE_XARRAY_ALLOC(minors_xa);

static inline struct pci_dev *minor_pdev(int minor)
{
	return xa_load(&minors_xa, minor);
}

/* -------------------------------------------------------------------------- */

static int open(struct inode *inode, struct file *file)
{
	printk("macchiato open()!\n");
	return 0;
}

/* -------------------------------------------------------------------------- */

static const struct file_operations f_ops = {
	.owner = THIS_MODULE,
	.open = open,
};

/* create a character device file for a macchiato device */
int macchiato_chardev_create(struct pci_dev *pdev)
{
	struct macchiato_dev_data *data;
	struct device *dev;
	void *errp;
	int err, minor;

	dev = &pdev->dev;
	data = pci_get_drvdata(pdev);

	if ((err = xa_alloc(&minors_xa, &minor, pdev, XA_LIMIT(0, MINORMASK),
			    GFP_KERNEL))) {
		dev_err(dev, "xa_alloc() failed: %pe\n", ERR_PTR(err));
		goto err_xa_alloc;
	}

	data->devt = MKDEV(MAJOR(base_devt), minor);

	cdev_init(&data->cdev, &f_ops);
	data->cdev.owner = THIS_MODULE;

	if ((err = cdev_add(&data->cdev, data->devt, 1))) {
		dev_err(dev, "cdev_add() failed: %pe\n", ERR_PTR(err));
		goto err_cdev_add;
	}

	/* create the device file in /dev */
	if (IS_ERR(errp = device_create(class, dev, data->devt, NULL,
					MACCHIATO_HW_NAME))) {
		dev_err(dev, "device_create() failed: %pe\n", errp);
		err = PTR_ERR(errp);
		goto err_device_create;
	}

	return 0;

err_device_create:
	cdev_del(&data->cdev);

err_cdev_add:
	xa_erase(&minors_xa, MINOR(data->devt));

err_xa_alloc:
	return err;
}

void macchiato_chardev_delete(struct pci_dev *pdev)
{
	struct macchiato_dev_data *data;

	data = pci_get_drvdata(pdev);
	device_destroy(class, data->devt);
	cdev_del(&data->cdev);
	xa_erase(&minors_xa, MINOR(data->devt));
}

int macchiato_chardev_init(void)
{
	int err;

	if ((err = alloc_chrdev_region(&base_devt, 0, MINORMASK,
				       MACCHIATO_HW_NAME))) {
		pr_err("alloc_chrdev_region() failed: %pe\n", ERR_PTR(err));
		goto err_alloc_chrdev_region;
	}

	if (IS_ERR(class = class_create(MACCHIATO_HW_NAME))) {
		pr_err("class_create() failed: %pe\n", class);
		err = PTR_ERR(class);
		goto err_class_create;
	}

	return 0;

err_class_create:
	unregister_chrdev_region(base_devt, MINORMASK);

err_alloc_chrdev_region:
	return err;
}

void macchiato_chardev_deinit(void)
{
	class_destroy(class);
	unregister_chrdev_region(base_devt, MINORMASK);
	xa_destroy(&minors_xa);
}
