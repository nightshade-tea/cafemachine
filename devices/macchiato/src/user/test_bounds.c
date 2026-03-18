#include "macchiato.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
  int fd;
  char *buf;
  size_t map_size = 4096;

  if ((fd = open("/dev/macchiato0", O_RDWR)) < 0) {
    perror("open");
    return 1;
  }

  printf("[!] testing ioctl without mapping memory...\n");
  if (ioctl(fd, MACCHIATO_OP_ADD_RULE, 100) < 0) {
    if (errno == EFAULT)
      printf("[+] correctly caught unmapped buffer (EFAULT)\n");
    else
      perror("[-] unexpected error");
  }

  buf = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (buf == MAP_FAILED) {
    perror("mmap");
    close(fd);
    return 1;
  }

  printf("[!] memory mapped (size=%zu). testing size overflow...\n", map_size);

  // Pass a size larger than the mapped buffer
  if (ioctl(fd, MACCHIATO_OP_SCAN_BUFFER, 8192) < 0) {
    if (errno == EINVAL)
      printf("[+] correctly caught size overflow (EINVAL)\n");
    else
      perror("[-] unexpected error");
  }

  close(fd);
  return 0;
}
