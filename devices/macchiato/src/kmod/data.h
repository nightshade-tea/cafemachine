#ifndef _MACCHIATO_DATA_H_
#define _MACCHIATO_DATA_H_

#include "macchiato.h"
#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/types.h>

struct macchiato_bar {
	unsigned long start; /* base physical address mapped to device memory */
	unsigned long end;   /* last physical address ... */
	unsigned long len;   /* size of bar in bytes */
	void __iomem *mmio;  /* base virtual address */
};

enum macchiato_lock_flag_bit {
	MACCHIATO_LOCK_FD,

	MACCHIATO_LOCK_CNT
};

/* struct for device instance specific data */
struct macchiato_dev_data {
	struct macchiato_bar bar[MACCHIATO_BAR_ID_CNT];
	struct completion sync;
	struct cdev cdev;
	dev_t devt;
	DECLARE_BITMAP(lock, MACCHIATO_LOCK_CNT);
};

struct macchiato_dev_data *macchiato_dev_data_alloc(void);

void macchiato_dev_data_free(struct macchiato_dev_data *data);

#endif /* _MACCHIATO_DATA_H_ */
