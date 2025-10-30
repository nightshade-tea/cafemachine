#ifndef TEA_SERVER_H
#define TEA_SERVER_H

#include "dump.h"
#include "tea_device.h"
#include "tea_log.h"
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

void *server_thread_start(void *pci_dev);

#endif /* TEA_SERVER_H */
