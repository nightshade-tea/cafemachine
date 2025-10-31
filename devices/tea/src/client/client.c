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

static int parse_flags(int argc, char **argv, struct tea_request *req) {
  int opt;

  static struct option long_options[] = {
      {"strict", no_argument, 0, 'S'},
      {"relaxed", no_argument, 0, 'R'},
      {"start", required_argument, 0, 'B'},
      {"end", required_argument, 0, 'E'},
      {"basename", required_argument, 0, 'N'},
      {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "SRB:E:N:", long_options, NULL)) !=
         -1) {
    switch (opt) {
    case 'S':
      req->cmd = TEA_DUMP_STRICT;
      break;

    case 'R':
      req->cmd = TEA_DUMP_RELAXED;
      break;

    case 'B':
      errno = 0;
      req->start = strtoull(optarg, NULL, 0);

      if (errno) {
        perror("failed to parse start address\n");
        return EINVAL;
      }

      break;

    case 'E':
      errno = 0;
      req->end = strtoull(optarg, NULL, 0);

      if (errno) {
        perror("failed to parse end address\n");
        return EINVAL;
      }

      break;

    case 'N':
      if (strlen(optarg) > sizeof req->basename) {
        perror("basename is too long\n");
        return EINVAL;
      }

      strncpy(req->basename, optarg, sizeof req->basename);
      break;

    default:
      fprintf(stderr, "invalid arguments\n");
      return EINVAL;
    }
  }

  return 0;
}

static int dump(int argc, char **argv) {
  const char *hostname;
  struct tea_request req = {0};
  int sock;
  int err;

  /* default values */
  req.magic = TEA_MAGIC;
  req.cmd = TEA_DUMP_STRICT;

  if ((err = parse_flags(argc, argv, &req)))
    return err;

  if (optind >= argc) {
    fprintf(stderr, "missing hostname argument\n");
    return EINVAL;
  }

  hostname = argv[optind];

  if ((sock = connect_tea(hostname)) < 0)
    return sock;

  err = send_req(sock, &req);
  close(sock);

  return err;
}

static int get(int argc, char **argv) {
  const char *hostname;
  struct tea_request req = {0};
  int sock;
  int err;

  /* default values */
  req.magic = TEA_MAGIC;
  req.cmd = TEA_RECV_DUMP;

  if ((err = parse_flags(argc, argv, &req)))
    return err;

  if (optind >= argc) {
    fprintf(stderr, "missing hostname argument\n");
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

static int help(int argc, char **argv, const char *name) {
  if (argc > 1) {
    fprintf(stderr, "too many arguments\n");
    return EINVAL;
  }

  if (argc == 0) {
    usage(name);
    return 0;
  }

  if (strcmp(argv[0], "dump") == 0) {
    usage_dump(name);
    return 0;
  }

  else if (strcmp(argv[0], "get") == 0) {
    usage_get(name);
    return 0;
  }

  else if (strcmp(argv[0], "help") == 0) {
    usage_help(name);
    return 0;
  }

  fprintf(stderr, "unknown command\n");
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

  if (strcmp(command, "dump") == 0)
    return dump(argc, argv);

  else if (strcmp(command, "get") == 0)
    return get(argc, argv);

  else if (strcmp(command, "help") == 0)
    return help(argc, argv, name);

  fprintf(stderr, "unknown command '%s'\n", command);
  ask_for_help(name);
  return EINVAL;
}
