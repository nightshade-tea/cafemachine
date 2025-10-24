#include "dump.h"
#include "tea.h"
#include "tea_log.h"
#include <stdio.h>
#include <string.h>

int tea_dump_mem(TeaState *dev, const char basename[TEA_MAX_BASENAME_LEN],
                 hwaddr first, hwaddr last)
{
    // TEA_DUMP_EXT already has a null terminator
    char filename[TEA_MAX_BASENAME_LEN + sizeof(TEA_DUMP_EXT)];
    size_t basename_len;
    FILE *dumpf;

    basename_len = strnlen(basename, TEA_MAX_BASENAME_LEN);

    /* if unset, use default value */
    if (basename_len == 0) {
        basename = TEA_DEFAULT_DUMP_BASENAME;
        basename_len = sizeof(basename) - 1;
        tea_log("dump filename not set, defaulting to %s%s\n", basename,
                TEA_DUMP_EXT);
    }

    /* assemble filename */
    memcpy(filename, basename, basename_len);
    memcpy(filename + basename_len, TEA_DUMP_EXT, sizeof(TEA_DUMP_EXT));

    tea_log("dump requested for %s\n", filename);

    /* create dump file */
    if (!(dumpf = fopen(filename, "wb+"))) {
        tea_log("failed to open %s, aborting dump\n", filename);
        return -1;
    }

    while (first <= last) {
        MemTxResult err;
        size_t readsz = MIN(TEA_DMA_BUF_SZ, last - first + 1);

        /* read memory */
        if ((err = pci_dma_read(&dev->pci_dev, first, dev->dma_buf, readsz)) !=
            MEMTX_OK) {
            tea_log("failed dma read of %zu bytes from %#" PRIx64 "\n", readsz,
                    first);
            memset(dev->dma_buf, 0, TEA_DMA_BUF_SZ);
        }

        /* write to dump file */
        if (fwrite(dev->dma_buf, readsz, 1, dumpf) != 1) {
            tea_log("failed to write to %s, aborting dump\n", filename);
            fclose(dumpf);
            return -1;
        }

        first += readsz;
    }

    fclose(dumpf);
    return 0;
}
