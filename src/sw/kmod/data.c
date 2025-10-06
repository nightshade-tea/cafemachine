#include "data.h"

struct cafe_dev_data *cafe_dev_data_alloc(void) {
    struct cafe_dev_data *data;

    if (!(data = kzalloc(sizeof (struct cafe_dev_data), GFP_KERNEL)))
        return NULL;

    for (int i = 0; i < CAFE_MUTEX_CNT; i++)
        mutex_init(&data->mutex[i]);

    return data;
}

void cafe_dev_data_free(struct cafe_dev_data *data) {
    kfree(data);
}
