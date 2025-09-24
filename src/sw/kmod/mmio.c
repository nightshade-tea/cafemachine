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

    if (!(data = pci_get_drvdata(pdev))) {
        dev_err (dev, "pci_get_drvdata() returned NULL!");
        return -ENOMEM;
    }

    /* The device driver needs to call pci_request_region() to verify no other
     * device is already using the same address resource. */

    /* Mark the PCI region associated with PCI device pdev BAR bar as being
     * reserved by owner res_name. Do not access any address inside the PCI
     * regions unless this call returns successfully. */

    if ((err = pci_request_region(pdev, CAFE_MMIO_BAR_NUM, CAFE_HW_NAME))) {
        dev_err (dev, "pci_request_region() failed: %d\n", err);
        return err;
    }

    /* Using this function you will get a __iomem address to your device BAR.
     * You can access it using ioread*() and iowrite*(). These functions hide
     * the details if this is a MMIO or PIO address space and will just do what
     * you expect from them in the correct way. */

    if (!(data->mmio = pci_iomap(pdev, CAFE_MMIO_BAR_NUM, CAFE_MMIO_SIZE))) {
        dev_err (dev, "pci_iomap() failed!");
        return -ENOMEM;
    }

    /* test mmio */
    dev_info (dev, "mmio test: writing %llx\n", MMIO_TEST_VAL);
    writeq (MMIO_TEST_VAL, data->mmio);
    mmio_test = readq(data->mmio);
    dev_info (dev, "mmio test: got %llx\n", mmio_test);

    if (mmio_test != MMIO_TEST_VAL)
        dev_warn (dev, "mmio test failed!\n");

    return 0;
}

void cafe_mmio_deinit(struct pci_dev *pdev) {
    struct cafe_dev_data *data;

    if ((data = pci_get_drvdata(pdev)) && data->mmio)
        pci_iounmap(pdev, data->mmio);

    pci_release_region(pdev, CAFE_MMIO_BAR_NUM);
}
