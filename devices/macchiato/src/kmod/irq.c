#include "irq.h"
#include "data.h"
#include "macchiato.h"

static irqreturn_t sync_handler(int irq, void *opaque)
{
	struct pci_dev *pdev;
	struct macchiato_dev_data *data;

	pdev = (struct pci_dev *)opaque;
	data = pci_get_drvdata(pdev);

	complete(&data->sync);
	macchiato_reg_write(pdev, MACCHIATO_LOWER_INT, MACCHIATO_IRQ_SYNC);

	return IRQ_HANDLED;
}

static const irq_handler_t handlers[MACCHIATO_IRQ_CNT] = {
	[MACCHIATO_IRQ_SYNC] = sync_handler,
};

int macchiato_irq_enable(struct pci_dev *pdev)
{
	struct device *dev;
	struct macchiato_dev_data *data;
	enum macchiato_irq hw_irqn;
	int err, irqn;

	dev = &pdev->dev;
	data = pci_get_drvdata(pdev);

	/* allocate interrupt vectors */
	if ((err = pci_alloc_irq_vectors(pdev, MACCHIATO_IRQ_CNT,
					 MACCHIATO_IRQ_CNT, PCI_IRQ_MSIX)) <
	    0) {
		dev_err(dev, "pci_alloc_irq_vectors() failed: %pe\n",
			ERR_PTR(err));
		goto err_pci_alloc_irq_vectors;
	}

	/* register the handlers for each interrupt vector */
	for (hw_irqn = 0; hw_irqn < MACCHIATO_IRQ_CNT; hw_irqn++) {
		irqn = pci_irq_vector(pdev, hw_irqn);

		if ((err = request_irq(irqn, handlers[hw_irqn], 0,
				       MACCHIATO_HW_NAME, pdev))) {
			dev_err(dev, "request_irq() failed (irqn=%d): %pe\n",
				irqn, ERR_PTR(err));
			goto err_request_irq;
		}
	}

	return 0;

err_request_irq:

	while (hw_irqn--) {
		irqn = pci_irq_vector(pdev, hw_irqn);
		free_irq(irqn, pdev);
	}

	pci_free_irq_vectors(pdev);

err_pci_alloc_irq_vectors:

	return err;
}

void macchiato_irq_disable(struct pci_dev *pdev)
{
	enum macchiato_irq hw_irqn;
	int irqn;

	hw_irqn = MACCHIATO_IRQ_CNT;

	while (hw_irqn--) {
		irqn = pci_irq_vector(pdev, hw_irqn);
		free_irq(irqn, pdev);
	}

	pci_free_irq_vectors(pdev);
}
