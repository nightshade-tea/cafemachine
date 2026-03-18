#include "macchiato.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
  int fd;
  char *buf;
  size_t size = 4096; // 1 page

  if ((fd = open("/dev/macchiato0", O_RDWR)) < 0) {
    perror("open");
    return 1;
  }

  buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (buf == MAP_FAILED) {
    perror("mmap");
    close(fd);
    return 1;
  }

  printf("[+] mmap successful at %p\n", buf);

  // Test page access
  strcpy(buf, "DMA memory mapping test successful.");
  printf("[+] readback: %s\n", buf);

  // Test simple ioctl
  if (ioctl(fd, MACCHIATO_OP_RESET_COMPILER, 0) < 0) {
    perror("ioctl RESET_COMPILER");
  } else {
    printf("[+] RESET_COMPILER ioctl successful\n");
  }

  // Rely on your release() function to do the unmap and free
  close(fd);
  printf("[+] fd closed, hardware lock released\n");
  return 0;
}
