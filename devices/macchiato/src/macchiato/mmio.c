#include "mmio.h"

/* handler for all device operations triggered by writing to MACCHIATO_CMD */
static void macchiato_cmd(MacchiatoState *dev)
{

    macchiato_log("got %s command\n",
                  macchiato_ops_str(dev->reg[MACCHIATO_CMD]));

    switch (dev->reg[MACCHIATO_CMD]) {
    default:
        break;
    }

    /* reset MACCHIATO_CMD after handling */
    dev->reg[MACCHIATO_CMD] = MACCHIATO_NOP;
}

static uint64_t macchiato_reg_read(void *opaque, hwaddr addr, unsigned size)
{
    MacchiatoState *dev = opaque;
    unsigned int idx;

    if ((idx = addr / MACCHIATO_MMIO_ACCESS_SIZE) >= MACCHIATO_REG_CNT) {
        macchiato_log("got invalid mmio read: addr=%#" PRIx64 " idx=%u\n", addr,
                      idx);
        return 0;
    }

    macchiato_log("got mmio read at register %s -> %#" PRIx64 "\n",
                  macchiato_reg_str(idx), dev->reg[idx]);

    return dev->reg[idx];
}

static void macchiato_reg_write(void *opaque, hwaddr addr, uint64_t data,
                                unsigned size)
{
    MacchiatoState *dev = opaque;
    unsigned int idx;

    if ((idx = addr / MACCHIATO_MMIO_ACCESS_SIZE) >= MACCHIATO_REG_CNT) {
        macchiato_log("got invalid mmio write: addr=%#" PRIx64 " idx=%u\n",
                      addr, idx);
        return;
    }

    macchiato_log("got mmio write at register %s <- %#" PRIx64 "\n",
                  macchiato_reg_str(idx), data);

    /* special cases */
    switch (idx) {
    case MACCHIATO_CMD:
        dev->reg[MACCHIATO_CMD] = data;
        macchiato_cmd(dev);
        return;

    case MACCHIATO_LOWER_INT:
        macchiato_irq_lower(dev, data);
        return;
    }

    dev->reg[idx] = data;
}

static uint64_t macchiato_mem_read(void *opaque, hwaddr addr, unsigned size)
{
    MacchiatoState *dev = opaque;
    unsigned int idx;

    if (addr >= MACCHIATO_MMIO_BAR1_SIZE) {
        macchiato_log("got invalid mmio read: addr=%#" PRIx64 "\n", addr);
        return 0;
    }

    uint64_t ret = 0;
    memcpy(&ret, dev->mem + addr, size);

    return ret;
}

static void macchiato_mem_write(void *opaque, hwaddr addr, uint64_t data,
                                unsigned size)
{
    MacchiatoState *dev = opaque;
    unsigned int idx;

    if (addr >= MACCHIATO_MMIO_BAR1_SIZE ||
        addr + size >= MACCHIATO_MMIO_BAR1_SIZE) {
        macchiato_log("got invalid mmio write: addr=%#" PRIx64 " size=%u\n",
                      addr, size);
        return 0;
    }

    memcpy(dev->mem + addr, &data, size);
}

void macchiato_mmio_init(MacchiatoState *dev, Error **errp)
{
    memory_region_init_io(&dev->bar[0], OBJECT(dev), &macchiato_reg_ops, dev,
                          MACCHIATO_MMIO_BAR0_NAME, MACCHIATO_MMIO_BAR0_SIZE);

    pci_register_bar(&dev->pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY,
                     &dev->bar[0]);

    memory_region_init_ram(&dev->bar[1], OBJECT(dev), &macchiato_mem_ops, dev,
                           MACCHIATO_MMIO_BAR1_NAME, MACCHIATO_MMIO_BAR1_SIZE);

    pci_register_bar(&dev->pci_dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY,
                     &dev->bar[1])

        macchiato_log("mmio initialized\n");
}

const MemoryRegionOps macchiato_reg_ops = {
    .read = macchiato_reg_read,
    .write = macchiato_reg_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid =
        {
            .min_access_size = MACCHIATO_MMIO_BAR0_MIN_ACCESS_SIZE,
            .max_access_size = MACCHIATO_MMIO_BAR0_MAX_ACCESS_SIZE,
        },
    .impl =
        {
            .min_access_size = MACCHIATO_MMIO_BAR0_MIN_ACCESS_SIZE,
            .max_access_size = MACCHIATO_MMIO_BAR0_MAX_ACCESS_SIZE,
        },
};

const MemoryRegionOps macchiato_mem_ops = {
    .read = macchiato_mem_read,
    .write = macchiato_mem_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid =
        {
            .min_access_size = MACCHIATO_MMIO_BAR1_MIN_ACCESS_SIZE,
            .max_access_size = MACCHIATO_MMIO_BAR1_MAX_ACCESS_SIZE,
        },
    .impl =
        {
            .min_access_size = MACCHIATO_MMIO_BAR1_MIN_ACCESS_SIZE,
            .max_access_size = MACCHIATO_MMIO_BAR1_MAX_ACCESS_SIZE,
        },
};
