#include "cafe_data.h"

struct cafe_dev_data *cafe_dev_data_alloc(void) {
    return kzalloc(sizeof (struct cafe_dev_data), GFP_KERNEL);
}

void cafe_dev_data_free(struct cafe_dev_data *data) {
    kfree(data);
}
