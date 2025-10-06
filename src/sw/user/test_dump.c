#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include "cafe.h"

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
