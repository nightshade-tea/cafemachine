#include "flags.h"
#include "network.h"
#include "tea.h"
#include "usage.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int dump(int argc, char *argv[]) {
  struct tea_request req = {0};
  const char *hostname;
  int sock;
  int set_flags;
  int err;

  /* default values */
  req.cmd = TEA_DUMP_STRICT;

  if ((err = parse_flags(argc, argv, &req, &set_flags)) < 0) {
    ask_for_cmd_help(argv[0], "dump");
    return -err;
  }

  if ((set_flags & FLAG_STRICT) && (set_flags & FLAG_RELAXED)) {
    fprintf(stderr, "conflicting options '--" FLAG_LONG_STRICT
                    "' and '--" FLAG_LONG_RELAXED "' \n");
    ask_for_cmd_help(argv[0], "dump");
    return EINVAL;
  }

  if ((set_flags & FLAG_RELAXED) && !(set_flags & FLAG_END)) {
    fprintf(stderr, "'--" FLAG_LONG_END "' is mandatory for relaxed mode\n");
    ask_for_cmd_help(argv[0], "dump");
    return EINVAL;
  }

  if ((optind + 1) >= argc) {
    fprintf(stderr, "missing hostname argument\n");
    ask_for_cmd_help(argv[0], "dump");
    return EINVAL;
  }

  if (argc - (optind + 1) > 1) {
    fprintf(stderr, "too many arguments\n");
    ask_for_cmd_help(argv[0], "dump");
    return EINVAL;
  }

  if (!(set_flags & (FLAG_STRICT | FLAG_RELAXED)))
    printf("no mode specified, defaulting to " FLAG_LONG_STRICT "\n");

  hostname = argv[(optind + 1)];

  if ((sock = connect_to_server(hostname)) < 0)
    return sock;

  err = send_request(sock, &req);
  close(sock);

  return err;
}

static int get(int argc, char *argv[]) {
  const char *hostname;
  struct tea_request req = {0};
  int set_flags;
  int sock;
  int err;

  /* default values */
  req.cmd = TEA_RECV_DUMP;

  if ((err = parse_flags(argc, argv, &req, &set_flags)) < 0) {
    ask_for_cmd_help(argv[0], "get");
    return -err;
  }

  if (set_flags & (~FLAG_BASENAME)) {
    fprintf(stderr, "invalid flags\n");
    ask_for_cmd_help(argv[0], "get");
    return EINVAL;
  }

  if ((optind + 1) >= argc) {
    fprintf(stderr, "missing hostname argument\n");
    ask_for_cmd_help(argv[0], "get");
    return EINVAL;
  }

  if (argc - (optind + 1) > 1) {
    fprintf(stderr, "too many arguments\n");
    ask_for_cmd_help(argv[0], "get");
    return EINVAL;
  }

  hostname = argv[(optind + 1)];

  if ((sock = connect_to_server(hostname)) < 0)
    return sock;

  return recv_dump(sock, &req);
}

static int help(int argc, char *argv[]) {
  if (argc > 3) {
    fprintf(stderr, "too many arguments\n");
    ask_for_cmd_help(argv[0], "help");

    return EINVAL;
  }

  if (argc == 2) {
    usage(argv[0]);
    return 0;
  }

  if (!strcmp(argv[2], "dump")) {
    usage_dump(argv[0]);
    return 0;
  }

  else if (!strcmp(argv[2], "get")) {
    usage_get(argv[0]);
    return 0;
  }

  else if (!strcmp(argv[2], "help")) {
    usage_help(argv[0]);
    return 0;
  }

  fprintf(stderr, "unknown command '%s'\n", argv[2]);
  ask_for_cmd_help(argv[0], "help");

  return EINVAL;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    usage(argv[0]);
    return EINVAL;
  }

  if (!strcmp(argv[1], "dump"))
    return dump(argc, argv);

  else if (!strcmp(argv[1], "get"))
    return get(argc, argv);

  else if (!strcmp(argv[1], "help"))
    return help(argc, argv);

  fprintf(stderr, "unknown command '%s'\n", argv[1]);
  ask_for_help(argv[0]);

  return EINVAL;
}
