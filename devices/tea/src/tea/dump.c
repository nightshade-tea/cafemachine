#include "dump.h"
#include "tea.h"
#include "tea_log.h"
#include "utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

#if TEA_DUMP_PAGES_ON_ERR
static int dump_pages(TeaState *dev, hwaddr first, hwaddr last)
{
    size_t buf_offset = 0;
    int all_failed = 1;

    tea_log("TEA_DUMP_PAGES_ON_ERR is set, retrying dump page by page on range "
            "%#" PRIx64 " -- %#" PRIx64 "\n",
            first, last);

    while (first <= last) {
        MemTxResult err;
        size_t readsz = MIN(TEA_PAGE_SZ, last - first + 1);

        /* read memory */
        if ((err = pci_dma_read(&dev->pci_dev, first, dev->dma_buf + buf_offset,
                                readsz)) != MEMTX_OK) {
            memset(dev->dma_buf + buf_offset, 0, readsz);
        }

        else {
            all_failed = 0;
        }

        first += readsz;
        buf_offset += readsz;
    }

    return all_failed;
}
#endif

static int dump_mem(TeaState *dev, const char basename[TEA_MAX_BASENAME_LEN],
                    hwaddr first, hwaddr last, bool relaxed)
{
    // TEA_DUMP_EXT already has a null terminator
    char filename[TEA_MAX_BASENAME_LEN + sizeof(TEA_DUMP_EXT)];
    FILE *dumpf;
    uint64_t failed_reads = 0;

    mount_filename(basename, filename);

    tea_log("dump requested for %s\n", filename);

    /* create dump file */
    if (!(dumpf = fopen(filename, "wb+"))) {
        tea_log("failed to open %s, aborting dump: %s\n", filename,
                strerror(errno));
        return -1;
    }

    while (first <= last && (relaxed || failed_reads < TEA_DUMP_TOLERANCE)) {
        MemTxResult err;
        size_t readsz = MIN(TEA_DMA_BUF_SZ, last - first + 1);

        /* read memory */
        if ((err = pci_dma_read(&dev->pci_dev, first, dev->dma_buf, readsz)) !=
            MEMTX_OK) {
            tea_log("(%" PRIu64 ") failed dma read of %zu bytes from %#" PRIx64
                    " (MemTxResult=%u)\n",
                    ++failed_reads, readsz, first, err);

#if TEA_DUMP_PAGES_ON_ERR
            if (!dump_pages(dev, first, first + readsz)) {
                failed_reads = 0;
            }
#else
            memset(dev->dma_buf, 0, TEA_DMA_BUF_SZ);
#endif
        }

        else {
            if (failed_reads) {
                tea_log("successful read, resetting failed_reads counter\n");
            }

            failed_reads = 0;
        }

        /* write to dump file */
        if (fwrite(dev->dma_buf, readsz, 1, dumpf) != 1) {
            tea_log("failed to write to %s, aborting dump: %s\n", filename,
                    strerror(errno));
            fclose(dumpf);
            return -1;
        }

        first += readsz;
    }

    fclose(dumpf);
    tea_log("finished memory dump\n");
    return 0;
}

int tea_dump_mem_strict(TeaState *dev,
                        const char basename[TEA_MAX_BASENAME_LEN], hwaddr first)
{
    return dump_mem(dev, basename, first, (1LLU << 48) - 1, false);
}

int tea_dump_mem_relaxed(TeaState *dev,
                         const char basename[TEA_MAX_BASENAME_LEN],
                         hwaddr first, hwaddr last)
{
    return dump_mem(dev, basename, first, last, true);
}
