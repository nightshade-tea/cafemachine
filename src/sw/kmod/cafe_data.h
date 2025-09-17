#ifndef _CAFE_KMOD_H_
#define _CAFE_KMOD_H_

#include "cafe_kmod.h"

struct cafe_dev_data {
  void __iomem *mmio;
};

struct cafe_dev_data *cafe_dev_data_alloc(void);

void cafe_dev_data_free(struct cafe_dev_data *data);

#endif /* _CAFE_KMOD_H_ */
