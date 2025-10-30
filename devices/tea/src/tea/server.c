#include "server.h"
#include "dump.h"
#include "tea_device.h"
#include "tea_log.h"
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

static int start_listening(void)
{
    struct sockaddr_in serv_addr;
    int sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        tea_log("failed to create socket : %s\n", strerror(errno));
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(TEA_SERVER_PORT);

    if (bind(sock, (struct sockaddr *)&serv_addr, sizeof serv_addr) < 0) {
        tea_log("failed to bind socket : %s\n", strerror(errno));
        return -1;
    }

    if (listen(sock, 1) < 0) {
        tea_log("failed to listen socket : %s\n", strerror(errno));
        return -1;
    }

    return sock;
}

static ssize_t msg_handler(TeaState *dev, int conn)
{
    struct tea_msg msg;
    ssize_t recvb;
    errno = 0;

    while ((recvb = recv(conn, &msg, sizeof msg, 0)) > 0) {
        if (msg.magic != TEA_MAGIC) {
            tea_log("invalid message received. ignoring\n");
            continue;
        }

        switch (msg.cmd) {
        case TEA_CMD_DUMP:
            tea_dump_mem_strict(dev, TEA_DEFAULT_DUMP_BASENAME, 0);
            break;
        default:
            tea_log("invalid command\n");
            break;
        }
    }

    return recvb;
}

static void conn_handler(TeaState *dev)
{
    int conn;

    for (;;) {
        errno = 0;

        tea_log("waiting for connection\n");

        if ((conn = accept(dev->listen_sock, NULL, NULL)) < 0) {
            tea_log("accept() failed : %s\n", strerror(errno));
            continue;
        }

        tea_log("connection accepted\n");

        if (msg_handler(dev, conn) < 0) {
            tea_log("recv() failed : %s\n", strerror(errno));
        }

        close(conn);
    }
}

void *server_thread_start(void *pci_dev)
{
    TeaState *dev = TEA_DEVICE(pci_dev);

#if TEA_HOTPLUG_DUMP
    tea_dump_mem_strict(dev, TEA_DEFAULT_DUMP_BASENAME, 0);
#endif

    if ((dev->listen_sock = start_listening()) < 0) {
        tea_log("start_listening() failed\n");
        return 0;
    }

    conn_handler(dev);

    close(dev->listen_sock);

    return 0;
}
