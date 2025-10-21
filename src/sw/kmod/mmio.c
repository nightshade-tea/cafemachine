#include "mmio.h"
#include "cafe.h"
#include "data.h"
#include "kmod.h"

int cafe_mmio_enable(struct pci_dev *pdev) {
  struct cafe_dev_data *data;
  struct device *dev;
  int err;

  dev = &pdev->dev;
  data = pci_get_drvdata(pdev);

  /* the device driver needs to call pci_request_region() to verify no other
   * device is already using the same address resource. */

  /* mark the pci region associated with pci device pdev bar as being
   * reserved by owner res_name. do not access any address inside the pci
   * regions unless this call returns successfully. */

  if ((err = pci_request_region(pdev, CAFE_MMIO_BAR_NUM, CAFE_HW_NAME))) {
    dev_err(dev, "pci_request_region() failed: %pe\n", ERR_PTR(err));
    goto err_pci_request_region;
  }

  /* using this function you will get a __iomem virtual address to your device
   * bar. you can access it using ioread*() and iowrite*(). these functions
   * hide the details if this is a mmio or pio address space and will just do
   * what you expect from them in the correct way. */

  if (!(data->bar.mmio = pci_iomap(pdev, CAFE_MMIO_BAR_NUM, CAFE_MMIO_SIZE))) {
    dev_err(dev, "pci_iomap() returned NULL\n");
    err = -ENOMEM;
    goto err_pci_iomap;
  }

  data->bar.start = pci_resource_start(pdev, CAFE_MMIO_BAR_NUM);
  data->bar.end = pci_resource_end(pdev, CAFE_MMIO_BAR_NUM);
  data->bar.len = pci_resource_len(pdev, CAFE_MMIO_BAR_NUM);

  return 0;

err_pci_iomap:
  pci_release_region(pdev, CAFE_MMIO_BAR_NUM);

err_pci_request_region:

  return err;
}

void cafe_mmio_disable(struct pci_dev *pdev) {
  struct cafe_dev_data *data;

  data = pci_get_drvdata(pdev);

  pci_iounmap(pdev, data->bar.mmio);
  pci_release_region(pdev, CAFE_MMIO_BAR_NUM);
}
