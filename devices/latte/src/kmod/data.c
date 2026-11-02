#include "data.h"
#include "latte.h"

struct latte_dev_data *latte_dev_data_alloc(void)
{
	struct latte_dev_data *data;

	if (!(data = kzalloc(sizeof(struct latte_dev_data), GFP_KERNEL)))
		return NULL;

	for (int i = 0; i < LATTE_WAIT_CNT; i++)
		init_completion(&data->devop_done[i]);

	return data;
}

void latte_dev_data_free(struct latte_dev_data *data)
{
	kfree(data);
}
