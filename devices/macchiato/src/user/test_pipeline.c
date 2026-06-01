#include "macchiato.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUF_SIZE (1024 * 1024)

static ssize_t read_chunk(FILE *f, size_t off, void *dst, size_t len) {
  if (fseek(f, (long)off, SEEK_SET) != 0)
    return -1;
  return (ssize_t)fread(dst, 1, len, f);
}

int main(int argc, char **argv) {
  int fd;
  char *buf;
  int ret = 1;

  if (argc != 3) {
    fprintf(stderr, "usage: %s <yara_rule_string> <target_file>\n", argv[0]);
    return 1;
  }

  /* 1. OPEN FILE */
  FILE *target_f = fopen(argv[2], "rb");
  if (!target_f) {
    perror("fopen target_file");
    return 1;
  }

  /* 2. OPEN DEVICE */
  if ((fd = open("/dev/macchiato0", O_RDWR)) < 0) {
    perror("open /dev/macchiato0");
    goto out_files;
  }

  /* 3. MMAP DMA BUFFER */
  buf = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (buf == MAP_FAILED) {
    perror("mmap");
    goto out_fd;
  }
  printf("[+] mmap ok (size=%d)\n", BUF_SIZE);

  /* 4. ADD_RULE */
  size_t rule_len = strlen(argv[1]) + 1;
  strcpy(buf, argv[1]);
  printf("[*] adding rule (%zu bytes)...\n", rule_len);
  if (ioctl(fd, MACCHIATO_IOC_ADD_RULE, rule_len) < 0) {
    perror("[-] ADD_RULE");
    goto out_mmap;
  }
  printf("[+] ADD_RULE ok\n");

  /* 5. SCAN */

  /* get target file size */
  fseek(target_f, 0, SEEK_END);
  size_t target_size = (size_t)ftell(target_f);
  rewind(target_f);

  printf("[*] target file size: %zu bytes\n", target_size);

  size_t file_off = 0;
  int scan_done = 0;
  int begin_scan = 1;

  while (!scan_done) {
    ssize_t chunk = read_chunk(target_f, file_off, buf, BUF_SIZE);
    if (chunk < 0) {
      perror("[-] read_chunk");
      goto out_mmap;
    }

    size_t chunk_sz = (size_t)chunk;

    int ioc =
        begin_scan ? MACCHIATO_IOC_BEGIN_SCAN : MACCHIATO_IOC_CONTINUE_SCAN;

    const char *ioc_name = begin_scan ? "BEGIN_SCAN" : "CONTINUE_SCAN";

    begin_scan = 0;

    printf("[*] %s (file_off=%zu chunk=%zu)...\n", ioc_name, file_off,
           chunk_sz);

    if (ioctl(fd, ioc, chunk_sz) == 0) {
      printf("[+] %s -> IRQ_DONE, scan complete\n", ioc_name);
      scan_done = 1;
      continue;
    }

    switch (errno) {
    case ESPIPE:
      printf("[+] %s -> IRQ_FIRST_BUF\n", ioc_name);
      file_off += chunk_sz;
      break;

    case ENODATA:
      printf("[+] %s -> IRQ_NEXT_BUF\n", ioc_name);
      file_off += chunk_sz;
      break;

    case EAGAIN:
      fprintf(stderr, "[-] %s -> IRQ_FAILED\n", ioc_name);
      goto out_mmap;

    default:
      fprintf(stderr, "[-] unexpected ioctl error: %d (%s)\n", errno,
              strerror(errno));
      goto out_mmap;
    }

    if (file_off >= target_size) {
      printf("[*] no more chunks, sending END_SCAN...\n");

      if (ioctl(fd, MACCHIATO_IOC_END_SCAN, 0) < 0) {
        perror("[-] END_SCAN");
        goto out_mmap;
      }

      printf("[+] END_SCAN ok\n");
      scan_done = 1;
    }
  }

  /* 6. GET_RESULTS */
  printf("[*] fetching results...\n");
  if (ioctl(fd, MACCHIATO_IOC_GET_RESULTS, 0) < 0) {
    perror("[-] GET_RESULTS");
    goto out_mmap;
  }
  printf("[+] GET_RESULTS ok\n");

  printf("[+] Scan complete. Hardware wrote back: \n%s\n", buf);
  ret = 0;

out_mmap:
  /* release() does it, no munmap */
out_fd:
  close(fd);
out_files:
  fclose(target_f);
  return ret;
}
