#ifndef _CAFE_DATA_H_
#define _CAFE_DATA_H_

#include <linux/slab.h>
#include "cafe.h"

struct cafe_bar {
  u64 start;
  u64 end;
  u64 len;
  void __iomem *mmio;
};

struct cafe_dev_data {
  struct cafe_bar bar;
  struct device *chrdev;
  struct completion devop_done[CAFE_WAIT_CNT];
  dev_t dev_num;
};

struct cafe_dev_data *cafe_dev_data_alloc(void);

void cafe_dev_data_free(struct cafe_dev_data *data);

#endif /* _CAFE_DATA_H_ */
