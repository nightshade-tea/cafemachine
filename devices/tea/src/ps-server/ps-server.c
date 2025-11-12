#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PSS_BACKLOG_SZ 128
#define PS_CMD "ps --no-headers -e -o pid"
#define PID_MAX_LEN 22 /* max length for a null-terminated pid string*/

static int create_listen_sock(int port)
{
    struct sockaddr_in addr;
    int sock;

    errno = 0;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() failed");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("bind() failed");
        return -1;
    }

    if (listen(sock, PSS_BACKLOG_SZ) < 0) {
        perror("listen() failed");
        return -1;
    }

    return sock;
}

void *send_ps(void *opaque)
{
    FILE *ps;
    char pid[PID_MAX_LEN];
    int conn = *((int *)opaque);

    if (!(ps = popen(PS_CMD, "r"))) {
        perror("popen() failed");
        return NULL;
    }

    while (fgets(pid, PID_MAX_LEN, ps)) {
        ssize_t pid_len;
        uint32_t i = 0;

        while (isspace(pid[i])) {
            i++;
        }
        pid_len = strlen(pid) - i + 1;

        if (send(conn, pid + i, pid_len, 0) < pid_len) {
            perror("send() failed");
            break;
        }
    }

    close(conn);

    return NULL;
}

int main(int argc, char **argv)
{
    int port, listen_sock, conn, err;

    if (argc < 2) {
        fprintf(stderr, "No port given\n");
        return 1;
    }

    if (!(port = atoi(argv[1]))) {
        fprintf(stderr, "Invalid port given\n");
        return 1;
    }

    if ((listen_sock = create_listen_sock(port)) < 0) {
        perror("create_listen_sock() failed");
        return 1;
    }

    printf("starting to listen to connections\n");
    while ((conn = accept(listen_sock, NULL, NULL)) >= 0) {
        pthread_t thread;

        printf("accepted connection.\n");
        if ((err = pthread_create(&thread, NULL, send_ps, &conn))) {
            perror("pthread_create() failed");
        }
    }

    perror("accept() failed");

    close(listen_sock);

    return 0;
}
