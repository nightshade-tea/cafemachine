/* scan a file with yara rules on the macchiato device
 *
 * usage: scan <rules file> <target file> */

#include "macchiato.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define DEV_PATH "/dev/" MACCHIATO_HW_NAME "0"

/* must not exceed PAGE_SIZE << MAX_PAGE_ORDER, the driver can't allocate a
 * larger dma buffer */
#define BUF_SZ (2 * 1024 * 1024)

/* read up to len bytes at offset off, retrying short reads */
static ssize_t read_at(int fd, void *buf, size_t len, off_t off) {
  size_t total = 0;
  ssize_t n;

  while (total < len) {
    if ((n = pread(fd, (char *)buf + total, len - total, off + total)) < 0) {
      if (errno == EINTR)
        continue;

      return -1;
    }

    if (n == 0)
      break;

    total += n;
  }

  return total;
}

/* compile the rules in path on the device */
static int add_rules(int dev, char *dmabuf, const char *path) {
  ssize_t len;
  int fd;

  if ((fd = open(path, O_RDONLY)) < 0) {
    perror(path);
    return -1;
  }

  len = read_at(fd, dmabuf, BUF_SZ, 0);
  close(fd);

  if (len < 0) {
    perror(path);
    return -1;
  }

  /* the rules need one more byte for the null terminator */
  if (len == BUF_SZ) {
    fprintf(stderr, "%s: rules larger than the dma buffer\n", path);
    return -1;
  }

  dmabuf[len] = '\0';

  if (ioctl(dev, MACCHIATO_IOC_RESET_COMPILER, 0) < 0 ||
      ioctl(dev, MACCHIATO_IOC_ADD_RULE, len + 1) < 0) {
    fprintf(stderr, "%s: failed to compile rules: %s\n", path, strerror(errno));
    return -1;
  }

  return 0;
}

/* send the file in buffer sized chunks, in the order the device asks for them.
 *
 * the dma buffer is device memory, file reads can't write into it directly,
 * so chunks are read into buf first and then copied. */
static int scan_file(int dev, char *dmabuf, char *buf, const char *path) {
  unsigned long cmd = MACCHIATO_IOC_BEGIN_SCAN;
  off_t off = 0;
  ssize_t len;
  int fd, ret = -1;

  if ((fd = open(path, O_RDONLY)) < 0) {
    perror(path);
    return -1;
  }

  if (ioctl(dev, MACCHIATO_IOC_RESET_SCANNER, 0) < 0) {
    perror("reset scanner");
    goto out;
  }

  for (;;) {
    if ((len = read_at(fd, buf, BUF_SZ, off)) < 0) {
      perror(path);
      goto out;
    }

    /* the device asked for data past the end of the file */
    if (len == 0 && cmd != MACCHIATO_IOC_BEGIN_SCAN) {
      if (ioctl(dev, MACCHIATO_IOC_END_SCAN, 0) < 0) {
        perror("end scan");
        goto out;
      }

      break;
    }

    memcpy(dmabuf, buf, len);

    if (ioctl(dev, cmd, len) == 0)
      break;

    switch (errno) {
    case ENODATA: /* next chunk */
      off += len;
      cmd = MACCHIATO_IOC_CONTINUE_SCAN;
      break;

    case ESPIPE: /* first chunk again */
      off = 0;
      cmd = MACCHIATO_IOC_REWIND_SCAN;
      break;

    default:
      perror("scan");
      goto out;
    }
  }

  ret = 0;

out:
  close(fd);
  return ret;
}

/* results format, written by the device into the dma buffer:
 *
 *   uint64_t size                  total size, including this field
 *   then, for each rule:
 *     uint64_t size                size of this entry
 *     uint8_t matched              non zero if the rule matched
 *     char name[]                  null terminated rule name */
static int print_results(int dev, const char *dmabuf) {
  uint64_t total, size;
  size_t off;

  if (ioctl(dev, MACCHIATO_IOC_GET_RESULTS, 0) < 0) {
    perror("get results");
    return -1;
  }

  memcpy(&total, dmabuf, sizeof total);
  if (total > BUF_SZ)
    total = BUF_SZ;

  for (off = sizeof total; off < total; off += size) {
    if (off + sizeof size + 1 > total)
      goto malformed;

    memcpy(&size, dmabuf + off, sizeof size);
    if (size < sizeof size + 2 || size > total - off)
      goto malformed;

    printf("%-8s %.*s\n", dmabuf[off + sizeof size] ? "match" : "no match",
           (int)(size - sizeof size - 2), dmabuf + off + sizeof size + 1);
  }

  return 0;

malformed:
  fprintf(stderr, "malformed results\n");
  return -1;
}

int main(int argc, char **argv) {
  char *dmabuf, *buf;
  int dev, ret = 1;

  if (argc != 3) {
    fprintf(stderr, "usage: %s <rules file> <target file>\n", argv[0]);
    return 1;
  }

  if (!(buf = malloc(BUF_SZ))) {
    perror("malloc");
    return 1;
  }

  if ((dev = open(DEV_PATH, O_RDWR)) < 0) {
    perror(DEV_PATH);
    goto out_buf;
  }

  /* the driver frees the dma buffer when dev is closed, no munmap needed */
  dmabuf = mmap(NULL, BUF_SZ, PROT_READ | PROT_WRITE, MAP_SHARED, dev, 0);
  if (dmabuf == MAP_FAILED) {
    perror("mmap");
    goto out_dev;
  }

  if (add_rules(dev, dmabuf, argv[1]) < 0 ||
      scan_file(dev, dmabuf, buf, argv[2]) < 0 ||
      print_results(dev, dmabuf) < 0)
    goto out_dev;

  ret = 0;

out_dev:
  close(dev);
out_buf:
  free(buf);
  return ret;
}
