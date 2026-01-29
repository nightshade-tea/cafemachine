#ifndef MON_H
#define MON_H

#include <errno.h>
#include <string.h>
#include <sys/queue.h>
#include <unistd.h>

#include "latte.h"
#include "latte_device.h"
#include "latte_log.h"

#include "exec/hwaddr.h"

struct mem_region {
    hwaddr start_addr;
    hwaddr end_addr;
    LIST_ENTRY(mem_region) regions;
};

typedef LIST_HEAD(mem_region_list, mem_region) mem_region_list;

int put_under_surveillance(hwaddr start_addr, hwaddr end_addr);

void *mon_thread(void *pci_dev);

#endif
