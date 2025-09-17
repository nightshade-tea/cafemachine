#include "mmio.h"
#include "kmod.h"
#include "data.h"
#include "hw/cafe.h"

#define MMIO_TEST_VAL 0x1cedcafeLLU

int cafe_mmio_init(struct pci_dev *dev) {
    int err;

    /* Mark the PCI region associated with PCI device pdev BAR bar as being
     * reserved by owner res_name. Do not access any address inside the PCI
     * regions unless this call returns successfully. */

    if ((err = pci_request_region(dev, CAFE_MMIO_BAR_NUM, CAFE_KMOD_NAME))) {
        dev_err (&dev->dev, "pci_request_region() failed: %d\n", err);
        return err;
    }

    /* Using this function you will get a __iomem address to your device BAR.
     * You can access it using ioread*() and iowrite*(). These functions hide
     * the details if this is a MMIO or PIO address space and will just do what
     * you expect from them in the correct way. */

    if (!(((struct cafe_dev_data *)dev->dev.driver_data)->mmio = pci_iomap(dev,
                    CAFE_MMIO_BAR_NUM, CAFE_MMIO_SIZE))) {
        dev_err (&dev->dev, "pci_iomap() failed!");
        return -ENOMEM;
    }

    dev_info (&dev->dev, "mmio test: writing %llx\n", MMIO_TEST_VAL);
    writeq (MMIO_TEST_VAL, ((struct cafe_dev_data
                    *)dev->dev.driver_data)->mmio);
    dev_info (&dev->dev, "valor lido %llx\n", readq(((struct cafe_dev_data
                        *)dev->dev.driver_data)->mmio));

    return 0;
}

void cafe_mmio_deinit(struct pci_dev *dev) {
    pci_iounmap(dev, ((struct cafe_dev_data *)dev->dev.driver_data)->mmio);
    pci_release_region(dev, CAFE_MMIO_BAR_NUM);
}
