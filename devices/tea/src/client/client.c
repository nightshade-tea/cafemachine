#include "flags.h"
#include "network.h"
#include "tea.h"
#include "usage.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
assemble_filename(const char basename[TEA_MAX_BASENAME_LEN],
                  char filename[TEA_MAX_BASENAME_LEN + sizeof TEA_DUMP_EXT]) {
  size_t basename_len;

  basename_len = strnlen(basename, TEA_MAX_BASENAME_LEN);

  /* if unset, use default value */
  if (basename_len == 0) {
    basename = TEA_DEFAULT_DUMP_BASENAME;
    basename_len = sizeof(TEA_DEFAULT_DUMP_BASENAME) - 1;
  }

  memcpy(filename, basename, basename_len);
  memcpy(filename + basename_len, TEA_DUMP_EXT, sizeof TEA_DUMP_EXT);
}

static int create_dump_file(const struct tea_request *req) {
  char filename[TEA_MAX_BASENAME_LEN + sizeof TEA_DUMP_EXT];
  int dumpfd;

  assemble_filename(req->basename, filename);

  printf("dump file: %s\n", filename);

  if ((dumpfd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
    perror("open() failed");
    return -1;
  }

  return dumpfd;
}

static int dump(int argc, char *argv[]) {
  struct tea_request req = {0};
  struct tea_response resp;
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
    return ENOTCONN;

  if (send_request(sock, &req) < 0) {
    fprintf(stderr, "failed to send request\n");
    err = EPROTO;
    goto exit_close;
  }

  if (recv_response(sock, &resp) < 0) {
    fprintf(stderr, "failed to get server response\n");
    err = EPROTO;
    goto exit_close;
  }

  printf("server response: %s\n", resp.err_msg);

  close(sock);
  return 0;

exit_close:
  close(sock);
  return err;
}

static int get(int argc, char *argv[]) {
  struct tea_request req = {0};
  struct tea_response resp;
  const char *hostname;
  int sock;
  int dumpfd;
  int set_flags;
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
    return ENOTCONN;

  if (send_request(sock, &req) < 0) {
    fprintf(stderr, "failed to send request\n");
    err = EPROTO;
    goto err_close_sock;
  }

  if (recv_response(sock, &resp) < 0) {
    fprintf(stderr, "failed to get server response\n");
    err = EPROTO;
    goto err_close_sock;
  }

  printf("server response: %s\n", resp.err_msg);

  if (resp.status != TEA_SUCCESS) {
    fprintf(stderr, "server returned an error, aborting\n");
    err = EPROTO;
    goto err_close_sock;
  }

  if ((dumpfd = create_dump_file(&req)) < 0) {
    err = errno;
    fprintf(stderr, "failed to create dump file\n");
    goto err_close_sock;
  }

  if (recv_dump(sock, dumpfd) < 0) {
    fprintf(stderr, "data transfer failed\n");
    err = EPROTO;
    goto err_close_dumpfd;
  }

  close(dumpfd);
  close(sock);

  return 0;

err_close_dumpfd:
  close(dumpfd);

err_close_sock:
  close(sock);

  return err;
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
