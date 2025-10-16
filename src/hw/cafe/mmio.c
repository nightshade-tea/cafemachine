#include "mmio.h"
#include "cafe_log.h"
#include "dma.h"
#include "dump.h"
#include "irq.h"
#include <sys/param.h>

/* if a read or write size is outside of the valid range, qemu automatically
 * drops it */
#define VALID_MIN_ACCESS_SIZE CAFE_MMIO_ACCESS_SIZE
#define VALID_MAX_ACCESS_SIZE CAFE_MMIO_ACCESS_SIZE

/* set the size range for reads and writes supported by our implementation */
#define IMPL_MIN_ACCESS_SIZE CAFE_MMIO_ACCESS_SIZE
#define IMPL_MAX_ACCESS_SIZE CAFE_MMIO_ACCESS_SIZE

/* handler for all device operations triggered by writing to CAFE_CMD */
static void cafe_cmd(CafeState *dev) {

  /* driver must wait for the DMA_BUF_AVAILABLE interrupt to use the dma buffer
   * again. */

  switch (dev->reg[CAFE_CMD]) {
  case CAFE_DMA_READ:
    cafe_dma_read(dev);
    cafe_irq_raise(dev, CAFE_INT_DMA_BUF_AVAILABLE);
    break;

  case CAFE_DMA_WRITE:
    cafe_dma_write(dev);
    cafe_irq_raise(dev, CAFE_INT_DMA_BUF_AVAILABLE);
    break;

  case CAFE_DUMP_DMA_BUF:
    cafe_dump_buf(dev);
    cafe_irq_raise(dev, CAFE_INT_DMA_BUF_AVAILABLE);
    break;
  }

  /* reset CAFE_CMD after handling */
  dev->reg[CAFE_CMD] = CAFE_NOP;
}

static uint64_t cafe_mmio_read(void *opaque, hwaddr addr, unsigned size) {
  CafeState *dev = opaque;
  uint64_t idx;

  if ((idx = addr / CAFE_MMIO_ACCESS_SIZE) >= CAFE_REG_CNT) {
    cafe_log("got invalid mmio read of %u bytes at addr %lx\n", size, addr);
    return 0;
  }

  /* todo: stringify addr */
  cafe_log("got mmio read of %u bytes at addr %lx\n", size, addr);

  return dev->reg[idx];
}

static void cafe_mmio_write(void *opaque, hwaddr addr, uint64_t data,
                            unsigned size) {
  CafeState *dev = opaque;

  cafe_log("got mmio write of %u bytes at addr %lx: data=%lx\n", size, addr,
           data);

  switch (addr / CAFE_MMIO_ACCESS_SIZE) {
  case CAFE_CMD:
    dev->reg[CAFE_CMD] = data;
    cafe_cmd(dev);
    break;

  case CAFE_DMA_SRC:
    dev->reg[CAFE_DMA_SRC] = data;
    break;

  case CAFE_DMA_DST:
    dev->reg[CAFE_DMA_DST] = data;
    break;

  case CAFE_DMA_SZ:
    dev->reg[CAFE_DMA_SZ] = MIN(data, CAFE_DMA_BUF_SZ);
    break;

  case CAFE_DUMP_FILE:
    dev->reg[CAFE_DUMP_FILE] = data;
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
    .valid =
        {
            .min_access_size = VALID_MIN_ACCESS_SIZE,
            .max_access_size = VALID_MAX_ACCESS_SIZE,
        },
    .impl =
        {
            .min_access_size = IMPL_MIN_ACCESS_SIZE,
            .max_access_size = IMPL_MAX_ACCESS_SIZE,
        },
};
