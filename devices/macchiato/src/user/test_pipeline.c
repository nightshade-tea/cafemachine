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

  /* have to use a userspace buf because of failed direct fread into dma buf
   *
   * [  269.776949] netfs: Couldn't get user pages (rc=-14)
   * [  269.777474] netfs: Zero-sized read [R=c2]
   * [  269.778152] ioctl( cmd=22787 arg=0 )
   *
   * rc=-14 is EFAULT (bad address). idk either, seems dmad regions do not
   * support direct file reads to it, so we need a middleman buf */
  char *buff = malloc(BUF_SIZE);
  if (!buff) {
    perror("malloc buff");
    goto out_mmap;
  }

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

  int scan_done = 0;
  size_t file_off = 0;

  int curr_ioc = MACCHIATO_IOC_BEGIN_SCAN;

  while (!scan_done) {
    ssize_t chunk_sz = read_chunk(target_f, file_off, buff, BUF_SIZE);
    if (chunk_sz < 0) {
      perror("[-] read_chunk");
      goto out_buff;
    }

    if (chunk_sz == 0) {
      if (ferror(target_f)) {
        fprintf(stderr, "[-] fread failed!\n");
        goto out_buff;
      }
    }

    memcpy(buf, buff, chunk_sz);

    if (ioctl(fd, curr_ioc, chunk_sz) == 0) {
      printf("[+] Scan complete\n");
      scan_done = 1;
      continue;
    }

    switch (errno) {
    case ESPIPE:
      printf("[+] Rewinding...\n");
      file_off = 0;
      curr_ioc = MACCHIATO_IOC_REWIND_SCAN;
      break;

    case ENODATA:
      printf("[+] Advancing...\n");
      file_off += chunk_sz;
      curr_ioc = MACCHIATO_IOC_CONTINUE_SCAN;
      break;

    case EAGAIN:
      fprintf(stderr, "[-] hardware error!\n");
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

out_buff:
  free(buff);
out_mmap:
  /* release() does it, no munmap */
out_fd:
  close(fd);
out_files:
  fclose(target_f);
  return ret;
}
