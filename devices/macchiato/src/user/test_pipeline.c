#include "macchiato.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int fd;
  char *buf;
  size_t size = 1024 * 1024; // 1MB buffer

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <rule_string> <scan_target>\n", argv[0]);
    return 1;
  }

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

  // 1. ADD RULE
  size_t rule_len = strlen(argv[1]) + 1;
  strcpy(buf, argv[1]);
  printf("[*] Adding rule (%zu bytes)...\n", rule_len);
  if (ioctl(fd, MACCHIATO_OP_ADD_RULE, rule_len) < 0) {
    perror("ioctl ADD_RULE");
    goto cleanup;
  }

  // 2. SCAN BUFFER
  size_t target_len = strlen(argv[2]) + 1;
  strcpy(buf, argv[2]); // overwrite the rule with the target data
  printf("[*] Scanning buffer (%zu bytes)...\n", target_len);
  if (ioctl(fd, MACCHIATO_OP_SCAN_BUFFER, target_len) < 0) {
    perror("ioctl SCAN_BUFFER");
    goto cleanup;
  }

  // 3. GET RESULTS
  printf("[*] Fetching results...\n");
  if (ioctl(fd, MACCHIATO_OP_GET_RESULTS, 0) < 0) {
    perror("ioctl GET_RESULTS");
    goto cleanup;
  }

  // The QEMU backend should have overwritten the buffer with the results
  printf("[+] Scan complete. Hardware wrote back: \n%s\n", buf);

cleanup:
  close(fd);
  return 0;
}
