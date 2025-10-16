#include "dump.h"
#include "cafe.h"
#include "cafe_log.h"
#include <stdio.h>
#include <string.h>

int cafe_dump_buf(CafeState *dev) {
  char filename[CAFE_REG_SZ + sizeof(CAFE_DMA_DUMP_EXT) + 1];
  FILE *f;

  /* assemble filename */
  memcpy(filename, (char *)&dev->reg[CAFE_DUMP_FILE], CAFE_REG_SZ);
  memcpy(filename + CAFE_REG_SZ, CAFE_DMA_DUMP_EXT, sizeof(CAFE_DMA_DUMP_EXT));
  filename[CAFE_REG_SZ + sizeof(CAFE_DMA_DUMP_EXT)] = '\0';

  cafe_log("dump requested for %s\n", filename);

  if (!(f = fopen(filename, "ab+"))) {
    cafe_log("failed to open %s, aborting dump\n", filename);
    return -1;
  }

  if (fwrite(dev->dma_buf, dev->reg[CAFE_DMA_SZ], 1, f) != 1) {
    cafe_log("failed to write to %s, aborting dump\n", filename);
    fclose(f);
    return -1;
  }

  fclose(f);
  return 0;
}
