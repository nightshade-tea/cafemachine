#ifndef _CAFE_DATA_H_
#define _CAFE_DATA_H_

#include "cafe.h"
#include <linux/slab.h>

struct cafe_bar {
  u64 start;          /* base physical address mapped to device memory */
  u64 end;            /* last physical address ... */
  u64 len;            /* size of bar in bytes */
  void __iomem *mmio; /* base virtual address */
};

/* struct for device instance specific data */
struct cafe_dev_data {
  struct cafe_bar bar;
  struct device *chrdev;                       /* /dev/cafe */
  struct completion devop_done[CAFE_WAIT_CNT]; /* sync operations */
  dev_t dev_num;                               /* major and minor */
};

struct cafe_dev_data *cafe_dev_data_alloc(void);

void cafe_dev_data_free(struct cafe_dev_data *data);

#endif /* _CAFE_DATA_H_ */
