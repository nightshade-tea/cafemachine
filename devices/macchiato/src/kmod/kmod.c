#include "kmod.h"
#include "data.h"
#include "irq.h"
#include "macchiato.h"
#include "mmio.h"

/* specifies which devices the driver supports */
static struct pci_device_id macchiato_id_table[] = {
	{PCI_DEVICE(MACCHIATO_VENDOR_ID, MACCHIATO_DEVICE_ID)},
	{},
};

/* the pci_device_id structure needs to be exported to user space to allow the
 * hotplug and module loading systems know what module works with what hardware
 * devices. the macro MODULE_DEVICE_TABLE accomplishes this. */

MODULE_DEVICE_TABLE(pci, macchiato_id_table);

/* device initialization */
static int macchiato_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct macchiato_dev_data *data;
	struct device *dev;
	int err;

	dev = &pdev->dev;

	dev_info(dev, "macchiato_probe()...");

	if (!(data = macchiato_dev_data_alloc())) {
		err = -ENOMEM;
		dev_err(dev, "macchiato_dev_data_alloc() failed: %pe\n",
			ERR_PTR(err));
		goto err_macchiato_dev_data_alloc;
	}

	/* store macchiato_dev_data on pdev->dev.driver_data */
	pci_set_drvdata(pdev, data);

	if ((err = pci_enable_device(pdev))) {
		dev_err(dev, "pci_enable_device() failed: %pe\n", ERR_PTR(err));
		goto err_pci_enable_device;
	}

	/* enable bus mastering for dma and msi */
	pci_set_master(pdev);

	/* the driver can only determine mmio and io port resource availability
	 * _after_ calling pci_enable_device(). */

	if ((err = macchiato_mmio_enable(pdev))) {
		dev_err(dev, "macchiato_mmio_enable() failed: %pe\n",
			ERR_PTR(err));
		goto err_macchiato_mmio_enable;
	}

	if ((err = macchiato_irq_enable(pdev))) {
		dev_err(dev, "macchiato_irq_enable() failed: %pe\n",
			ERR_PTR(err));
		goto err_macchiato_irq_enable;
	}

	pr_cont(" ok.\n");
	return 0;

err_macchiato_irq_enable:
	macchiato_mmio_disable(pdev);

err_macchiato_mmio_enable:
	pci_disable_device(pdev);

err_pci_enable_device:
	macchiato_dev_data_free(pci_get_drvdata(pdev));
	pci_set_drvdata(pdev, NULL);

err_macchiato_dev_data_alloc:

	dev_err(dev, "macchiato_probe() failed: %pe\n", ERR_PTR(err));
	return err;
}

/* device deinitialization */
static void macchiato_remove(struct pci_dev *pdev)
{
	struct device *dev;

	dev = &pdev->dev;
	dev_info(dev, "macchiato_remove()...");

	macchiato_irq_disable(pdev);

	/* drivers should call pci_release_region() after calling
	 * pci_disable_device(). The idea is to prevent two devices colliding on
	 * the same address range. */

	pci_disable_device(pdev);
	macchiato_mmio_disable(pdev);

	macchiato_dev_data_free(pci_get_drvdata(pdev));
	pci_set_drvdata(pdev, NULL);

	pr_cont(" ok.\n");
}

/* macchiato driver struct */
static struct pci_driver macchiato_pci_driver = {
	.name = MACCHIATO_HW_NAME,
	.id_table = macchiato_id_table,
	.probe = macchiato_probe,
	.remove = macchiato_remove,
};

/* driver initialization */
static int __init macchiato_init(void)
{
	int err;

	pr_info("macchiato_init()...");

	if ((err = pci_register_driver(&macchiato_pci_driver))) {
		pr_err("pci_register_driver() failed: %pe\n", ERR_PTR(err));
		goto err_exit;
	}

	pr_cont(" ok.\n");
	return 0;

err_exit:

	pr_err("macchiato_init() failed: %pe\n", ERR_PTR(err));
	return err;
}

/* driver deinitialization */
static void macchiato_exit(void)
{
	pr_info("macchiato_exit()...");
	pci_unregister_driver(&macchiato_pci_driver);
	pr_cont(" ok.\n");
}

/* register entry point and exit point */
module_init(macchiato_init);
module_exit(macchiato_exit);
