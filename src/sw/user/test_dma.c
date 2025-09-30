#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include "hw/cafe.h"

int main() {
  uint64_t *mem;
  int fd;

  if ((fd = open("/dev/cafe", O_RDWR)) < 0) {
    perror("failed to open /dev/cafe\n");
    return 1;
  }

  if ((mem = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) ==
      MAP_FAILED) {
    perror("mmap failed\n");
    return 1;
  }

  mem[CAFE_DMA_SRC] = 0LL;
  mem[CAFE_DMA_SZ] = 512LL;
  mem[CAFE_CMD] = 0LL;

  return 0;
}
