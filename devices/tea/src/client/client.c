#include "tea.h"
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int sock;
  struct sockaddr_in addr;
  socklen_t addrlen;
  struct hostent *server;
  struct tea_request req;
  struct tea_response resp;

  if (argc < 2) {
    printf("usage: ./client <hostname>\n");
    return EINVAL;
  }

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket() failed");
    return errno;
  }

  if (!(server = gethostbyname(argv[1]))) {
    herror("gethostbyname() failed");
    return errno;
  }

  memset(&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(TEA_SERVER_PORT);
  memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);

  printf("connecting... ");
  fflush(stdout);

  addrlen = sizeof addr;
  if (connect(sock, (struct sockaddr *)&addr, addrlen) < 0) {
    perror("connect() failed");
    return errno;
  }

  printf("done\n");

  req.magic = TEA_MAGIC;
  req.cmd = TEA_DUMP_STRICT;
  req.start = 0;
  strncpy(req.basename, "tea", TEA_MAX_BASENAME_LEN);

  printf("requesting dump... ");
  fflush(stdout);

  if (send(sock, &req, sizeof req, 0) < 0) {
    perror("send() failed");
    return errno;
  }

  if (recv(sock, &resp, sizeof resp, 0) < 0) {
    perror("recv() failed");
    return errno;
  }

  if (resp.magic != TEA_MAGIC) {
    printf("received invalid response\n");
    return EXIT_FAILURE;
  }

  puts(resp.err_msg);
  close(sock);

  return resp.status;
}
