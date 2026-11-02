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
  size_t size = 4096;

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
  printf("[+] mmap ok at %p (size=%zu)\n", buf, size);

  const char *msg = "DMA memory mapping test successful.";
  strcpy(buf, msg);
  if (strcmp(buf, msg) == 0)
    printf("[+] read/write to DMA buffer ok\n");
  else
    printf("[-] read/write to DMA buffer FAILED\n");

  if (ioctl(fd, MACCHIATO_IOC_RESET_COMPILER, 0) < 0) {
    perror("[-] RESET_COMPILER");
    close(fd);
    return 1;
  }
  printf("[+] RESET_COMPILER ok\n");

  if (ioctl(fd, MACCHIATO_IOC_RESET_SCANNER, 0) < 0) {
    perror("[-] RESET_SCANNER");
    close(fd);
    return 1;
  }
  printf("[+] RESET_SCANNER ok\n");

  /* release() frees dmabuf, no munmap */
  close(fd);
  printf("[+] fd closed, hardware lock released\n");
  return 0;
}
