#include "dump.h"
#include "tea.h"
#include "tea_log.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

static int tea_dump_mem(TeaState *dev,
                        const char basename[TEA_MAX_BASENAME_LEN], hwaddr first,
                        hwaddr last, bool relaxed)
{
    // TEA_DUMP_EXT already has a null terminator
    char filename[TEA_MAX_BASENAME_LEN + sizeof(TEA_DUMP_EXT)];
    size_t basename_len;
    FILE *dumpf;
    uint64_t failed_reads = 0;

    basename_len = strnlen(basename, TEA_MAX_BASENAME_LEN);

    /* if unset, use default value */
    if (basename_len == 0) {
        basename = TEA_DEFAULT_DUMP_BASENAME;
        basename_len = sizeof(TEA_DEFAULT_DUMP_BASENAME) - 1;
        tea_log("dump filename not set, defaulting to %s%s\n", basename,
                TEA_DUMP_EXT);
    }

    /* assemble filename */
    memcpy(filename, basename, basename_len);
    memcpy(filename + basename_len, TEA_DUMP_EXT, sizeof(TEA_DUMP_EXT));

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

            memset(dev->dma_buf, 0, TEA_DMA_BUF_SZ);
        }

        else {
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

    tea_log("finished memory dump\n");
    fclose(dumpf);
    return 0;
}

int tea_dump_mem_strict(TeaState *dev,
                        const char basename[TEA_MAX_BASENAME_LEN], hwaddr first)
{
    return tea_dump_mem(dev, basename, first, (1LLU << 48) - 1, false);
}

int tea_dump_mem_relaxed(TeaState *dev,
                         const char basename[TEA_MAX_BASENAME_LEN],
                         hwaddr first, hwaddr last)
{
    return tea_dump_mem(dev, basename, first, last, true);
}
