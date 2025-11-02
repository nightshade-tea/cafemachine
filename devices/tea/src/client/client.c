#include "flags.h"
#include "tea.h"
#include "usage.h"
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int connect_tea(const char *hostname) {
  int sock;
  struct sockaddr_in addr;
  struct hostent *server;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket() failed");
    return -1;
  }

  if (!(server = gethostbyname(hostname))) {
    herror("gethostbyname() failed");
    close(sock);
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(TEA_SERVER_PORT);
  memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);

  printf("connecting to %s... ", hostname);
  fflush(stdout);

  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("connect() failed");
    close(sock);
    return -1;
  }

  printf("done\n");
  return sock;
}

static enum tea_status send_req(int sock, const struct tea_request *req) {
  printf("sending request... ");
  fflush(stdout);

  if (send(sock, req, sizeof(*req), 0) < 0) {
    perror("send() failed");
    return TEA_FAIL;
  }

  struct tea_response resp;
  if (recv(sock, &resp, sizeof(resp), 0) < 0) {
    perror("recv() failed");
    return TEA_FAIL;
  }

  if (resp.magic != TEA_MAGIC) {
    fprintf(stderr, "response has invalid magic value\n");
    return TEA_FAIL;
  }

  puts(resp.err_msg);
  return resp.status;
}

static int dump(int argc, char *argv[]) {
  const char *hostname;
  struct tea_request req = {0};
  int sock;
  int num_flags;
  int set_flags;
  int err;

  /* default values */
  req.magic = TEA_MAGIC;
  req.cmd = TEA_DUMP_STRICT;

  if ((num_flags = parse_flags(argc, argv, &req, &set_flags)) < 0) {
    ask_for_cmd_help(argv[0], "dump");
    return -num_flags;
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

  if ((sock = connect_tea(hostname)) < 0)
    return sock;

  err = send_req(sock, &req);
  close(sock);

  return err;
}

static int get(int argc, char *argv[]) {
  const char *hostname;
  struct tea_request req = {0};
  int sock;
  int num_flags;
  int set_flags;
  int err;

  /* default values */
  req.magic = TEA_MAGIC;
  req.cmd = TEA_RECV_DUMP;

  if ((num_flags = parse_flags(argc, argv, &req, &set_flags)) < 0) {
    ask_for_cmd_help(argv[0], "get");
    return -num_flags;
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

  if ((sock = connect_tea(hostname)) < 0)
    return sock;

  err = send_req(sock, &req);

  /* todo: get dump file */

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
