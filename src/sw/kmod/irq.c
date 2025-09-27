#include "irq.h"
#include "data.h"
#include "hw/cafe.h"

static irqreturn_t cafe_irq_handler(int irq, void *data) {
    struct pci_dev *pdev;
    struct device *dev;
    struct cafe_dev_data *dev_data;

    pdev = (struct pci_dev *)data;
    dev = &pdev->dev;
    dev_data = pci_get_drvdata(pdev);

    dev_info(dev, "got interrupt %d!\n", irq);
    writeq(0, dev_data->bar.mmio);

    return IRQ_HANDLED;
}

int cafe_irq_enable(struct pci_dev *pdev) {
    struct device *dev;
    int irqn;
    int err;

    dev = &pdev->dev;

    if ((err = pci_alloc_irq_vectors(pdev, CAFE_HW_MSI_VECTOR_CNT,
                CAFE_HW_MSI_VECTOR_CNT, PCI_IRQ_MSI)) < 0)
    {
        dev_err(dev, "pci_alloc_irq_vectors() failed: %pe\n", ERR_PTR(err));
        goto err_pci_alloc_irq_vectors;
    }

    if ((irqn = pci_irq_vector(pdev, 0)) < 0) {
        err = irqn;
        dev_err(dev, "pci_irq_vector() failed: %pe\n", ERR_PTR(err));
        goto err_pci_irq_vector;
    }

    if ((err = request_irq(irqn, cafe_irq_handler, 0, CAFE_HW_NAME, pdev))) {
        dev_err(dev, "request_irq() failed: %pe\n", ERR_PTR(err));
        goto err_request_irq;
    }

    return 0;

err_request_irq:

err_pci_irq_vector:
    pci_free_irq_vectors(pdev);

err_pci_alloc_irq_vectors:

    return err;
}

void cafe_irq_disable(struct pci_dev *pdev) {
    int irqn;

    if ((irqn = pci_irq_vector(pdev, 0)) >= 0)
        free_irq(irqn, pdev);

    pci_free_irq_vectors(pdev);
}
