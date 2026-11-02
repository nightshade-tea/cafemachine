#include "irq.h"
#include "data.h"
#include "macchiato.h"
#include "mmio.h"

/* this sucks  */
#define prk(fmt, ...) printk("%s(): " fmt "\n", __func__, ##__VA_ARGS__)

/* actual handler  */
static irqreturn_t sync_handler(int hw_irqn, void *opaque)
{
	struct pci_dev *pdev;
	struct macchiato_dev_data *data;

	prk("got %s", macchiato_irq_str(hw_irqn));

	pdev = (struct pci_dev *)opaque;
	data = pci_get_drvdata(pdev);

	data->last_hwirq = hw_irqn;
	complete(&data->sync);

	prk("lowering %s", macchiato_irq_str(hw_irqn));
	macchiato_reg_write(pdev, MACCHIATO_REG_LOWER_IRQ, hw_irqn);

	prk("handled %s", macchiato_irq_str(hw_irqn));
	return IRQ_HANDLED;
}

static irqreturn_t buf_op_handler(int hw_irqn, void *opaque)
{
	struct pci_dev *pdev;
	struct macchiato_dev_data *data;

	prk("got %s", macchiato_irq_str(hw_irqn));

	pdev = (struct pci_dev *)opaque;
	data = pci_get_drvdata(pdev);

	data->last_hwirq = hw_irqn;
	complete(&data->sync);

	/* DO NOT LOWER!!
	 * device should stay blocked until user resolves this buf op.
	 * next ioctl will lower it after writing to REG_CMD */

	prk("handled %s", macchiato_irq_str(hw_irqn));
	return IRQ_HANDLED;
}

/* dummy handlers just so we dont have to make a mapping of virtual irq numbers
 * to device irq numbers... in our case they just happen to all do the same
 * thing */

static irqreturn_t done_handler(int irq, void *opaque)
{
	return sync_handler(MACCHIATO_IRQ_DONE, opaque);
}

static irqreturn_t failed_handler(int irq, void *opaque)
{
	return sync_handler(MACCHIATO_IRQ_FAILED, opaque);
}

static irqreturn_t next_buf_handler(int irq, void *opaque)
{
	return buf_op_handler(MACCHIATO_IRQ_NEXT_BUF, opaque);
}

static irqreturn_t first_buf_handler(int irq, void *opaque)
{
	return buf_op_handler(MACCHIATO_IRQ_FIRST_BUF, opaque);
}

static const irq_handler_t handlers[MACCHIATO_IRQ_CNT] = {
	[MACCHIATO_IRQ_DONE] = done_handler,
	[MACCHIATO_IRQ_FAILED] = failed_handler,
	[MACCHIATO_IRQ_NEXT_BUF] = next_buf_handler,
	[MACCHIATO_IRQ_FIRST_BUF] = first_buf_handler,
};

int macchiato_irq_enable(struct pci_dev *pdev)
{
	struct device *dev;
	struct macchiato_dev_data *data;
	int err, hw_irqn, irqn;

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
	int hw_irqn, irqn;

	hw_irqn = MACCHIATO_IRQ_CNT;

	while (hw_irqn--) {
		irqn = pci_irq_vector(pdev, hw_irqn);
		free_irq(irqn, pdev);
	}

	pci_free_irq_vectors(pdev);
}
