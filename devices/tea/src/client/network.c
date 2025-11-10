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

/* TODO: unlink dump file on fail */
/* TODO: TEA_DUMP_TOLERANCE flag */
/* TODO: TEA_DUMP_PAGES_ON_ERR flag */
/* TODO: fix TEA_DUMP_TOLERANCE can't be zero bug */
/* TODO: rewrite strict dump mode considering an end address
 *       (if 0 then (1 << 48) - 1) else end */
/* TODO: after all these, test extensively */

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

  if (header->type != type) {
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

static size_t get_recvbufsz(int sock) {
  static socklen_t bufsz = 0; /* cached value */
  socklen_t optlen;

  if (bufsz)
    return bufsz;

  optlen = sizeof bufsz;

  if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsz, &optlen) < 0) {
    perror("getsockopt() failed");
    return bufsz = 0;
  }

  if (optlen != sizeof bufsz) {
    fprintf(stderr, "type mismatch getting SO_RCVBUF\n");
    return bufsz = 0;
  }

  return bufsz;
}

ssize_t recv_dump(int sock, int dumpfd) {
  struct tea_header transfer;
  size_t bufsz;
  void *buf;
  size_t offset;
  ssize_t recvb;

  /* read data transfer header */
  if (recv_header(sock, TEA_DATA_TRANSFER, &transfer) < 0) {
    fprintf(stderr, "failed to get data transfer header\n");
    return -1;
  }

  /* allocate read buffer */
  if (!(bufsz = get_recvbufsz(sock))) {
    fprintf(stderr, "get_recvbufsz() failed\n");
    return -1;
  }

  if (!(buf = malloc(bufsz))) {
    perror("malloc() failed");
    return -1;
  }

  /* buffered read from sock to file */
  printf("receiving dump file... ");
  fflush(stdout);

  offset = 0;

  while (offset < transfer.payloadsz) {
    if ((recvb = recvbuf(sock, buf, MIN(bufsz, transfer.payloadsz - offset))) <
        0) {
      fprintf(stderr, "recvbuf() failed\n");
      goto err_free_buf;
    }

    if (recvb == 0) {
      fprintf(stderr, "recvbuf(): connection closed\n");
      goto err_free_buf;
    }

    if (writebuf(dumpfd, buf, recvb) < 0) {
      fprintf(stderr, "writebuf() failed\n");
      goto err_free_buf;
    }

    offset += recvb;
  }

  printf("done\n");
  printf("%zd bytes transferred\n", offset);

  free(buf);
  return offset;

err_free_buf:
  free(buf);
  return -1;
}
