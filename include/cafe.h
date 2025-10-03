#ifndef CAFE_H
#define CAFE_H

#define CAFE_HW_NAME "cafe"
#define CAFE_VENDOR_ID 0x1ced
#define CAFE_DEVICE_ID 0xcafe

#define CAFE_MMIO_NAME "cafe-mmio"
#define CAFE_MMIO_SIZE 4096
#define CAFE_MMIO_BAR_NUM 0
#define CAFE_REG_SZ 8

#define CAFE_IRQ_MAX_VECTORS 32

enum {
  CAFE_INT_DUMP_READY,
  CAFE_HW_MSI_VECTOR_CNT
};

#define CAFE_DMA_BUF_SZ 0x10000
#define CAFE_DMA_DUMP_EXT ".dat"

enum {
  CAFE_DMA_READ = 1,
  CAFE_DMA_WRITE,
  CAFE_DUMP_DMA_BUF,
};

enum {
  CAFE_CMD,
  CAFE_DMA_SRC,
  CAFE_DMA_DST,
  CAFE_DMA_SZ,
  CAFE_DUMP_FILENAME,
  CAFE_LOWER_INT, /* write the interrupt number to lower */
  REG_MAX_NUM
};

enum {
  CAFE_IOCTL_DUMP_MEM,
  CAFE_IOCTL_CNT
};

#endif /* CAFE_H */
