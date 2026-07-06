#include "mmio.h"

#define MACCHIATO_ACCESS_SIZE 8
#define PAGE_SIZE 4096
#define ROUND_UP_TO_PAGE(x) (((x) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define MACCHIATO_BAR_CTL_SZ ROUND_UP_TO_PAGE(MACCHIATO_REG_CNT * 8)

/* handler for all device operations triggered by writing to MACCHIATO_CMD */
static void macchiato_cmd(MacchiatoState *dev)
{
    macchiato_log("got %s\n", macchiato_op_str(dev->reg[MACCHIATO_REG_CMD]));

    switch (dev->reg[MACCHIATO_REG_CMD]) {

    case MACCHIATO_OP_RESET_COMPILER:
        reset_compiler(dev);
        break;

    case MACCHIATO_OP_RESET_SCANNER:
        reset_scanner(dev);
        break;

    case MACCHIATO_OP_ADD_RULE:
        add_rule(dev);
        break;

    case MACCHIATO_OP_END_SCAN:
        end_scan(dev);
        break;

    case MACCHIATO_OP_GET_RESULTS:
        get_results(dev);
        break;

    /* just notify wrk to call begin_scan */
    case MACCHIATO_OP_BEGIN_SCAN:
        pthread_mutex_lock(&dev->wrk_lock);
        dev->begin_scan = true;
        pthread_cond_signal(&dev->wrk_cond);
        pthread_mutex_unlock(&dev->wrk_lock);
        break;

    case MACCHIATO_OP_CONTINUE_SCAN:
    case MACCHIATO_OP_REWIND_SCAN:
        /* just signals a unlock to the blocked thread, no handling */
        break;
    }
}

static uint64_t macchiato_reg_read(void *opaque, hwaddr addr, unsigned size)
{
    MacchiatoState *dev = opaque;
    unsigned int idx;

    if ((idx = addr / MACCHIATO_ACCESS_SIZE) >= MACCHIATO_REG_CNT) {
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

    if ((idx = addr / MACCHIATO_ACCESS_SIZE) >= MACCHIATO_REG_CNT) {
        macchiato_log("got invalid mmio write: addr=%#" PRIx64 " idx=%u\n",
                      addr, idx);
        return;
    }

    macchiato_log("got mmio write at register %s <- %#" PRIx64 "\n",
                  macchiato_reg_str(idx), data);

    /* special cases */
    switch (idx) {
    case MACCHIATO_REG_CMD:
        dev->reg[MACCHIATO_REG_CMD] = data;
        macchiato_cmd(dev);
        return;

    case MACCHIATO_REG_LOWER_IRQ:
        macchiato_irq_lower(dev, data);
        return;
    }

    dev->reg[idx] = data;
}

void macchiato_mmio_init(MacchiatoState *dev, Error **errp)
{
    memory_region_init_io(&dev->bar[MACCHIATO_BAR_ID_CTL], OBJECT(dev),
                          &macchiato_reg_ops, dev, "macchiato-bar-ctl",
                          MACCHIATO_BAR_CTL_SZ);

    pci_register_bar(&dev->pci_dev, MACCHIATO_BAR_ID_CTL,
                     PCI_BASE_ADDRESS_SPACE_MEMORY,
                     &dev->bar[MACCHIATO_BAR_ID_CTL]);

    macchiato_log("mmio initialized\n");
}

const MemoryRegionOps macchiato_reg_ops = {
    .read = macchiato_reg_read,
    .write = macchiato_reg_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid =
        {
            .min_access_size = MACCHIATO_ACCESS_SIZE,
            .max_access_size = MACCHIATO_ACCESS_SIZE,
        },
    .impl =
        {
            .min_access_size = MACCHIATO_ACCESS_SIZE,
            .max_access_size = MACCHIATO_ACCESS_SIZE,
        },
};
