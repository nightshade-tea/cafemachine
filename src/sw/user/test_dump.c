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

  if ((fd = open("/dev/cafe", O_RDWR)) < 0) {
    perror("failed to open /dev/cafe\n");
    return 1;
  }

  if (!(ioctl(fd, CAFE_IOCTL_DUMP_MEM))) {
    perror("ioctl failed!\n");
    return 1;
  }

  return 0;
}
