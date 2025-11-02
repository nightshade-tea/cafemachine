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

static int dump(int argc, char *argv[], const char *name) {
  const char *hostname;
  struct tea_request req = {0};
  int sock;
  int num_flags;
  int set_flags;
  int err;

  /* default values */
  req.magic = TEA_MAGIC;
  req.cmd = TEA_DUMP_STRICT;

  if ((num_flags = parse_flags(argc, argv, &req, &set_flags)) < 0)
    return -num_flags;

  if ((set_flags & FLAG_RELAXED) && !(set_flags & FLAG_END)) {
    fprintf(stderr, "'--" FLAG_LONG_END "' is mandatory for relaxed mode\n");
    ask_for_cmd_help(name, "get");
    return EINVAL;
  }

  if (argc - num_flags == 0) {
    fprintf(stderr, "missing hostname argument\n");
    ask_for_cmd_help(name, "get");
    return EINVAL;
  }

  if (argc - num_flags > 1) {
    fprintf(stderr, "too many arguments\n");
    ask_for_cmd_help(name, "get");
    return EINVAL;
  }

  if (!(set_flags & (FLAG_STRICT | FLAG_RELAXED)))
    printf("no mode specified, defaulting to " FLAG_LONG_STRICT "\n");

  hostname = argv[optind];

  if ((sock = connect_tea(hostname)) < 0)
    return sock;

  err = send_req(sock, &req);
  close(sock);

  return err;
}

static int get(int argc, char *argv[], const char *name) {
  const char *hostname;
  struct tea_request req = {0};
  int sock;
  int num_flags;
  int set_flags;
  int err;

  /* default values */
  req.magic = TEA_MAGIC;
  req.cmd = TEA_RECV_DUMP;

  if ((num_flags = parse_flags(argc, argv, &req, &set_flags)) < 0)
    return -num_flags;

  if (argc - num_flags == 0) {
    fprintf(stderr, "missing hostname argument\n");
    ask_for_cmd_help(name, "get");
    return EINVAL;
  }

  if (argc - num_flags > 1) {
    fprintf(stderr, "too many arguments\n");
    ask_for_cmd_help(name, "get");
    return EINVAL;
  }

  hostname = argv[optind];

  if ((sock = connect_tea(hostname)) < 0)
    return sock;

  err = send_req(sock, &req);

  /* todo: get dump file */

  close(sock);

  return err;
}

static int help(int argc, char *argv[], const char *name) {
  if (argc > 1) {
    fprintf(stderr, "too many arguments\n");
    ask_for_cmd_help(name, "help");

    return EINVAL;
  }

  if (argc == 0) {
    usage(name);
    return 0;
  }

  if (!strcmp(argv[0], "dump")) {
    usage_dump(name);
    return 0;
  }

  else if (!strcmp(argv[0], "get")) {
    usage_get(name);
    return 0;
  }

  else if (!strcmp(argv[0], "help")) {
    usage_help(name);
    return 0;
  }

  fprintf(stderr, "unknown command '%s'\n", argv[0]);
  ask_for_cmd_help(name, "help");

  return EINVAL;
}

int main(int argc, char *argv[]) {
  const char *name;
  const char *command;

  name = argv[0];

  if (argc < 2) {
    usage(name);
    return EINVAL;
  }

  command = argv[1];
  argc -= 2;
  argv += 2;

  if (!strcmp(command, "dump"))
    return dump(argc, argv, name);

  else if (!strcmp(command, "get"))
    return get(argc, argv, name);

  else if (!strcmp(command, "help"))
    return help(argc, argv, name);

  fprintf(stderr, "unknown command '%s'\n", command);
  ask_for_help(name);

  return EINVAL;
}
