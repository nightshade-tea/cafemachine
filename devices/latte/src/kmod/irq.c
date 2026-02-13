#include "irq.h"
#include "data.h"
#include "latte.h"

static irqreturn_t latte_handler(int irq, void *opaque)
{
	struct pci_dev *pdev;
	struct device *dev;
	struct latte_dev_data *data;
	int dev_irq;

	pdev = (struct pci_dev *)opaque;
	dev = &pdev->dev;
	data = pci_get_drvdata(pdev);

	dev_irq = irq - data->irq.base;

	dev_info(dev, "got interrupt %d -> %d (%s)\n", irq, dev_irq,
		 latte_irq_vec_str(dev_irq));

	switch (dev_irq) {
	case LATTE_INT_SYNC:
		complete(&data->devop_done[LATTE_WAIT_NOTIFY]);
		break;
	default:
		dev_warn(dev, "unhandled interrupt %d -> %d !\n", irq, dev_irq);
		break;
	}

	writeq(dev_irq, data->bar.mmio + LATTE_LOWER_INT * 8);

	return IRQ_HANDLED;
}

int latte_irq_enable(struct pci_dev *pdev)
{
	struct device *dev;
	struct latte_dev_data *data;
	int err, irqn;

	dev = &pdev->dev;
	data = pci_get_drvdata(pdev);

	/* for some reason if we try to allocate more than 1 irq_vec we get
	 * -ENOSPC ... */

	/* allocate a contiguous block of interrupt vectors for our device */
	if ((data->irq.allocated = pci_alloc_irq_vectors(
		     pdev, LATTE_INT_VECTOR_CNT, LATTE_INT_VECTOR_CNT,
		     PCI_IRQ_MSI)) < 0) {
		err = data->irq.allocated;
		dev_err(dev, "pci_alloc_irq_vectors() failed: %pe\n",
			ERR_PTR(err));
		goto err_pci_alloc_irq_vectors;
	}

	data->irq.base = pci_irq_vector(pdev, 0);

	/* register the handlers for each interrupt vector */
	for (int i = 0; i < data->irq.allocated; i++) {
		irqn = data->irq.base + i;

		if ((err = request_irq(irqn, latte_handler, 0, LATTE_HW_NAME,
				       pdev))) {
			dev_err(dev, "request_irq() failed (irqn=%d): %pe\n",
				irqn, ERR_PTR(err));
			goto err_request_irq;
		}
	}

	return 0;

err_request_irq:

	while (data->irq.base <= --irqn)
		free_irq(irqn, pdev);

	pci_free_irq_vectors(pdev);

err_pci_alloc_irq_vectors:

	return err;
}

void latte_irq_disable(struct pci_dev *pdev)
{
	struct latte_dev_data *data;

	data = pci_get_drvdata(pdev);

	for (int i = 0; i < data->irq.allocated; i++)
		free_irq(data->irq.base + i, pdev);

	pci_free_irq_vectors(pdev);
}
