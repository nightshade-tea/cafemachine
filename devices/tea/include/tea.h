#ifndef TEA_H
#define TEA_H

/* device attributes */
#define TEA_HW_NAME "tea"
#define TEA_VENDOR_ID 0x1ced
#define TEA_DEVICE_ID 0x7ea

/* dma settings */
#define TEA_DMA_BUF_SZ (2 * 1024 * 1024) /* 2 MiB */
#define TEA_DUMP_EXT ".dat"
#define TEA_DEFAULT_DUMP_BASENAME "teadump"
#define TEA_MAX_BASENAME_LEN 8
#define TEA_DUMP_TOLERANCE                                                     \
    1 /* number of allowed sequential failed reads                             \
       */
#define TEA_HOTPLUG_DUMP 0
#define TEA_PAGE_SZ 4096
#define TEA_DUMP_PAGES_ON_ERR 0

#define TEA_SERVER_PORT 0x77ea
#define TEA_MAGIC 0x1ced7ea

enum tea_cmd { TEA_CMD_DUMP };

struct tea_msg {
    int magic;
    int cmd;
};

#endif /* TEA_H */
