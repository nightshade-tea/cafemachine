#include "flags.h"
#include "tea.h"
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __OPTSTR                                                               \
  FLAG_OPTSTR_STRICT FLAG_OPTSTR_RELAXED FLAG_OPTSTR_START FLAG_OPTSTR_END     \
      FLAG_OPTSTR_TOLERANCE FLAG_OPTSTR_BASENAME

#define __DEF_FLAG(x) {FLAG_LONG_##x, FLAG_HAS_ARG_##x, 0, FLAG_SHORT_##x},

static struct option long_options[] = {
    __DEF_FLAG(STRICT) __DEF_FLAG(RELAXED) __DEF_FLAG(START) __DEF_FLAG(END)
        __DEF_FLAG(TOLERANCE) __DEF_FLAG(BASENAME){0, 0, 0, 0}};

int parse_flags(int argc, char *argv[], struct tea_request *req,
                int *set_flags) {
  int opt;
  int num_set;

  *set_flags = 0;
  num_set = 0;
  opterr = 1;

  while ((opt = getopt_long(argc, argv, __OPTSTR, long_options, NULL)) != -1) {
    num_set++;

    switch (opt) {
    case FLAG_SHORT_STRICT:
      *set_flags |= FLAG_STRICT;
      req->cmd = TEA_DUMP_STRICT;
      break;

    case FLAG_SHORT_RELAXED:
      *set_flags |= FLAG_RELAXED;
      req->cmd = TEA_DUMP_RELAXED;
      break;

    case FLAG_SHORT_START:
      errno = 0;

      *set_flags |= FLAG_START;
      req->start = strtoull(optarg, NULL, 0);

      if (errno) {
        perror("failed to parse start address\n");
        return -EINVAL;
      }

      break;

    case FLAG_SHORT_END:
      errno = 0;

      *set_flags |= FLAG_END;
      req->end = strtoull(optarg, NULL, 0);

      if (errno) {
        perror("failed to parse end address\n");
        return -EINVAL;
      }

      break;

    case FLAG_SHORT_TOLERANCE:
      errno = 0;

      *set_flags |= FLAG_TOLERANCE;
      req->tolerance = strtoul(optarg, NULL, 0);

      if (errno) {
        perror("failed to parse tolerance value\n");
        return -EINVAL;
      }

      break;

    case FLAG_SHORT_BASENAME:
      if (strlen(optarg) > sizeof req->basename) {
        fprintf(stderr, "basename is too long\n");
        return -EINVAL;
      }

      *set_flags |= FLAG_BASENAME;
      strncpy(req->basename, optarg, sizeof req->basename);
      break;

    default:
      return -EINVAL;
    }
  }

  return num_set;
}
