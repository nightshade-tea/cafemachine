#include "flags.h"
#include "tea.h"
#include "usage.h"
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
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

/* todo: comment behaviour */
static ssize_t tea_send(int sock, enum tea_msg_t type, const void *payload,
                        uint64_t payloadsz) {
  struct tea_header header = {
      .magic = TEA_MAGIC,
      .type = type,
      .payloadsz = payloadsz,
  };

  ssize_t bytes_sent;
  if ((bytes_sent = send(sock, &header, sizeof header, 0)) != sizeof header) {
    fprintf(stderr, "send() failed : %zd bytes sent : err %s\n", bytes_sent,
            strerror(errno));
    return MIN(bytes_sent, 0);
  }

  uint64_t offset = 0;
  while (offset < payloadsz) {

    if ((bytes_sent = send(sock, payload + offset, payloadsz - offset, 0)) <
        0) {
      perror("send() failed");
      return offset;
    }

    offset += bytes_sent;
  }

  return offset;
}

static int tea_recv(int sock, void *buf, uint64_t bufsz) {
  struct tea_header header = {0};

  ssize_t bytes_recv;
  if ((bytes_recv = recv(sock, &header, sizeof header, 0)) != sizeof header) {
    if (bytes_recv)
      fprintf(stderr, "recv() failed : %ld\n", bytes_recv);
    return bytes_recv;
  }

  if (header.magic != TEA_MAGIC) {
    fprintf(stderr, "header has invalid magic\n");
    return -1;
  }

  if (bufsz < header.payloadsz) {
    fprintf(stderr, "not enough space on buffer\n");
    return -1;
  }

  uint64_t offset = 0;
  while (offset < header.payloadsz) {

    if ((bytes_recv = recv(sock, buf + offset, header.payloadsz - offset, 0)) <
        0) {
      perror("recv() failed");
      return offset;
    }

    offset += bytes_recv;
  }

  return offset;
}

static enum tea_status send_req(int sock, const struct tea_request *req) {
  printf("sending request... ");
  fflush(stdout);

  if (tea_send(sock, TEA_REQ, req, sizeof(*req)) != sizeof(*req)) {
    perror("tea_send() failed");
    return TEA_FAIL;
  }

  struct tea_response resp;
  if (tea_recv(sock, &resp, sizeof(resp)) != sizeof(resp)) {
    perror("tea_recv() failed");
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

static int download_dump(int sock, struct tea_request *req) {
  char filename[TEA_MAX_BASENAME_LEN + sizeof(TEA_DUMP_EXT)] = {0};
  struct tea_response res = {0};
  struct tea_header header = {0};
  size_t total_bytes_recv;
  size_t basename_len;
  uint64_t dumpsz;
  int dumpf;
  int bufsz;
  socklen_t optlen;
  void *buf;

  printf("sending request... ");
  fflush(stdout);

  if (tea_send(sock, TEA_REQ, req, sizeof *req) <= 0) {
    fprintf(stderr, "tea_send() failed");
    goto close_sock;
  }

  printf("done\n");

  /* wait for response */
  if (recv(sock, &header, sizeof header, 0) <= 0) {
    perror("recv() failed");
    goto close_sock;
  }

  if (header.magic != TEA_MAGIC) {
    fprintf(stderr, "invalid header");
    goto close_sock;
  }

  if (header.type == TEA_RES) {
    if (recv(sock, &res, sizeof res, 0) < 0) {
      perror("recv() failed");
    } else {
      fprintf(stderr, "err: %s\n", res.err_msg);
    }
    goto close_sock;
  }

  dumpsz = header.payloadsz;

  printf("dump size: ~%jd Gb\n", dumpsz >> 30);
  fflush(stdout);

  /* creating filename and opening it */
  basename_len = strnlen(req->basename, TEA_MAX_BASENAME_LEN);
  memcpy(filename, req->basename, basename_len);
  memcpy(filename + basename_len, TEA_DUMP_EXT, sizeof(TEA_DUMP_EXT));
  if ((dumpf = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR)) <
      0) {
    perror("open() failed");
    goto close_sock;
  }

  /* allocate buffer for optimal receiving rate */
  optlen = sizeof bufsz;
  if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsz, &optlen) < 0) {
    perror("getsockopt() failed");
    goto close_file;
  }

  if (!(buf = malloc(bufsz))) {
    perror("malloc() failed");
    goto close_file;
  }

  printf("receiving dump file... ");
  fflush(stdout);

  /* receive dump file */
  total_bytes_recv = 0;
  while (total_bytes_recv < dumpsz) {
    ssize_t bytes_recv;

    if ((bytes_recv =
             recv(sock, buf, MIN(bufsz, dumpsz - total_bytes_recv), 0)) < 0) {
      perror("recv() failed");
      goto free_buf;
    }

    if (write(dumpf, buf, bytes_recv) < bytes_recv) {
      perror("write() failed");
      goto free_buf;
    }

    total_bytes_recv += bytes_recv;
  }

  if (tea_recv(sock, &res, sizeof res) <= 0) {
    perror("tea_recv() failed");
    goto free_buf;
  }

  printf("done\n");
  fflush(stdout);

free_buf:
  free(buf);
close_file:
  close(dumpf);
close_sock:
  close(sock);

  return 0;
}

static int get(int argc, char *argv[]) {
  const char *hostname;
  struct tea_request req = {0};
  int num_flags;
  int set_flags;
  int sock;

  /* default values */
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

  return download_dump(sock, &req);
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
