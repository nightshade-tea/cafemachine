#include "cafe.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char **argv) {
  uint64_t *mem;
  int fd;
  int count;

  if ((fd = open("/dev/cafe", O_RDWR)) < 0) {
    perror("failed to open /dev/cafe");
    return 1;
  }

  if ((mem = mmap(NULL, CAFE_MMIO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                  0)) == MAP_FAILED) {
    perror("mmap failed");
    return 1;
  }

  if (argc < 2)
    count = 1;
  else
    count = atoi(argv[1]);

  if (count < 1 || count > CAFE_MMIO_SIZE / CAFE_REG_SZ)
    count = CAFE_MMIO_SIZE / CAFE_REG_SZ;

  for (int i = 0; i < count; i++)
    mem[i] = 0xcafecafe;

  return 0;
}
