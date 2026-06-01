#include "macchiato.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
  int fd, pass = 1;
  char *buf;
  size_t map_size = 4096;

  if ((fd = open("/dev/macchiato0", O_RDWR)) < 0) {
    perror("open");
    return 1;
  }

  printf("[*] testing ioctl without mmap...\n");
  if (ioctl(fd, MACCHIATO_IOC_ADD_RULE, 100) < 0) {
    if (errno == EFAULT) {
      printf("[+] ADD_RULE before mmap: correctly got EFAULT\n");
    } else {
      printf("[-] ADD_RULE before mmap: unexpected errno=%d (%s)\n", errno,
             strerror(errno));
      pass = 0;
    }
  } else {
    printf("[-] ADD_RULE before mmap: expected failure but got success\n");
    pass = 0;
  }

  if (ioctl(fd, MACCHIATO_IOC_BEGIN_SCAN, 100) < 0) {
    if (errno == EFAULT) {
      printf("[+] BEGIN_SCAN before mmap: correctly got EFAULT\n");
    } else {
      printf("[-] BEGIN_SCAN before mmap: unexpected errno=%d (%s)\n", errno,
             strerror(errno));
      pass = 0;
    }
  } else {
    printf("[-] BEGIN_SCAN before mmap: expected failure but got success\n");
    pass = 0;
  }

  printf("[+] mmapping...\n");
  buf = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (buf == MAP_FAILED) {
    perror("mmap");
    close(fd);
    return 1;
  }
  printf("[+] mmap ok (size=%zu)\n", map_size);

  printf("[*] testing size overflow on ADD_RULE...\n");
  if (ioctl(fd, MACCHIATO_IOC_ADD_RULE, map_size + 1) < 0) {
    if (errno == EINVAL) {
      printf("[+] ADD_RULE overflow: correctly got EINVAL\n");
    } else {
      printf("[-] ADD_RULE overflow: unexpected errno=%d (%s)\n", errno,
             strerror(errno));
      pass = 0;
    }
  } else {
    printf("[-] ADD_RULE overflow: expected failure but got success\n");
    pass = 0;
  }

  printf("[*] testing size overflow on BEGIN_SCAN...\n");
  if (ioctl(fd, MACCHIATO_IOC_BEGIN_SCAN, map_size + 1) < 0) {
    if (errno == EINVAL) {
      printf("[+] BEGIN_SCAN overflow: correctly got EINVAL\n");
    } else {
      printf("[-] BEGIN_SCAN overflow: unexpected errno=%d (%s)\n", errno,
             strerror(errno));
      pass = 0;
    }
  } else {
    printf("[-] BEGIN_SCAN overflow: expected failure but got success\n");
    pass = 0;
  }

  printf("[*] testing double mmap...\n");
  void *buf2 = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (buf2 == MAP_FAILED) {
    if (errno == ENFILE) {
      printf("[+] double mmap: correctly got ENFILE\n");
    } else {
      printf("[-] double mmap: unexpected errno=%d (%s)\n", errno,
             strerror(errno));
      pass = 0;
    }
  } else {
    printf("[-] double mmap: expected failure but got success\n");
    munmap(buf2, map_size);
    pass = 0;
  }

  close(fd);
  printf("\n%s\n",
         pass ? "[+] all bounds tests passed" : "[-] some tests FAILED");
  return 0;
}
