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
    printf("smt\n");
    return 1;
  }

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("failed to create socket");
    return 1;
  }

  if (!(server = gethostbyname(argv[1]))) {
    herror("failed to get host by name");
    return 1;
  }

  memset(&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(TEA_SERVER_PORT);
  memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);

  addrlen = sizeof addr;
  if (connect(sock, (struct sockaddr *)&addr, addrlen) < 0) {
    perror("failed to connect");
    return 1;
  }

  msg.magic = TEA_MAGIC;
  msg.cmd = TEA_CMD_DUMP;

  if (send(sock, &msg, sizeof msg, 0) < 0) {
    perror("failed to send msg");
    return 1;
  }

  printf("msg sent\n");

  close(sock);

  return 0;
}
