#include "mmio.h"
#include "irq.h"
#include "latte_log.h"
#include "mon.h"
#include <sys/param.h>

/* if a read or write size is outside of the valid range, qemu automatically
 * drops it */
#define VALID_MIN_ACCESS_SIZE LATTE_MMIO_ACCESS_SIZE
#define VALID_MAX_ACCESS_SIZE LATTE_MMIO_ACCESS_SIZE

/* set the size range for reads and writes supported by our implementation */
#define IMPL_MIN_ACCESS_SIZE LATTE_MMIO_ACCESS_SIZE
#define IMPL_MAX_ACCESS_SIZE LATTE_MMIO_ACCESS_SIZE

/* handler for all device operations triggered by writing to LATTE_CMD */
static void latte_cmd(LatteState *dev)
{

    latte_log("got %s command\n", latte_ops_str(dev->reg[LATTE_CMD]));

    switch (dev->reg[LATTE_CMD]) {
    case LATTE_REGISTER_MR:
        put_under_surveillance(dev->reg[LATTE_ADDR],
                               dev->reg[LATTE_ADDR] +
                                   dev->reg[LATTE_REGION_SZ]);
        latte_irq_raise(dev, LATTE_WAIT_NOTIFY);
        break;
    }

    /* reset LATTE_CMD after handling */
    dev->reg[LATTE_CMD] = LATTE_NOP;
}

static uint64_t latte_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    LatteState *dev = opaque;
    unsigned int idx;

    if ((idx = addr / LATTE_MMIO_ACCESS_SIZE) >= LATTE_REG_CNT) {
        latte_log("got invalid mmio read: addr=%#" PRIx64 " idx=%u\n", addr,
                  idx);
        return 0;
    }

    latte_log("got mmio read at register %s -> %#" PRIx64 "\n",
              latte_reg_str(idx), dev->reg[idx]);

    return dev->reg[idx];
}

static void latte_mmio_write(void *opaque, hwaddr addr, uint64_t data,
                             unsigned size)
{
    LatteState *dev = opaque;
    unsigned int idx;

    if ((idx = addr / LATTE_MMIO_ACCESS_SIZE) >= LATTE_REG_CNT) {
        latte_log("got invalid mmio write: addr=%#" PRIx64 " idx=%u\n", addr,
                  idx);
        return;
    }

    latte_log("got mmio write at register %s <- %#" PRIx64 "\n",
              latte_reg_str(idx), data);

    /* special cases */
    switch (idx) {
    case LATTE_CMD:
        dev->reg[LATTE_CMD] = data;
        latte_cmd(dev);
        return;

    case LATTE_LOWER_INT:
        latte_irq_lower(dev, data);
        return;
    }

    dev->reg[idx] = data;
}

void latte_mmio_init(LatteState *dev, Error **errp)
{
    memory_region_init_io(&dev->mmio, OBJECT(dev), &latte_mmio_ops, dev,
                          LATTE_MMIO_NAME, LATTE_MMIO_SIZE);
    pci_register_bar(&dev->pci_dev, LATTE_MMIO_BAR_NUM,
                     PCI_BASE_ADDRESS_SPACE_MEMORY, &dev->mmio);

    latte_log("mmio initialized\n");
}

const MemoryRegionOps latte_mmio_ops = {
    .read = latte_mmio_read,
    .write = latte_mmio_write,
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
