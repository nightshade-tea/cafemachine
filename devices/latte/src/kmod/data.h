#ifndef _LATTE_DATA_H_
#define _LATTE_DATA_H_

#include "latte.h"
#include <linux/slab.h>

struct latte_bar {
	u64 start;	    /* base physical address mapped to device memory */
	u64 end;	    /* last physical address ... */
	u64 len;	    /* size of bar in bytes */
	void __iomem *mmio; /* base virtual address */
};

/* msi! */
struct latte_irq {
	int base;      /* first virtual irq number */
	int allocated; /* number of vectors allocated */
};

/* struct for device instance specific data */
struct latte_dev_data {
	struct latte_bar bar;
	struct latte_irq irq;
	struct completion devop_done[LATTE_WAIT_CNT]; /* sync operations */
};

struct latte_dev_data *latte_dev_data_alloc(void);

void latte_dev_data_free(struct latte_dev_data *data);

#endif /* _LATTE_DATA_H_ */
