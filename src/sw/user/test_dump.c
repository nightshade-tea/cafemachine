#include "cafe.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

int main() {
  int fd;
  char dumpfile[] = "cafdump";
  unsigned long arg = *((unsigned long *)dumpfile);
  long err;

  if ((fd = open("/dev/cafe", O_RDWR)) < 0) {
    perror("failed to open /dev/cafe\n");
    return 1;
  }

  if ((err = ioctl(fd, CAFE_IOCTL_DUMP_MEM, arg))) {
    fprintf(stderr, "ioctl failed! (%ld)\n", err);
    return 2;
  }

  return 0;
}
