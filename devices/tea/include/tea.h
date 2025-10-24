#ifndef TEA_H
#define TEA_H

/* device attributes */
#define TEA_HW_NAME "tea"
#define TEA_VENDOR_ID 0x1ced
#define TEA_DEVICE_ID 0x7ea

/* dma settings */
#define TEA_DMA_BUF_SZ (2 * 0x100000) /* 2 MiB */
#define TEA_DUMP_EXT ".dat"
#define TEA_DEFAULT_DUMP_BASENAME "teadump"
#define TEA_MAX_BASENAME_LEN 8

#endif /* TEA_H */
