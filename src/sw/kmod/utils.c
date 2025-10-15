#include "utils.h"

static int get_max(struct resource *res, void *arg) {
  phys_addr_t *max = arg;
  if (res->end > *max)
    *max = res->end;

  return 0;
}

phys_addr_t cafe_find_max_ram_addr() {
  unsigned long flags;
  phys_addr_t max;

  flags = IORESOURCE_SYSTEM_RAM | IORESOURCE_BUSY;
  max = 0;

  walk_iomem_res_desc(IORES_DESC_NONE, flags, 0, -1, &max, get_max);

  return max;
}
