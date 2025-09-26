#include "mmio.h"

#define VALID_MIN_ACCESS_SIZE 8
#define VALID_MAX_ACCESS_SIZE 8
#define IMPL_MIN_ACCESS_SIZE 8
#define IMPL_MAX_ACCESS_SIZE 8

static uint64_t cafe_mmio_read(void *opaque, hwaddr addr, unsigned size) {
    CafeState *dev = opaque;

    /* todo: check if read is valid */

    return dev->buf[addr / 8];
}

static void cafe_mmio_write(void *opaque, hwaddr addr, uint64_t data,
        unsigned size) {
    CafeState *dev = opaque;

    /* todo: check if write is valid */

    dev->buf[addr / 8] = data;
}

void cafe_mmio_init(CafeState *dev, Error **errp) {
    memory_region_init_io(&dev->mmio, OBJECT(dev), &cafe_mmio_ops, dev,
            CAFE_MMIO_NAME, CAFE_MMIO_SIZE);
    pci_register_bar(&dev->pci_dev, CAFE_MMIO_BAR_NUM,
            PCI_BASE_ADDRESS_SPACE_MEMORY, &dev->mmio);
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
