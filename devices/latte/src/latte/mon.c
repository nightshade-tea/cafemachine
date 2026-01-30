#include "mon.h"

mem_region_list surveillance_list;

static struct mem_region *create_mem_region(hwaddr start_addr, hwaddr end_addr)
{
    struct mem_region *mr = malloc(sizeof *mr);
    if (!mr) {
        latte_log("failed to allocate mem_region: %s\n", strerror(errno));
        return NULL;
    }

    mr->start_addr = start_addr;
    mr->end_addr = end_addr;

    return mr;
}

int put_under_surveillance(hwaddr start_addr, hwaddr end_addr)
{
    errno = 0;

    struct mem_region *mr = create_mem_region(start_addr, end_addr);
    if (!mr) {
        latte_log("failed to create mem_region\n");
        return errno;
    }

    LIST_INSERT_HEAD(&surveillance_list, mr, regions);

    return 0;
}

static void mock_addr()
{
    put_under_surveillance(0x1000, 0x8000);
    put_under_surveillance(0x20000, 0x80000);
}

static int surveil(LatteState *dev, struct mem_region *mr)
{
    void *buf = malloc(mr->end_addr - mr->start_addr);
    latte_dump_mem_relaxed(dev, buf, mr->start_addr, mr->end_addr, 0);

    return 1;
}

void *mon_thread(void *pci_dev)
{
    LatteState *dev = LATTE_DEVICE(pci_dev);
    LIST_INIT(&surveillance_list); // race condition maybe

    mock_addr();
    latte_log("surveillance thread started\n");

    for (;;) {
        struct mem_region *mr = NULL;

        latte_log("surveillance round starting...\n");

        LIST_FOREACH (mr, &surveillance_list, regions) {
            int corrupted = surveil(dev, mr);
            if (corrupted) {
                // TODO raise nmi
                latte_log("memory region [%#" HWADDR_PRIx " - %#" HWADDR_PRIx
                          "] corrupted!!!\n",
                          mr->start_addr, mr->end_addr);
            } else {
                latte_log("memory region [%#" HWADDR_PRIx " - %#" HWADDR_PRIx
                          "] intact !!!\n",
                          mr->start_addr, mr->end_addr);
            }
        }

        latte_log("surveillance round finished. %ds until next round\n",
                  LATTE_SURVEILLANCE_RATE);
        sleep(LATTE_SURVEILLANCE_RATE);
    }

    return NULL;
}
