#include "mmio.h"
#include "data.h"
#include "kmod.h"
#include "macchiato.h"

int macchiato_mmio_enable(struct pci_dev *pdev)
{
	struct macchiato_dev_data *data;
	struct device *dev;
	int err, id;

	dev = &pdev->dev;
	data = pci_get_drvdata(pdev);

	for (id = 0; id < MACCHIATO_BAR_ID_CNT; id++) {

		/* the device driver needs to call pci_request_region() to
		 * verify no other device is already using the same address
		 * resource. */

		/* mark the pci region associated with pci device pdev bar as
		 * being reserved by owner res_name. do not access any address
		 * inside the pci regions unless this call returns successfully.
		 */

		if ((err = pci_request_region(pdev, id, MACCHIATO_HW_NAME))) {
			dev_err(dev, "pci_request_region(id=%d) failed: %pe\n",
				id, ERR_PTR(err));
			goto err_pci_request_region;
		}

		/* using this function you will get a __iomem virtual address to
		 * your device bar. you can access it using ioread*() and
		 * iowrite*(). these functions hide the details if this is a
		 * mmio or pio address space and will just do what you expect
		 * from them in the correct way. */

		if (!(data->bar[id].mmio = pci_iomap(
			      pdev, id, 0 /* zero maps the whole bar */))) {
			dev_err(dev, "pci_iomap(id=%d) returned null\n", id);
			err = -ENOMEM;
			goto err_pci_iomap;
		}

		data->bar[id].start = pci_resource_start(pdev, id);
		data->bar[id].end = pci_resource_end(pdev, id);
		data->bar[id].len = pci_resource_len(pdev, id);
	}

	return 0;

err_pci_iomap:
	pci_release_region(pdev, id);

err_pci_request_region:

	while (id--) {
		pci_iounmap(pdev, data->bar[id].mmio);
		pci_release_region(pdev, id);
	}

	return err;
}

void macchiato_mmio_disable(struct pci_dev *pdev)
{
	struct macchiato_dev_data *data;
	int id;

	data = pci_get_drvdata(pdev);
	id = MACCHIATO_BAR_ID_CNT;

	while (id--) {
		pci_iounmap(pdev, data->bar[id].mmio);
		pci_release_region(pdev, id);
	}
}
