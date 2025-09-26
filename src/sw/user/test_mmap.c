#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>

int main() {
  int fd;
  uint64_t *mem;

  if ((fd = open("/dev/cafe", O_RDWR)) < 0) {
    fprintf(stderr, "open falhou\n");
    return 1;
  }

  mem = mmap(NULL, 8, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

  if (mem == MAP_FAILED) {
    fprintf(stderr, "mmap falhou\n");
    return 1;
  }

  for (int i = 0; i < (4096 / 8) ; i++) {
    mem[i] = 0xcafecafe;

    if (mem[i] != 0xcafecafe)
      fprintf(stderr, "got %x\n", mem[i]);
  }

  return 0;
}
