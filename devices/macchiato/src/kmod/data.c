#include "data.h"
#include "latte.h"

struct latte_dev_data *latte_dev_data_alloc(void)
{
	struct latte_dev_data *data;

	if (!(data = kzalloc(sizeof data, GFP_KERNEL)))
		return NULL;

	init_completion(&data->sync);

	return data;
}

void latte_dev_data_free(struct latte_dev_data *data)
{
	kfree(data);
}
