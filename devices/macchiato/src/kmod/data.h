#ifndef _MACCHIATO_DATA_H_
#define _MACCHIATO_DATA_H_

#include "macchiato.h"
#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/types.h>

enum macchiato_lock_flag_bit {
	MACCHIATO_LOCK_FD, /* open(), release()  */

	MACCHIATO_LOCK_CNT
};

struct macchiato_bar {
	unsigned long start; /* base physical address mapped to device memory */
	unsigned long end;   /* last physical address ... */
	unsigned long len;   /* size of bar in bytes */
	void __iomem *mmio;  /* base virtual address */
};

struct macchiato_dmabuf {
	unsigned char *kvaddr;
	dma_addr_t paddr;
	size_t size;
};

/* struct for device instance specific data */
struct macchiato_dev_data {
	struct pci_dev *pdev;

	struct macchiato_bar bar[MACCHIATO_BAR_ID_CNT];
	struct macchiato_dmabuf dmabuf;
	int last_hwirq; /* ugly  */

	struct completion sync;
	struct cdev cdev;
	dev_t devt;
	DECLARE_BITMAP(lock, MACCHIATO_LOCK_CNT);
};

int macchiato_dev_data_alloc(struct pci_dev *pdev);

void macchiato_dev_data_free(struct pci_dev *pdev);

#endif /* _MACCHIATO_DATA_H_ */
