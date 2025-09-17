#ifndef _CAFE_DATA_H_
#define _CAFE_DATA_H_

#include <linux/slab.h>

struct cafe_dev_data {
  void __iomem *mmio;
};

struct cafe_dev_data *cafe_dev_data_alloc(void);

void cafe_dev_data_free(struct cafe_dev_data *data);

#endif /* _CAFE_DATA_H_ */
