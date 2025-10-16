#ifndef CAFE_H
#define CAFE_H

/* device attributes */
#define CAFE_HW_NAME "cafe"
#define CAFE_VENDOR_ID 0x1ced
#define CAFE_DEVICE_ID 0xcafe

/* mmio settings */
#define CAFE_MMIO_NAME "cafe-mmio"
#define CAFE_MMIO_SIZE 4096
#define CAFE_MMIO_BAR_NUM 0
#define CAFE_MMIO_ACCESS_SIZE 8
#define CAFE_REG_SZ 8

/* dma settings */
#define CAFE_DMA_BUF_SZ (2 * 0x100000) /* 2 MiB */
#define CAFE_DMA_DUMP_EXT ".dat"

/* irq settings */
#define CAFE_IRQ_MAX_VECTORS 32

/* interrupt vector values */
enum cafe_irq_vec {
  CAFE_INT_DMA_BUF_AVAILABLE, /* notify a dma operation completion */
  CAFE_INT_VECTOR_CNT
};

/* device operations (valid CAFE_CMD values) */
enum cafe_ops {
  CAFE_NOP,
  CAFE_DMA_READ, /* read DMA_SZ bytes from DMA_SRC to dma_buf */
  CAFE_DMA_WRITE, /* write DMA_SZ bytes from dma_buf to DMA_DST */
  CAFE_DUMP_DMA_BUF, /* write contents of dma_buf into DUMP_FILE */
};

/* device control registers */
enum cafe_reg {
  CAFE_CMD, /* register for triggering device operations */
  CAFE_DMA_SRC, /* source physical address to read into dma_buf */
  CAFE_DMA_DST, /* destination physical address to write dma_buf */
  CAFE_DMA_SZ, /* dma operation read/write size */
  CAFE_DUMP_FILE, /* dump filename */
  CAFE_LOWER_INT, /* write the interrupt number to lower */
  CAFE_REG_CNT
};

/* driver ioctl operations */
enum cafe_ioctl {
  CAFE_IOCTL_DUMP_MEM, /* dump all memory into a dump file */
  CAFE_IOCTL_CNT
};

/* driver operation completion ids (synchronization) */
enum cafe_complete {
  CAFE_WAIT_DMA, /* wait for dma operation completion */
  CAFE_WAIT_CNT
};

#endif /* CAFE_H */
