#ifndef NETWORK_H
#define NETWORK_H

#include "tea.h"
#include <sys/types.h>

int connect_to_server(const char *hostname);

ssize_t send_request(int sock, const struct tea_request *req);

ssize_t recv_response(int sock, struct tea_response *resp);

ssize_t recv_dump(int sock, int dumpfd);

#endif /* network.h */
