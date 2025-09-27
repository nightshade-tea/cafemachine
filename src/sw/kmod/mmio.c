#include "mmio.h"
#include "kmod.h"
#include "data.h"
#include "hw/cafe.h"

#define MMIO_TEST_VAL 0x1cedcafeLLU

int cafe_mmio_init(struct pci_dev *pdev) {
    struct cafe_dev_data *data;
    struct device *dev;
    uint64_t mmio_test;
    int err;

    dev = &pdev->dev;
    data = pci_get_drvdata(pdev);

    /* The device driver needs to call pci_request_region() to verify no other
     * device is already using the same address resource. */

    /* Mark the PCI region associated with PCI device pdev BAR bar as being
     * reserved by owner res_name. Do not access any address inside the PCI
     * regions unless this call returns successfully. */

    if ((err = pci_request_region(pdev, CAFE_MMIO_BAR_NUM, CAFE_HW_NAME))) {
        dev_err (dev, "pci_request_region() failed: %pe\n", ERR_PTR(err));
        goto err_pci_request_region;
    }

    /* Using this function you will get a __iomem address to your device BAR.
     * You can access it using ioread*() and iowrite*(). These functions hide
     * the details if this is a MMIO or PIO address space and will just do what
     * you expect from them in the correct way. */

    if (!(data->bar.mmio = pci_iomap(pdev, CAFE_MMIO_BAR_NUM, CAFE_MMIO_SIZE)))
    {
        dev_err (dev, "pci_iomap() returned NULL\n");
        err = -ENOMEM;
        goto err_pci_iomap;
    }

    data->bar.start = pci_resource_start(pdev, CAFE_MMIO_BAR_NUM);
    data->bar.end = pci_resource_end(pdev, CAFE_MMIO_BAR_NUM);
    data->bar.len = pci_resource_len(pdev, CAFE_MMIO_BAR_NUM);

    /* test mmio */
    dev_info (dev, "mmio test: writing %llx\n", MMIO_TEST_VAL);
    writeq (MMIO_TEST_VAL, data->bar.mmio);
    mmio_test = readq(data->bar.mmio);
    dev_info (dev, "mmio test: got %llx\n", mmio_test);

    if (mmio_test != MMIO_TEST_VAL)
        dev_warn (dev, "mmio test failed!\n");

    return 0;

err_pci_iomap:
    pci_release_region(pdev, CAFE_MMIO_BAR_NUM);

err_pci_request_region:

    return err;
}

void cafe_mmio_deinit(struct pci_dev *pdev) {
    struct cafe_dev_data *data;

    data = pci_get_drvdata(pdev);

    pci_iounmap(pdev, data->bar.mmio);
    pci_release_region(pdev, CAFE_MMIO_BAR_NUM);
}
