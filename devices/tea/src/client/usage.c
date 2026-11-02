#include "usage.h"
#include "flags.h"
#include "tea.h"
#include <stdio.h>

/* TODO: add more detailed explanations for each flag */
/* TODO: say which flags are mandatory, which are optional */
/* TODO: say which flags are specific to strict/relaxed mode */

#define DUMP_CMD_DESC "trigger a memory dump"
#define GET_CMD_DESC "download a memory dump from the " TEA_HW_NAME " device"
#define HELP_CMD_DESC "display help information"

#define STR(x) _STR(x)
#define _STR(x) #x

static void desc_flag_strict() {
  printf("\n  -%c, --" FLAG_LONG_STRICT "\n", FLAG_SHORT_STRICT);
  printf("    stop after '--" FLAG_LONG_TOLERANCE
         "' consecutive failed reads\n");
}

static void desc_flag_relaxed() {
  printf("\n  -%c, --" FLAG_LONG_RELAXED "\n", FLAG_SHORT_RELAXED);
  printf("    ignore failed reads\n");
}

static void desc_flag_start() {
  printf("\n  -%c, --" FLAG_LONG_START "\n", FLAG_SHORT_START);
  printf("    start dump from this physical address\n");
}

static void desc_flag_end() {
  printf("\n  -%c, --" FLAG_LONG_END "\n", FLAG_SHORT_END);
  printf("    stop dump after reaching this physical address\n");
}

static void desc_flag_dump_pages() {
  printf("\n  -%c, --" FLAG_LONG_DUMP_PAGES "\n", FLAG_SHORT_DUMP_PAGES);
  printf("    retry dump page by page on error\n");
}

static void desc_flag_tolerance() {
  printf("\n  -%c, --" FLAG_LONG_TOLERANCE "\n", FLAG_SHORT_TOLERANCE);
  printf("    number of allowed consecutive failed reads. if this flag is not\n"
         "    set or set to zero, TEA_DEFAULT_DUMP_TOLERANCE=" STR(
             TEA_DEFAULT_DUMP_TOLERANCE) " is used.\n");
}

static void desc_flag_basename() {
  printf("\n  -%c, --" FLAG_LONG_BASENAME "\n", FLAG_SHORT_BASENAME);
  printf("    basename for dump file (max " STR(
      TEA_MAX_BASENAME_LEN) " characters)\n");
}

static void list_commands() {
  printf("\ncommands:\n");
  printf("  dump -- " DUMP_CMD_DESC "\n");
  printf("  get  -- " GET_CMD_DESC "\n");
  printf("  help -- " HELP_CMD_DESC "\n");
}

static void desc_arg_hostname() {
  printf("\n  hostname\n");
  printf("    hostname for the " TEA_HW_NAME
         " device (where the server is running)\n");
}

void ask_for_help(const char *name) {
  printf("\nrun '%s help' for more information\n", name);
}

void ask_for_cmd_help(const char *name, const char *cmd) {
  if (!cmd) {
    printf("\nrun '%s help <command>' for more information about a specific "
           "command\n",
           name);
    return;
  }

  printf("\nrun '%s help %s' for more information about the %s command\n", name,
         cmd, cmd);
}

void usage(const char *name) {
  printf("usage: %s <command> [flags] [<args>]\n", name);
  list_commands();
  ask_for_cmd_help(name, NULL);
}

void usage_dump(const char *name) {
  printf("%s dump [flags] <hostname> -- " DUMP_CMD_DESC "\n", name);

  printf("\narguments:\n");
  desc_arg_hostname();

  printf("\nflags:\n");

  desc_flag_strict();
  desc_flag_relaxed();
  desc_flag_start();
  desc_flag_end();
  desc_flag_dump_pages();
  desc_flag_tolerance();
  desc_flag_basename();
}

void usage_get(const char *name) {
  printf("%s get [flags] <hostname> -- " GET_CMD_DESC "\n", name);

  printf("\narguments:\n");
  desc_arg_hostname();

  printf("\nflags:\n");
  desc_flag_basename();
}

void usage_help(const char *name) {
  printf("%s help <command> -- " HELP_CMD_DESC "\n", name);
  list_commands();
}
