#include "mon.h"

mem_region_list surveillance_list;

static struct mem_region *create_mem_region(hwaddr start_addr, hwaddr end_addr)
{
    struct mem_region *mr = malloc(sizeof *mr);
    if (!mr) {
        latte_log("failed to allocate mem_region: %s", strerror(errno));
        return NULL;
    }

    mr->start_addr = start_addr;
    mr->end_addr = end_addr;

    return mr;
}

static void init_surveillance_list(void)
{
    LIST_INIT(&surveillance_list);
}

int put_under_surveillance(hwaddr start_addr, hwaddr end_addr)
{
    errno = 0;

    struct mem_region *mr = create_mem_region(start_addr, end_addr);
    if (!mr) {
        latte_log("failed to create mem_region");
        return errno;
    }

    LIST_INSERT_HEAD(&surveillance_list, mr, regions);

    return 0;
}

static int surveil(struct mem_region *mr)
{
    return 1;
}

void *mon_thread(void *pci_dev)
{
    for (;;) {
        struct mem_region *mr = NULL;

        LIST_FOREACH (mr, &surveillance_list, regions) {
            int corrupted = surveil(mr);
            if (corrupted) {
                // TODO raise nmi
                latte_log("memory region [%#" HWADDR_PRIx " - %#" HWADDR_PRIx
                          "] corrupted!!!",
                          mr->start_addr, mr->end_addr);
            } else {
                latte_log("memory region [%#" HWADDR_PRIx " - %#" HWADDR_PRIx
                          "] intact !!!",
                          mr->start_addr, mr->end_addr);
            }
        }

        sleep(LATTE_SURVEILLANCE_RATE);
    }

    return NULL;
}
