#include "cafe.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

int main() {
  char basename[sizeof(unsigned long)] = "mem";
  unsigned long arg;
  int fd;

  memcpy(&arg, basename, sizeof arg);

  if ((fd = open("/dev/cafe", O_RDWR)) < 0) {
    perror("failed to open /dev/cafe");
    return 1;
  }

  if (ioctl(fd, CAFE_IOCTL_DUMP_MEM, arg) < 0) {
    perror("ioctl failed");
    return 2;
  }

  return 0;
}
