#include "cafe.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

int main() {
  uint64_t *mem;
  int fd;

  if ((fd = open("/dev/cafe", O_RDWR)) < 0) {
    perror("failed to open /dev/cafe");
    return 1;
  }

  if ((mem = mmap(NULL, CAFE_MMIO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                  0)) == MAP_FAILED) {
    perror("mmap failed");
    return 1;
  }

  mem[CAFE_DMA_SRC] = 0;
  mem[CAFE_DMA_SZ] = 512;
  mem[CAFE_CMD] = CAFE_DMA_READ;

  return 0;
}
