#include "data.h"
#include "macchiato.h"

struct macchiato_dev_data *macchiato_dev_data_alloc(void)
{
	struct macchiato_dev_data *data;

	if (!(data = kzalloc(sizeof data, GFP_KERNEL)))
		return NULL;

	init_completion(&data->sync);

	return data;
}

void macchiato_dev_data_free(struct macchiato_dev_data *data)
{
	kfree(data);
}
