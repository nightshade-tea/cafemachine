#include "network.h"
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

static ssize_t writebuf(int fd, const void *buf, size_t count) {
  size_t offset;
  ssize_t writb;

  offset = 0;

  while (offset < count) {
    if ((writb = write(fd, buf + offset, count - offset)) < 0) {
      if (errno == EINTR)
        continue;

      perror("write() failed");
      return -1;
    }

    offset += writb;
  }

  return offset;
}

static ssize_t sendbuf(int sockfd, const void *buf, size_t len) {
  size_t offset;
  ssize_t sentb;

  offset = 0;

  while (offset < len) {
    if ((sentb = send(sockfd, buf + offset, len - offset, MSG_NOSIGNAL)) < 0) {
      if (errno == EINTR)
        continue;

      perror("send() failed");
      return -1;
    }

    offset += sentb;
  }

  return offset;
}

static ssize_t recvbuf(int sockfd, void *buf, size_t len) {
  size_t offset;
  ssize_t recvb;

  offset = 0;

  while (offset < len) {
    if ((recvb = recv(sockfd, buf + offset, len - offset, 0)) < 0) {
      if (errno == EINTR)
        continue;

      perror("recv() failed");
      return -1;
    }

    if (recvb == 0)
      return 0;

    offset += recvb;
  }

  return offset;
}

static ssize_t send_payload(int sock, enum tea_msg_type type,
                            const void *payload, size_t payloadsz) {
  struct tea_header header = {
      .magic = TEA_MAGIC,
      .type = type,
      .payloadsz = payloadsz,
  };

  if (sendbuf(sock, &header, sizeof header) < 0) {
    fprintf(stderr, "failed to send header\n");
    return -1;
  }

  if (sendbuf(sock, payload, payloadsz) < 0) {
    fprintf(stderr, "failed to send payload\n");
    return -1;
  }

  return payloadsz;
}

static ssize_t recv_header(int sock, enum tea_msg_type type,
                           struct tea_header *header) {
  if (recvbuf(sock, header, sizeof *header) <= 0) {
    fprintf(stderr, "failed to recv header\n");
    return -1;
  }

  if (header->magic != TEA_MAGIC) {
    fprintf(stderr, "header has invalid magic\n");
    return -1;
  }

  if (type != TEA_ANY && header->type != type) {
    fprintf(stderr, "unexpected header type\n");
    return -1;
  }

  return sizeof *header;
}

static ssize_t recv_payload(int sock, enum tea_msg_type type, void *payload,
                            size_t payloadsz) {
  struct tea_header header;

  if (recv_header(sock, type, &header) < 0)
    return -1;

  if (header.payloadsz != payloadsz) {
    fprintf(stderr, "unexpected header payloadsz\n");
    return -1;
  }

  if (recvbuf(sock, payload, header.payloadsz) <= 0) {
    fprintf(stderr, "failed to recv payload\n");
    return -1;
  }

  return payloadsz;
}

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

int connect_to_server(const char *hostname) {
  struct sockaddr_in addr = {0};
  struct hostent *server;
  int sock;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket() failed");
    return -1;
  }

  if (!(server = gethostbyname(hostname))) {
    herror("gethostbyname() failed");
    close(sock);
    return -1;
  }

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

ssize_t send_request(int sock, const struct tea_request *req) {
  printf("sending request... ");
  fflush(stdout);

  if (send_payload(sock, TEA_REQUEST, req, sizeof *req) < 0) {
    fprintf(stderr, "send_payload() failed\n");
    return -1;
  }

  printf("done\n");

  return sizeof *req;
}

ssize_t recv_response(int sock, struct tea_response *resp) {
  printf("waiting for response... ");
  fflush(stdout);

  if (recv_payload(sock, TEA_RESPONSE, resp, sizeof *resp) < 0) {
    fprintf(stderr, "recv_payload() failed\n");
    return -1;
  }

  printf("done\n");

  return sizeof *resp;
}

ssize_t recv_dump(int sock, const struct tea_request *req) {
  char filename[TEA_MAX_BASENAME_LEN + sizeof TEA_DUMP_EXT];
  struct tea_header header;
  struct tea_response response;
  int dumpfd;

  size_t bufsz;
  socklen_t optlen;
  void *buf;

  size_t offset;
  ssize_t recvb;

  /* ask for data transfer */
  if (send_request(sock, req) < 0)
    return -1;

  if (recv_header(sock, TEA_ANY, &header) < 0)
    return -1;

  /* check reply */
  switch (header.type) {
  case TEA_DATA_TRANSFER:
    break; /* good */

  case TEA_RESPONSE:
    struct tea_response resp;

    if (header.payloadsz != sizeof resp) {
      fprintf(stderr, "unexpected header payloadsz\n");
      return -1;
    }

    if (recvbuf(sock, &resp, sizeof resp) <= 0) {
      fprintf(stderr, "failed to recv payload\n");
      return -1;
    }

    fprintf(stderr, "got response: %s\n", resp.err_msg);
    return -1;

  default:
    fprintf(stderr, "unexpected header type\n");
    return -1;
  }

  /* create local dump file */
  assemble_filename(req->basename, filename);

  if ((dumpfd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 644)) < 0) {
    perror("open() failed");
    return -1;
  }

  /* allocate read buffer */
  optlen = sizeof bufsz;

  if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsz, &optlen) < 0) {
    perror("getsockopt() failed");
    goto err_close;
  }

  if (optlen != sizeof bufsz) {
    fprintf(stderr, "SO_RCVBUF is not size_t?\n");
    goto err_close;
  }

  if (!(buf = malloc(bufsz))) {
    perror("malloc() failed");
    goto err_close;
  }

  /* buffered read... */

  printf("receiving dump file... ");
  fflush(stdout);

  offset = 0;

  while (offset < header.payloadsz) {
    if ((recvb = recvbuf(sock, buf, MIN(bufsz, header.payloadsz - offset))) <
        0) {
      fprintf(stderr, "recvbuf() failed\n");
      goto err_free;
    }

    if (recvb == 0) {
      fprintf(stderr, "recvbuf(): connection closed\n");
      goto err_free;
    }

    if (writebuf(dumpfd, buf, recvb) < 0) {
      fprintf(stderr, "writebuf() failed\n");
      goto err_free;
    }

    offset += recvb;
  }

  recv_response(sock, &response);

  printf("done\n");

  free(buf);
  close(dumpfd);
  return 0;

err_free:
  free(buf);

err_close:
  close(dumpfd);
  return -1;
}
