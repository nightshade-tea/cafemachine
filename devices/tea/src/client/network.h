#ifndef NETWORK_H
#define NETWORK_H

#include "tea.h"
#include <sys/types.h>

#define DUMP_FILE_PERMS 0644

int connect_to_server(const char *hostname);

ssize_t send_request(int sock, const struct tea_request *req);

ssize_t recv_response(int sock, struct tea_response *resp);

ssize_t recv_dump(int sock, const struct tea_request *req);

#endif /* network.h */
