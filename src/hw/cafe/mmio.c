#include "mmio.h"
#include "dma.h"
#include "cafe_log.h"
#include "irq.h"
#include "dump.h"
#include <sys/param.h>

#define ACCESS_SIZE 8
#define VALID_MIN_ACCESS_SIZE ACCESS_SIZE
#define VALID_MAX_ACCESS_SIZE ACCESS_SIZE
#define IMPL_MIN_ACCESS_SIZE ACCESS_SIZE
#define IMPL_MAX_ACCESS_SIZE ACCESS_SIZE

static void cafe_cmd (CafeState *dev) {
  switch (dev->r[CAFE_CMD]) {
    case CAFE_DMA_READ:
      cafe_dma_read (dev, dev->dma_buf);
      break;

    case CAFE_DMA_WRITE:
      cafe_dma_write (dev, dev->dma_buf);
      break;

    case CAFE_DUMP_DMA_BUF:
      cafe_dump_buf(dev, dev->dma_buf);
      cafe_irq_raise(dev, CAFE_INT_DUMP_READY);
      break;
  }

  dev->r[CAFE_CMD] = 0;
}

static uint64_t cafe_mmio_read(void *opaque, hwaddr addr, unsigned size) {
    CafeState *dev = opaque;
    cafe_log("got mmio read of %u bytes at addr %lx\n", size, addr);

    switch (addr / ACCESS_SIZE) {
      case CAFE_CMD:
        return dev->r[CAFE_CMD];

      case CAFE_DMA_SRC:
        return dev->r[CAFE_DMA_SRC];

      case CAFE_DMA_DST:
        return dev->r[CAFE_DMA_DST];

      case CAFE_DMA_SZ:
        return dev->r[CAFE_DMA_SZ];

      case CAFE_DUMP_FILENAME:
        return dev->r[CAFE_DUMP_FILENAME];
    }

    return 0LL;
}

static void cafe_mmio_write(void *opaque, hwaddr addr, uint64_t data,
        unsigned size) {
    CafeState *dev = opaque;


    cafe_log("got mmio write of %u bytes at addr %lx: data=%lx\n", size,
        addr, data);

    switch (addr / ACCESS_SIZE) {
      case CAFE_CMD:
        dev->r[CAFE_CMD] = data;
        cafe_cmd(dev);
        break;

      case CAFE_DMA_SRC:
        dev->r[CAFE_DMA_SRC] = data;
        break;

      case CAFE_DMA_DST:
        dev->r[CAFE_DMA_DST] = data;
        break;

      case CAFE_DMA_SZ:
        dev->r[CAFE_DMA_SZ] = MIN(data, CAFE_DMA_BUF_SZ);
        break;

      case CAFE_DUMP_FILENAME:
        dev->r[CAFE_DUMP_FILENAME] = data;
        break;

      case CAFE_LOWER_INT:
        cafe_irq_lower(dev, data);
        break;
    }

}

void cafe_mmio_init(CafeState *dev, Error **errp) {
    memory_region_init_io(&dev->mmio, OBJECT(dev), &cafe_mmio_ops, dev,
            CAFE_MMIO_NAME, CAFE_MMIO_SIZE);
    pci_register_bar(&dev->pci_dev, CAFE_MMIO_BAR_NUM,
            PCI_BASE_ADDRESS_SPACE_MEMORY, &dev->mmio);

    cafe_log("mmio initialized\n");
}

const MemoryRegionOps cafe_mmio_ops = {
    .read = cafe_mmio_read,
    .write = cafe_mmio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = VALID_MIN_ACCESS_SIZE,
        .max_access_size = VALID_MAX_ACCESS_SIZE,
    },
    .impl = {
        .min_access_size = IMPL_MIN_ACCESS_SIZE,
        .max_access_size = IMPL_MAX_ACCESS_SIZE,
    },
};
