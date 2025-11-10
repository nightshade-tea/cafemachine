#include "dump.h"
#include "tea.h"
#include "tea_log.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

static int dump_pages(TeaState *dev, hwaddr first, hwaddr last)
{
    size_t buf_offset = 0;
    int all_failed = 1;

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

static int dump_mem(TeaState *dev, const char basename[TEA_MAX_BASENAME_LEN],
                    hwaddr first, hwaddr last, bool relaxed,
                    uint8_t enable_dump_pages, uint32_t tolerance)
{
    // TEA_DUMP_EXT already has a null terminator
    char filename[TEA_MAX_BASENAME_LEN + sizeof TEA_DUMP_EXT];
    FILE *dumpf;
    uint32_t failed_reads = 0;

    if (!tolerance) {
        tolerance = TEA_DEFAULT_DUMP_TOLERANCE;
    }

    assemble_filename(basename, filename);

    tea_log("dump requested for %s\n", filename);

    /* create dump file */
    if (!(dumpf = fopen(filename, "wb+"))) {
        tea_log("failed to open %s, aborting dump: %s\n", filename,
                strerror(errno));
        return -1;
    }

    while (first <= last && (relaxed || failed_reads < tolerance)) {
        MemTxResult err;
        size_t readsz = MIN(TEA_DMA_BUF_SZ, last - first + 1);

        /* read memory */
        if ((err = pci_dma_read(&dev->pci_dev, first, dev->dma_buf, readsz)) !=
            MEMTX_OK) {
            tea_log("(%" PRIu32 ") failed dma read of %zu bytes from %#" PRIx64
                    " (MemTxResult=%u)\n",
                    ++failed_reads, readsz, first, err);

            if (enable_dump_pages) {
                tea_log("enable_dump_pages is set, retrying dump page by page "
                        "on range "
                        "%#" PRIx64 " -- %#" PRIx64 "\n",
                        first, first + readsz);

                if (!dump_pages(dev, first, first + readsz)) {
                    failed_reads = 0;
                }
            }

            else {
                memset(dev->dma_buf, 0, TEA_DMA_BUF_SZ);
            }
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

void assemble_filename(
    const char basename[TEA_MAX_BASENAME_LEN],
    char filename[TEA_MAX_BASENAME_LEN + sizeof TEA_DUMP_EXT])
{
    size_t basename_len;

    basename_len = strnlen(basename, TEA_MAX_BASENAME_LEN);

    /* if unset, use default value */
    if (basename_len == 0) {
        basename = TEA_DEFAULT_DUMP_BASENAME;
        basename_len = sizeof(TEA_DEFAULT_DUMP_BASENAME) - 1;
        tea_log("dump filename not set, defaulting to %s%s\n", basename,
                TEA_DUMP_EXT);
    }

    memcpy(filename, basename, basename_len);
    memcpy(filename + basename_len, TEA_DUMP_EXT, sizeof TEA_DUMP_EXT);
}

int tea_dump_mem_strict(TeaState *dev,
                        const char basename[TEA_MAX_BASENAME_LEN], hwaddr first,
                        hwaddr last, uint8_t dump_pages, uint32_t tolerance)
{
    if (!last) {
        last = (1LLU << 48) - 1;
    }

    return dump_mem(dev, basename, first, last, false, dump_pages, tolerance);
}

int tea_dump_mem_relaxed(TeaState *dev,
                         const char basename[TEA_MAX_BASENAME_LEN],
                         hwaddr first, hwaddr last, uint8_t dump_pages)
{
    return dump_mem(dev, basename, first, last, true, dump_pages, 0);
}
