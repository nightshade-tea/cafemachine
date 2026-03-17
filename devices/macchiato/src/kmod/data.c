#include "data.h"
#include "macchiato.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

int macchiato_dev_data_alloc(struct pci_dev *pdev)
{
	struct macchiato_dev_data **pdata;

	/* who gives a fuck  */
	pdata = (struct macchiato_dev_data **)&pdev->dev.driver_data;

	if (!(*pdata = kzalloc(sizeof **pdata, GFP_KERNEL)))
		return -ENOMEM;

	(*pdata)->pdev = pdev;
	init_completion(&(*pdata)->sync);

	return 0;
}

void macchiato_dev_data_free(struct pci_dev *pdev)
{
	kfree(pci_get_drvdata(pdev));
}
