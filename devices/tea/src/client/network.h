#ifndef NETWORK_H
#define NETWORK_H

#include "tea.h"
#include <sys/types.h>

int connect_tea(const char *hostname);

ssize_t tea_send(int sock, enum tea_msg_type type, const void *payload,
                 size_t payloadsz);

int tea_recv(int sock, void *buf, uint64_t bufsz);

enum tea_status send_req(int sock, const struct tea_request *req);

int download_dump(int sock, struct tea_request *req);

#endif /* network.h */
