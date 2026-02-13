#ifndef LATTE_H
#define LATTE_H

#include "latte_enum.h"

/* device attributes */
#define LATTE_HW_NAME "latte"
#define LATTE_VENDOR_ID 0x1ced
#define LATTE_DEVICE_ID 0x1a73

/* mmio settings */
#define LATTE_MMIO_NAME "latte-mmio"
#define LATTE_MMIO_SIZE 4096
#define LATTE_MMIO_BAR_NUM 0
#define LATTE_MMIO_ACCESS_SIZE 8
#define LATTE_REG_SZ 8

/* dma settings */
#define LATTE_DMA_READ_SZ (2 * 0x100000) /* 2 MiB */
#define LATTE_PAGE_SZ 4096

/* dump options */
#define LATTE_DEFAULT_DUMP_TOLERANCE                                           \
    512 /* number of allowed consecutive failed reads */
#define LATTE_MIN_DUMP_SIZE 0x1000

/* irq settings */
#define LATTE_IRQ_MAX_VECTORS 32

/* surveillance settings */
#define LATTE_SURVEILLANCE_RATE 10
#define LATTE_HASH_ALGO EVP_sha256

/* interrupt vector values */
__LATTE_ENUM(latte_irq_vec, LATTE_INT_SYNC, LATTE_INT_VECTOR_CNT)

/* device operations (valid LATTE_CMD values) */
__LATTE_ENUM(latte_ops, LATTE_NOP,
             LATTE_REGISTER_MR, /* register memory region */
)

/* device control registers */
__LATTE_ENUM(latte_reg,
             LATTE_CMD, /* register for triggering device operations */
             LATTE_ADDR, LATTE_REGION_SZ,
             LATTE_LOWER_INT, /* write the interrupt number to lower */
             LATTE_REG_CNT)

/* driver operation completion ids (synchronization) */
__LATTE_ENUM(latte_complete, LATTE_WAIT_NOTIFY, LATTE_WAIT_CNT)

/* memory region status */
__LATTE_ENUM(latte_mr_status, LATTE_MR_STATUS_UNKNOWN, LATTE_MR_STATUS_NORMAL,
             LATTE_MR_STATUS_CORRUPTED)

#endif /* LATTE_H */
