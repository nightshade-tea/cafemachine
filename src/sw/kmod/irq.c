#include "irq.h"
#include "cafe.h"
#include "data.h"

/* interrupt handlers */
static irqreturn_t cafe_handler_dma_buf_available(int irq, void *data) {
  struct pci_dev *pdev;
  struct device *dev;
  struct cafe_dev_data *dev_data;

  pdev = (struct pci_dev *)data;
  dev = &pdev->dev;
  dev_data = pci_get_drvdata(pdev);

  // dev_info(dev, "got interrupt %d (DMA_BUF_AVAILABLE)!\n", irq);

  complete(&dev_data->devop_done[CAFE_WAIT_DMA]);

  writeq(CAFE_INT_DMA_BUF_AVAILABLE, dev_data->bar.mmio + CAFE_LOWER_INT * 8);

  return IRQ_HANDLED;
}

/* interrupt handlers array */
static irqreturn_t (*const handlers[CAFE_INT_VECTOR_CNT])(int, void *) = {
    cafe_handler_dma_buf_available};

int cafe_irq_enable(struct pci_dev *pdev) {
  struct device *dev;
  int err;

  dev = &pdev->dev;

  /* allocate a contiguous block of interrupt vectors for our device */
  if ((err = pci_alloc_irq_vectors(pdev, CAFE_INT_VECTOR_CNT,
                                   CAFE_INT_VECTOR_CNT, PCI_IRQ_MSI)) < 0) {
    dev_err(dev, "pci_alloc_irq_vectors() failed: %pe\n", ERR_PTR(err));
    goto err_pci_alloc_irq_vectors;
  }

  /* register the handlers for each interrupt vector */
  for (int i = 0; i < CAFE_INT_VECTOR_CNT; i++) {
    int irqn;

    if ((irqn = pci_irq_vector(pdev, i)) < 0) {
      err = irqn;
      dev_err(dev, "pci_irq_vector() failed: %pe\n", ERR_PTR(err));
      goto err_pci_irq_vector;
    }

    if ((err = request_irq(irqn, handlers[i], i, CAFE_HW_NAME, pdev))) {
      dev_err(dev, "request_irq() failed: %pe\n", ERR_PTR(err));
      goto err_request_irq;
    }
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

  for (int i = 0; i < CAFE_INT_VECTOR_CNT; i++) {
    if ((irqn = pci_irq_vector(pdev, i)) >= 0)
      free_irq(irqn, pdev);
  }

  pci_free_irq_vectors(pdev);
}

