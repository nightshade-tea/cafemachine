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
#define LATTE_DMA_BUF_SZ (2 * 0x100000) /* 2 MiB */

/* irq settings */
#define LATTE_IRQ_MAX_VECTORS 32

/* interrupt vector values */
__LATTE_ENUM(
    latte_irq_vec,
    LATTE_INT_DMA_BUF_AVAILABLE, /* notify a dma operation completion */
    LATTE_INT_VECTOR_CNT)

/* device operations (valid LATTE_CMD values) */
__LATTE_ENUM(latte_ops, LATTE_NOP,
             LATTE_DMA_READ,  /* read DMA_SZ bytes from DMA_SRC to dma_buf */
             LATTE_DMA_WRITE, /* write DMA_SZ bytes from dma_buf to DMA_DST */
)

/* device control registers */
__LATTE_ENUM(latte_reg,
             LATTE_CMD,     /* register for triggering device operations */
             LATTE_DMA_SRC, /* source physical address to read into dma_buf */
             LATTE_DMA_DST, /* destination physical address to write dma_buf */
             LATTE_DMA_SZ,  /* dma operation read/write size */
             LATTE_LOWER_INT, /* write the interrupt number to lower */
             LATTE_REG_CNT)

/* driver operation completion ids (synchronization) */
__LATTE_ENUM(latte_complete,
             LATTE_WAIT_DMA, /* wait for dma operation completion */
             LATTE_WAIT_CNT)

#endif /* LATTE_H */
