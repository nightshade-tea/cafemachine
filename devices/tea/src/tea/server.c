#include "server.h"
#include "dump.h"
#include "tea_device.h"
#include "tea_log.h"
#include "utils.h"
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

static ssize_t tea_send(int sock, enum tea_msg_t type, const void *payload,
                        uint64_t payloadsz)
{
    struct tea_header header = {
        .magic = TEA_MAGIC,
        .type = type,
        .payloadsz = payloadsz,
    };

    ssize_t bytes_sent;
    if ((bytes_sent = send(sock, &header, sizeof header, 0)) != sizeof header) {
        tea_log("send() failed : %zd bytes sent : err %s\n", bytes_sent,
                strerror(errno));
        return MIN(bytes_sent, 0);
    }

    uint64_t offset = 0;
    while (offset < payloadsz) {

        if ((bytes_sent = send(sock, payload + offset, payloadsz - offset, 0)) <
            0) {
            tea_log("send() failed");
            return offset;
        }

        offset += bytes_sent;
    }

    return offset;
}

static size_t tea_recv(int sock, void *buf, uint64_t bufsz)
{
    struct tea_header header = {0};

    ssize_t bytes_recv;
    if ((bytes_recv = recv(sock, &header, sizeof header, 0)) != sizeof header) {
        if (bytes_recv) {
            tea_log("recv() failed : %ld\n", bytes_recv);
        }
        return bytes_recv;
    }

    if (header.magic != TEA_MAGIC) {
        tea_log("header has invalid magic\n");
        return -1;
    }

    if (bufsz < header.payloadsz) {
        tea_log("not enough space on buffer\n");
        return -1;
    }

    uint64_t offset = 0;
    while (offset < header.payloadsz) {

        if ((bytes_recv =
                 recv(sock, buf + offset, header.payloadsz - offset, 0)) < 0) {
            tea_log("recv() failed");
            return offset;
        }

        offset += bytes_recv;
    }

    return offset;
}

static void tea_resp(int sock, enum tea_status status, const char *err_msg)
{
    struct tea_response resp = {.status = status};

    strncpy(resp.err_msg, err_msg, TEA_MAX_ERR_MSG_LEN);
    tea_send(sock, TEA_RES, &resp, sizeof resp);
}

static int create_listen_sock(void)
{
    struct sockaddr_in addr;
    int sock;

    errno = 0;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        tea_log("socket() failed: %s\n", strerror(errno));
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(TEA_SERVER_PORT);

    if (bind(sock, (struct sockaddr *)&addr, sizeof addr) < 0) {
        tea_log("bind() failed: %s\n", strerror(errno));
        return -1;
    }

    if (listen(sock, TEA_CONN_BACKLOG) < 0) {
        tea_log("listen() failed: %s\n", strerror(errno));
        return -1;
    }

    return sock;
}

static void tea_send_dump(TeaState *dev, char basename[TEA_MAX_BASENAME_LEN],
                          int conn)
{
    char filename[TEA_MAX_BASENAME_LEN + sizeof(TEA_DUMP_EXT)];
    int dumpf;
    struct stat stat;
    off_t dumpsz;
    void *dump;

    mount_filename(basename, filename);

    if ((dumpf = open(filename, O_RDONLY)) < 0) {
        tea_log("open() failed %s\n", strerror(errno));
        tea_resp(conn, TEA_FAIL, TEA_ERR_INVALID_DUMP);
        return;
    }

    if (fstat(dumpf, &stat) < 0) {
        tea_log("fstat() failed %s\n", strerror(errno));
        tea_resp(conn, TEA_FAIL, TEA_ERR_INTERNAL);
        goto close_file;
    }

    dumpsz = stat.st_size;

    if ((dump = mmap(NULL, dumpsz, PROT_READ, MAP_SHARED, dumpf, 0)) ==
        MAP_FAILED) {
        tea_log("mmap() failed %s\n", strerror(errno));
        tea_resp(conn, TEA_FAIL, TEA_ERR_INTERNAL);
        goto close_file;
    }

    if (madvise(dump, dumpsz, MADV_SEQUENTIAL) < 0) {
        tea_log("madvise() failed %s\n", strerror(errno));
        tea_resp(conn, TEA_FAIL, TEA_ERR_INTERNAL);
        goto unmap_file;
    }

    tea_log("sending dump...\n");

    if (tea_send(conn, TEA_DATA, dump, dumpsz) < 0) {
        tea_log("failed : %s\n", strerror(errno));
    } else {
        tea_log("dump sent!\n");
    }

unmap_file:
    munmap(dump, dumpsz);
close_file:
    close(dumpf);

    return;
}

static ssize_t req_handler(TeaState *dev, int conn)
{
    struct tea_request req;
    ssize_t recvb;

    errno = 0;

    while ((recvb = tea_recv(conn, &req, sizeof req)) > 0) {
        if (recvb != sizeof req) {
            tea_log("invalid request received: " TEA_ERR_INVALID_SZ
                    " (recvb=%zu)\n",
                    recvb);
            tea_resp(conn, TEA_FAIL, TEA_ERR_INVALID_SZ);
            continue;
        }

        switch (req.cmd) {
        case TEA_DUMP_STRICT:
            tea_dump_mem_strict(dev, req.basename, req.start);
            break;

        case TEA_DUMP_RELAXED:
            tea_dump_mem_relaxed(dev, req.basename, req.start, req.end);
            break;

        case TEA_RECV_DUMP:
            tea_send_dump(dev, req.basename, conn);
            break;

        default:
            tea_log("invalid request received: " TEA_ERR_INVALID_CMD
                    " (cmd=%#x)\n",
                    req.cmd);
            tea_resp(conn, TEA_FAIL, TEA_ERR_INVALID_CMD);
            continue;
        }

        tea_resp(conn, TEA_SUCCESS, TEA_ERR_SUCCESS);
    }

    return recvb;
}

static void serv(TeaState *dev)
{
    int conn;

    for (;;) {
        errno = 0;

        tea_log("waiting for connection\n");

        if ((conn = accept(dev->listen_sock, NULL, NULL)) < 0) {
            tea_log("accept() failed: %s\n", strerror(errno));
            continue;
        }

        tea_log("connection accepted\n");
        req_handler(dev, conn);

        close(conn);
    }
}

void *server_thread_start(void *pci_dev)
{
    TeaState *dev = TEA_DEVICE(pci_dev);

#if TEA_HOTPLUG_DUMP
    tea_dump_mem_strict(dev, TEA_DEFAULT_DUMP_BASENAME, 0);
#endif

    if ((dev->listen_sock = create_listen_sock()) < 0) {
        tea_log("create_listen_sock() failed\n");
        return NULL;
    }

    serv(dev);
    close(dev->listen_sock);

    return NULL;
}
