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

typedef struct __attribute__((packed)) CafeMemory {
  uint8_t cmd;
  uint64_t dma_src;
  uint64_t dma_dst;
  uint64_t dma_sz;
} CafeMemory;

#define __CAFE_MEM_OFFSETOF(member) offsetof(struct CafeMemory, member)
#define __CAFE_MEM_SIZEOF(member) sizeof(((struct CafeMemory*)0)->member)

enum {
  CAFE_CMD = __CAFE_MEM_OFFSETOF(cmd),
  CAFE_DMA_SRC = __CAFE_MEM_OFFSETOF(dma_src),
  CAFE_DMA_DST = __CAFE_MEM_OFFSETOF(dma_dst),
  CAFE_DMA_SZ = __CAFE_MEM_OFFSETOF(dma_sz),
};

enum {
  CAFE_CMD_SIZE = __CAFE_MEM_SIZEOF(cmd),
  CAFE_DMA_SRC_SIZE = __CAFE_MEM_SIZEOF(dma_src),
  CAFE_DMA_DST_SIZE = __CAFE_MEM_SIZEOF(dma_dst),
  CAFE_DMA_SZ_SIZE = __CAFE_MEM_SIZEOF(dma_sz),
};

#endif /* CAFE_H */
