#include <string.h>
#include <stdio.h>
#include "dump.h"
#include "cafe.h"
#include "cafe_log.h"

int cafe_dump_buf(CafeState *dev, void *buf) {
  char filename[CAFE_REG_SZ + sizeof(CAFE_DMA_DUMP_EXT) + 1];
  FILE *f;

  /* assemble filename */
  memcpy(filename, (char *)&dev->r[CAFE_DUMP_FILENAME], CAFE_REG_SZ);
  memcpy(filename + CAFE_REG_SZ, CAFE_DMA_DUMP_EXT,
      sizeof(CAFE_DMA_DUMP_EXT));
  filename[CAFE_REG_SZ + sizeof(CAFE_DMA_DUMP_EXT)] = '\0';

  if (!(f = fopen(filename, "ab+"))) {
    cafe_log("failed to open %s, aborting dump\n", filename);
    return -1;
  }

  if (fwrite(buf, dev->r[CAFE_DMA_SZ], 1, f) < dev->r[CAFE_DMA_SZ]) {
    cafe_log("failed to write to %s, aborting dump\n", filename);
    fclose(f);
    return -1;
  }

  fclose(f);
  return 0;
}
