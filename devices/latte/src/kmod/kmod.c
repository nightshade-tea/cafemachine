#include "kmod.h"
#include "data.h"
#include "dma.h"
#include "integrity.h"
#include "irq.h"
#include "latte.h"
#include "mmio.h"

/* specifies which devices the driver supports */
static struct pci_device_id latte_id_table[] = {
	{PCI_DEVICE(LATTE_VENDOR_ID, LATTE_DEVICE_ID)},
	{},
};

/* the pci_device_id structure needs to be exported to user space to allow the
 * hotplug and module loading systems know what module works with what hardware
 * devices. the macro MODULE_DEVICE_TABLE accomplishes this. */

MODULE_DEVICE_TABLE(pci, latte_id_table);

/* device initialization */
static int latte_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct latte_dev_data *data;
	struct device *dev;
	int err;

	dev = &pdev->dev;

	dev_info(dev, "found latte device\n");

	if (!(data = latte_dev_data_alloc())) {
		err = -ENOMEM;
		dev_err(dev, "latte_dev_data_alloc() failed: %pe\n",
			ERR_PTR(err));
		goto err_latte_dev_data_alloc;
	}

	/* store latte_dev_data on pdev->dev.driver_data */
	pci_set_drvdata(pdev, data);

	if ((err = pci_enable_device(pdev))) {
		dev_err(dev, "pci_enable_device() failed: %pe\n", ERR_PTR(err));
		goto err_pci_enable_device;
	}

	/* enable bus mastering for dma and msi */
	pci_set_master(pdev);

	/* enable memory-write-invalidate for dma */
	if ((err = pci_set_mwi(pdev))) {
		dev_err(dev, "pci_set_mwi() failed: %pe\n", ERR_PTR(err));
		goto err_pci_set_mwi;
	}

	/* the driver can only determine mmio and io port resource availability
	 * _after_ calling pci_enable_device(). */

	if ((err = latte_mmio_enable(pdev))) {
		dev_err(dev, "latte_mmio_enable() failed: %pe\n", ERR_PTR(err));
		goto err_latte_mmio_enable;
	}

	if ((err = latte_irq_enable(pdev))) {
		dev_err(dev, "latte_irq_enable() failed: %pe\n", ERR_PTR(err));
		goto err_latte_irq_enable;
	}

	if ((err = latte_dma_enable(pdev))) {
		dev_err(dev, "latte_dma_enable() failed: %pe\n", ERR_PTR(err));
		goto err_latte_dma_enable;
	}

	if ((err = latte_register_protected_regions(pdev))) {
		dev_err(dev, "latte_register_protected_regions() failed: %pe\n",
			ERR_PTR(err));
		goto err_latte_register_protected_regions;
	}

	dev_info(dev, "latte device initialized\n");
	return 0;

err_latte_register_protected_regions:
err_latte_dma_enable:
	latte_irq_disable(pdev);

err_latte_irq_enable:
	latte_mmio_disable(pdev);

err_latte_mmio_enable:
err_pci_set_mwi:
	pci_disable_device(pdev);

err_pci_enable_device:
	latte_dev_data_free(pci_get_drvdata(pdev));
	pci_set_drvdata(pdev, NULL);

err_latte_dev_data_alloc:

	dev_err(dev, "failed to initialize latte device!\n");
	return err;
}

/* device deinitialization */
static void latte_remove(struct pci_dev *pdev)
{
	struct device *dev;

	dev = &pdev->dev;

	latte_irq_disable(pdev);

	/* drivers should call pci_release_region() after calling
	 * pci_disable_device(). The idea is to prevent two devices colliding on
	 * the same address range. */

	pci_disable_device(pdev);
	latte_mmio_disable(pdev);

	latte_dev_data_free(pci_get_drvdata(pdev));
	pci_set_drvdata(pdev, NULL);

	dev_info(dev, "latte device removed\n");
}

/* latte driver struct */
static struct pci_driver latte_pci_driver = {
	.name = LATTE_HW_NAME,
	.id_table = latte_id_table,
	.probe = latte_probe,
	.remove = latte_remove,
};

/* driver initialization */
static int __init latte_init(void)
{
	int err;

	if ((err = latte_integrity_init())) {
		pr_err("latte_integrity_init() failed: %pe\n", ERR_PTR(err));
		goto err_exit;
	}

	if ((err = pci_register_driver(&latte_pci_driver))) {
		pr_err("pci_register_driver() failed: %pe\n", ERR_PTR(err));
		goto err_exit;
	}

	printk("latte driver loaded\n");
	return 0;

err_exit:

	pr_err("failed to load latte driver!\n");
	return err;
}

/* driver deinitialization */
static void latte_exit(void)
{
	pci_unregister_driver(&latte_pci_driver);
	printk("latte driver unloaded\n");
}

/* register entry point and exit point */
module_init(latte_init);
module_exit(latte_exit);
