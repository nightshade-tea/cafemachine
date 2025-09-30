#ifndef CAFE_H
#define CAFE_H

#define CAFE_HW_NAME "cafe"
#define CAFE_VENDOR_ID 0x1ced
#define CAFE_DEVICE_ID 0xcafe

#define CAFE_MMIO_NAME "cafe-mmio"
#define CAFE_MMIO_SIZE 4096
#define CAFE_MMIO_BAR_NUM 0

#define CAFE_IRQ_MAX_VECTORS 32
#define CAFE_HW_MSI_VECTOR_CNT 1

#define CAFE_DMA_BUF_SZ 0x1000

enum cafe_cmd {
  CAFE_DMA_READ,
  CAFE_DMA_WRITE,
};

enum {
  CAFE_CMD,
  CAFE_DMA_SRC,
  CAFE_DMA_DST,
  CAFE_DMA_SZ,
  REG_MAX_NUM,
};

#endif /* CAFE_H */
