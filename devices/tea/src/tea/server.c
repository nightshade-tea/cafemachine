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

static int setup_listen_socket(void)
{
    struct sockaddr_in addr;
    int sock;

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

static ssize_t sendbuf(int sockfd, const void *buf, size_t len)
{
    size_t offset;
    ssize_t sentb;

    offset = 0;

    while (offset < len) {
        if ((sentb = send(sockfd, buf + offset, len - offset, MSG_NOSIGNAL)) <
            0) {
            if (errno == EINTR) {
                continue;
            }

            tea_log("send() failed: %s\n", strerror(errno));
            return -1;
        }

        offset += sentb;
    }

    return offset;
}

static ssize_t recvbuf(int sockfd, void *buf, size_t len)
{
    size_t offset;
    ssize_t recvb;

    offset = 0;

    while (offset < len) {
        if ((recvb = recv(sockfd, buf + offset, len - offset, 0)) < 0) {
            if (errno == EINTR) {
                continue;
            }

            tea_log("recv() failed: %s\n", strerror(errno));
            return -1;
        }

        if (recvb == 0) {
            return 0;
        }

        offset += recvb;
    }

    return offset;
}

static ssize_t send_payload(int sock, enum tea_msg_type type,
                            const void *payload, size_t payloadsz)
{
    struct tea_header header = {
        .magic = TEA_MAGIC,
        .type = type,
        .payloadsz = payloadsz,
    };

    if (sendbuf(sock, &header, sizeof header) < 0) {
        tea_log("failed to send header\n");
        return -1;
    }

    if (sendbuf(sock, payload, payloadsz) < 0) {
        tea_log("failed to send payload\n");
        return -1;
    }

    return payloadsz;
}

static ssize_t recv_payload(int sock, enum tea_msg_type type, void *payload,
                            size_t payloadsz)
{
    struct tea_header header;
    ssize_t err;

    if ((err = recvbuf(sock, &header, sizeof header)) < 0) {
        tea_log("failed to recv header\n");
        return -1;
    }

    if (err == 0) {
        return 0;
    }

    if (header.magic != TEA_MAGIC) {
        tea_log("header has invalid magic\n");
        return -1;
    }

    if (header.type != type) {
        tea_log("unexpected header type\n");
        return -1;
    }

    if (header.payloadsz != payloadsz) {
        tea_log("unexpected header payloadsz\n");
        return -1;
    }

    if (recvbuf(sock, payload, header.payloadsz) <= 0) {
        tea_log("failed to recv payload\n");
        return -1;
    }

    return payloadsz;
}

static void send_response(int sock, enum tea_status status, const char *err_msg)
{
    struct tea_response resp = {.status = status};

    strncpy(resp.err_msg, err_msg, TEA_MAX_ERR_MSG_LEN);
    send_payload(sock, TEA_RESPONSE, &resp, sizeof resp);
}

static void send_dump(int sock, char basename[TEA_MAX_BASENAME_LEN])
{
    char filename[TEA_MAX_BASENAME_LEN + sizeof TEA_DUMP_EXT];
    int dumpf;
    struct stat stat;
    off_t dumpsz;
    void *dump;

    assemble_filename(basename, filename);

    if ((dumpf = open(filename, O_RDONLY)) < 0) {
        tea_log("open() failed: %s\n", strerror(errno));

        if (errno == ENOENT) {
            send_response(sock, TEA_FAIL, TEA_ERR_NOT_FOUND);
        } else {
            send_response(sock, TEA_FAIL, TEA_ERR_INTERNAL);
        }

        return;
    }

    if (fstat(dumpf, &stat) < 0) {
        tea_log("fstat() failed: %s\n", strerror(errno));
        send_response(sock, TEA_FAIL, TEA_ERR_INTERNAL);
        goto exit_close;
    }

    if (!(dumpsz = stat.st_size)) { /* if dumpsz=0, mmap will fail ... */
        tea_log("dump file is empty, sending header only\n");
        send_payload(sock, TEA_DATA_TRANSFER, NULL, dumpsz);
        goto exit_close;
    }

    if ((dump = mmap(NULL, dumpsz, PROT_READ, MAP_SHARED, dumpf, 0)) ==
        MAP_FAILED) {
        tea_log("mmap() failed: %s\n", strerror(errno));
        send_response(sock, TEA_FAIL, TEA_ERR_INTERNAL);
        goto exit_close;
    }

    if (madvise(dump, dumpsz, MADV_SEQUENTIAL) < 0) {
        tea_log("madvise() failed: %s\n", strerror(errno));
        send_response(sock, TEA_FAIL, TEA_ERR_INTERNAL);
        goto exit_unmap;
    }

    tea_log("sending dump... ");

    if (send_payload(sock, TEA_DATA_TRANSFER, dump, dumpsz) == dumpsz) {
        qemu_log("done\n"); /* send_payload() already prints errors on fail */
    }

exit_unmap:
    munmap(dump, dumpsz);
exit_close:
    close(dumpf);
}

static void connection_handler(TeaState *dev, int conn_sock)
{
    struct tea_request req;

    while (recv_payload(conn_sock, TEA_REQUEST, &req, sizeof req) > 0) {
        switch (req.cmd) {
        case TEA_DUMP_STRICT: /* TODO: strict should consider req.end */
            tea_dump_mem_strict(dev, req.basename, req.start, req.tolerance);
            send_response(conn_sock, TEA_SUCCESS, TEA_ERR_SUCCESS);
            break;

        case TEA_DUMP_RELAXED:
            tea_dump_mem_relaxed(dev, req.basename, req.start, req.end);
            send_response(conn_sock, TEA_SUCCESS, TEA_ERR_SUCCESS);
            break;

        case TEA_RECV_DUMP:
            send_response(conn_sock, TEA_SUCCESS, TEA_ERR_SUCCESS);
            send_dump(conn_sock, req.basename);
            break;

        default:
            tea_log(TEA_ERR_INVALID_CMD ": cmd=%#x\n", req.cmd);
            send_response(conn_sock, TEA_FAIL, TEA_ERR_INVALID_CMD);
            continue;
        }
    }
}

static void serv(TeaState *dev)
{
    int conn;

    for (;;) {
        tea_log("waiting for connection...\n");

        if ((conn = accept(dev->listen_sock, NULL, NULL)) < 0) {
            tea_log("accept() failed: %s\n", strerror(errno));
            continue;
        }

        tea_log("connection accepted\n");
        connection_handler(dev, conn);

        close(conn);
        tea_log("connection closed\n");
    }
}

void *server_thread_start(void *pci_dev)
{
    TeaState *dev = TEA_DEVICE(pci_dev);

#if TEA_HOTPLUG_DUMP
    tea_dump_mem_strict(dev, TEA_DEFAULT_DUMP_BASENAME, 0);
#endif

    if ((dev->listen_sock = setup_listen_socket()) < 0) {
        tea_log("setup_listen_socket() failed\n");
        return NULL;
    }

    serv(dev);
    close(dev->listen_sock);

    return NULL;
}
