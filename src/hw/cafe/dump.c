#include "dump.h"
#include "cafe.h"
#include "cafe_log.h"
#include <stdio.h>
#include <string.h>

int cafe_dump_mem(CafeState *dev) {
  // CAFE_DMA_DUMP_EXT already has a null terminator
  char filename[CAFE_REG_SZ + sizeof(CAFE_DMA_DUMP_EXT)];
  const char *basename;
  size_t basename_len;
  FILE *dumpf;
  hwaddr addr, last_addr;

  /* get CAFE_DUMP_FILE str */
  basename = (const char *)&dev->reg[CAFE_DUMP_FILE];
  basename_len = strnlen(basename, CAFE_REG_SZ);

  /* if unset, use default value */
  if (basename_len == 0) {
    basename = CAFE_DMA_DEFAULT_DUMP_FILE;
    basename_len = strnlen(basename, CAFE_REG_SZ);
    cafe_log("dump filename not set, defaulting to %s%s\n", basename,
             CAFE_DMA_DUMP_EXT);
  }

  /* assemble filename */
  memcpy(filename, basename, basename_len);
  memcpy(filename + basename_len, CAFE_DMA_DUMP_EXT, sizeof(CAFE_DMA_DUMP_EXT));

  cafe_log("dump requested for %s\n", filename);

  /* create dump file */
  if (!(dumpf = fopen(filename, "wb+"))) {
    cafe_log("failed to open %s, aborting dump\n", filename);
    return -1;
  }

  addr = dev->reg[CAFE_DUMP_FIRST_ADDR];
  last_addr = dev->reg[CAFE_DUMP_LAST_ADDR];

  while (addr <= last_addr) {
    MemTxResult err;
    size_t readsz = MIN(CAFE_DMA_BUF_SZ, last_addr - addr + 1);

    /* read memory */
    if ((err = pci_dma_read(&dev->pci_dev, addr, dev->dma_buf, readsz)) !=
        MEMTX_OK) {
      cafe_log("failed dma read of %zu bytes from %#" PRIx64 "\n", readsz,
               addr);
      memset(dev->dma_buf, 0, CAFE_DMA_BUF_SZ);
    }

    /* write to dump file */
    if (fwrite(dev->dma_buf, readsz, 1, dumpf) != 1) {
      cafe_log("failed to write to %s, aborting dump\n", filename);
      fclose(dumpf);
      return -1;
    }

    addr += readsz;
  }

  fclose(dumpf);
  return 0;
}
