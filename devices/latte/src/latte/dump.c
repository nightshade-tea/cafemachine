#include "dump.h"
#include "latte.h"
#include "latte_log.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

static int dump_pages(LatteState *dev, char *buf, hwaddr first, hwaddr last)
{
    size_t buf_offset = 0;
    int all_failed = 1;

    while (first <= last) {
        MemTxResult err;
        size_t readsz = MIN(LATTE_PAGE_SZ, last - first + 1);

        /* read memory */
        if ((err = pci_dma_read(&dev->pci_dev, first, buf + buf_offset,
                                readsz)) != MEMTX_OK) {
            memset(buf + buf_offset, 0, readsz);
        }

        else {
            all_failed = 0;
        }

        first += readsz;
        buf_offset += readsz;
    }
    return all_failed;
}

static int dump_mem(LatteState *dev, char *buf, hwaddr first, hwaddr last,
                    bool relaxed, uint8_t enable_dump_pages, uint32_t tolerance)
{

    uint32_t failed_reads = 0;

    if (!tolerance) {
        tolerance = LATTE_DEFAULT_DUMP_TOLERANCE;
    }

    hwaddr offset = 0;
    while (first <= last && (relaxed || failed_reads < tolerance)) {
        MemTxResult err;
        size_t readsz = MIN(LATTE_DMA_READ_SZ, last - first + 1);

        /* read memory */
        if ((err = pci_dma_read(&dev->pci_dev, first, buf + offset, readsz)) !=
            MEMTX_OK) {
            latte_log("(%" PRIu32
                      ") failed dma read of %zu bytes from %#" PRIx64
                      " (MemTxResult=%u)\n",
                      ++failed_reads, readsz, first, err);

            if (enable_dump_pages) {
                latte_log("enable_dump_pages is set, retrying dump page by "
                          "page on range "
                          "%#" PRIx64 " -- %#" PRIx64 "\n",
                          first, first + readsz);

                if (!dump_pages(dev, buf + offset, first, first + readsz)) {
                    failed_reads = 0;
                }
            }

        }

        else {
            if (failed_reads) {
                latte_log("successful read, resetting failed_reads counter\n");
            }

            failed_reads = 0;
        }

        offset += readsz;
        first += readsz;
    }

    latte_log("finished memory dump\n");
    return 0;
}

int latte_dump_mem_strict(LatteState *dev, char *buf, hwaddr first, hwaddr last,
                          uint8_t dump_pages, uint32_t tolerance)
{
    if (!last) {
        last = (1LLU << 48) - 1;
    }

    return dump_mem(dev, buf, first, last, false, dump_pages, tolerance);
}

int latte_dump_mem_relaxed(LatteState *dev, char *buf, hwaddr first,
                           hwaddr last, uint8_t dump_pages)
{
    return dump_mem(dev, buf, first, last, true, dump_pages, 0);
}
