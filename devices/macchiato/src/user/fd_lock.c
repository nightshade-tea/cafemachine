/* Test chardev open () / release () locking mechanism (MACCHIATO_LOCK_FD)  */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int fd, s;

  if (argc != 2) {
    fprintf(stderr, "%s\n", strerror(EINVAL));
    return EINVAL;
  }

  if ((fd = open("/dev/macchiato0", 0)) < 0) {
    perror("open () :");
    return errno;
  }

  printf("/dev/macchiato0 opened\n");

  s = atoi(argv[1]);

  printf("sleep=%ds\n", s);
  sleep(s);

  if (close(fd)) {
    perror("close () :");
    return errno;
  }

  printf("/dev/macchiato0 closed\n");

  return 0;
}
