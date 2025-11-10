#ifndef TEA_H
#define TEA_H

#include <stdint.h>
#include <unistd.h>

/* device attributes */
#define TEA_HW_NAME "tea"
#define TEA_VENDOR_ID 0x1ced
#define TEA_DEVICE_ID 0x7ea

/* dma settings */
#define TEA_DMA_BUF_SZ (2 * 1024 * 1024) /* 2 MiB */
#define TEA_PAGE_SZ 4096

#define TEA_MAX_BASENAME_LEN 8
#define TEA_DEFAULT_DUMP_BASENAME "teadump"
#define TEA_DUMP_EXT ".dat" /* file extension for dump files */

/* dump options */
#define TEA_DEFAULT_DUMP_TOLERANCE                                             \
    1                           /* number of allowed consecutive failed reads */
#define TEA_HOTPLUG_DUMP 0      /* dump on hotplug feature */
#define TEA_DUMP_PAGES_ON_ERR 0 /* retry dump page by page on error */

/* server options */
#define TEA_MAGIC 0x1ced07ea
#define TEA_SERVER_PORT 7734
#define TEA_MAX_ERR_MSG_LEN 64
#define TEA_CONN_BACKLOG 1 /* max sz for the queue of pending connections */

#define TEA_ERR_SUCCESS "ok"
#define TEA_ERR_INVALID_MV "invalid magic value"
#define TEA_ERR_INVALID_SZ "invalid request size"
#define TEA_ERR_INVALID_CMD "invalid command"
#define TEA_ERR_NOT_FOUND "file not found"
#define TEA_ERR_INTERNAL "internal server error"

enum tea_cmd { TEA_DUMP_STRICT, TEA_DUMP_RELAXED, TEA_RECV_DUMP, TEA_CMD_CNT };

enum tea_status { TEA_SUCCESS, TEA_FAIL, TEA_STATUS_CNT };

enum tea_msg_type {
    TEA_RESPONSE,
    TEA_REQUEST,
    TEA_DATA_TRANSFER,
    TEA_MSG_TYPE_CNT
};

struct __attribute__((packed)) tea_header {
    uint32_t magic;
    enum tea_msg_type type;
    size_t payloadsz;
};

struct __attribute__((packed)) tea_request {
    enum tea_cmd cmd;
    uint64_t start;
    uint64_t end;
    uint32_t tolerance;
    char basename[TEA_MAX_BASENAME_LEN];
};

struct __attribute__((packed)) tea_response {
    enum tea_status status;
    char err_msg[TEA_MAX_ERR_MSG_LEN];
};
#endif /* TEA_H */
