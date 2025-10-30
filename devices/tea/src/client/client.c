#include "tea.h"
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int sock;
  socklen_t addrlen;
  struct sockaddr_in addr;
  struct hostent *server;
  struct tea_msg msg;

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

  msg.magic = TEA_MAGIC;
  msg.type = TEA_CMD_DUMP_STRICT;
  msg.start = 0;
  strcpy(msg.basename, "tea");

  printf("requesting dump... ");
  fflush(stdout);

  if (send(sock, &msg, sizeof msg, 0) < 0) {
    perror("send() failed");
    return errno;
  }

  if (recv(sock, &msg, sizeof msg, 0) < 0) {
    perror("recv() failed");
    return errno;
  }

  if (msg.magic != TEA_MAGIC) {
    printf("received invalid msg\n");
    return errno;
  }

  switch (msg.type) {
  case TEA_SUCCESS:
    printf("done\n");
    break;
  default:
    printf("failed\n");
    break;
  }

  close(sock);

  return 0;
}
