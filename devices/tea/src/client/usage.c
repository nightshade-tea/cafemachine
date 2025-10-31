#include "usage.h"
#include <stdio.h>

#define STR(x) _STR(x)
#define _STR(x) #x
#define DUMP_CMD_DESC "trigger a memory dump"
#define GET_CMD_DESC "download a memory dump from the tea device"
#define HELP_CMD_DESC "display help information"

static void desc_flag_strict() {
  printf("\n  -S, --strict\n");
  printf("    stop after TEA_DUMP_TOLERANCE consecutive failed reads\n");
}

static void desc_flag_relaxed() {
  printf("\n  -R, --relaxed\n");
  printf("    ignore failed reads\n");
}

static void desc_flag_start() {
  printf("\n  -B, --start\n");
  printf("    start dump from this physical address\n");
}

static void desc_flag_end() {
  printf("\n  -E, --end\n");
  printf("    stop dump after reaching this physical address\n");
}

static void desc_flag_basename() {
  printf("\n  -N, --basename\n");
  printf("    basename for dump file (max " STR(
      TEA_MAX_BASENAME_LEN) " characters)\n");
}

void ask_for_help(const char *name) {
  printf("\nrun '%s help <command>' for more information\n", name);
}

void usage(const char *name) {
  printf("usage: %s <command> [flags] [<args>]\n", name);

  printf("\ncommands:\n");
  printf("  dump -- " DUMP_CMD_DESC "\n");
  printf("  get  -- " GET_CMD_DESC "\n");
  printf("  help -- " HELP_CMD_DESC "\n");

  ask_for_help(name);
}

void usage_dump(const char *name) {
  printf("%s dump [flags] <hostname> -- " DUMP_CMD_DESC "\n", name);

  printf("\nflags:\n");

  desc_flag_strict();
  desc_flag_relaxed();
  desc_flag_start();
  desc_flag_end();
  desc_flag_basename();
}

void usage_get(const char *name) {
  printf("%s get [flags] <hostname> -- " DUMP_CMD_DESC "\n", name);

  printf("\nflags:\n");
  desc_flag_basename();
}

void usage_help(const char *name) {
  printf("%s help <command> -- " DUMP_CMD_DESC "\n", name);
}
