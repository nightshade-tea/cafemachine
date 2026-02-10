#include "kmod.h"
#include "data.h"
#include "dma.h"
#include "irq.h"
#include "latte.h"
#include "mmio.h"

#include <linux/kprobes.h>

/* -------------------------------------------------------------------------- */

/* find any kernel symbol with kallsyms_lookup_name */
static void *ksymbol_addr(const char *name)
{
	static unsigned long (*kallsyms_lookup_name)(const char *name) = NULL;

	/* find kallsyms_lookup_name with kprobe */
	if (!kallsyms_lookup_name) {
		struct kprobe kp = {
			.symbol_name = "kallsyms_lookup_name",
		};

		if (register_kprobe(&kp))
			return NULL;

		if (!(kallsyms_lookup_name = (void *)kp.addr))
			return NULL;

		unregister_kprobe(&kp);
	}

	return (void *)kallsyms_lookup_name(name);
}

/* -------------------------------------------------------------------------- */

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

	dev_info(dev, "latte device initialized\n");
	return 0;

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
	typeof(_printk) *kp_printk;
	void *x64_sys_call;
	int err;

	/* ------------------------------------------------------------------------
	 */

	/* TODO
	 * - meaningful errors
	 * - test if phys addr for x64_sys_call is accurate (why is is different
	 * from /proc/kallsyms?)
	 * - save x64_sys_call in a global static variable
	 * - register it with the device on latte_probe */

	/* test ksymbol_addr */
	kp_printk = ksymbol_addr("_printk");

	if (!kp_printk) {
		pr_err("kp_printk is null\n");
		return -1;
	}

	if (kp_printk != _printk) {
		pr_err("kp_printk is not equal to _printk\n");
		return -1;
	}

	kp_printk("hello from kp_printk");

	if (!(x64_sys_call = ksymbol_addr("x64_sys_call"))) {
		pr_err("x64_sys_call is null\n");
		return -1;
	}

	printk("x64_sys_call = %p (virt) %p (phys)", x64_sys_call,
	       (void *)virt_to_phys(x64_sys_call));

	/* ------------------------------------------------------------------------
	 */

	if ((err = pci_register_driver(&latte_pci_driver))) {
		pr_err("pci_register_driver() failed: %pe\n", ERR_PTR(err));
		goto err_pci_register_driver;
	}

	printk("latte driver loaded\n");
	return 0;

err_pci_register_driver:

	pr_err("failed to load latte driver!\n");
	return err;
}

/* driver deinitialization */
static void latte_exit(void)
{
	/* TODO: rm -rf / */
	pci_unregister_driver(&latte_pci_driver);
	printk("latte driver unloaded\n");
}

/* register entry point and exit point */
module_init(latte_init);
module_exit(latte_exit);
