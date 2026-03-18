#include "scan.h"

/*
static const uint8_t *get_blk_addr(YR_MEMORY_BLOCK *self);
static YR_MEMORY_BLOCK *get_first_blk(YR_MEMORY_BLOCK_ITERATOR *self);
static YR_MEMORY_BLOCK *get_next_blk(YR_MEMORY_BLOCK_ITERATOR *self);

static struct iterator_context iterctx = {
  .block = {
    .context = &iterctx,
    .fetch_data = get_blk_addr,
  }
};

static YR_MEMORY_BLOCK_ITERATOR iterator = {
    .context = &iterctx,
    .first = get_first_blk,
    .next = get_next_blk,
};
*/

void reset_compiler(MacchiatoState *dev)
{
    if (dev->compiler) {
        yr_compiler_destroy(dev->compiler);
    }

    if (yr_compiler_create(&dev->compiler)) {
        macchiato_log("failed to create yara compiler");
        macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
        return;
    }

    macchiato_log("compiler reset\n");
    macchiato_irq_raise(dev, MACCHIATO_IRQ_DONE);

    return;
}

void reset_scanner(MacchiatoState *dev)
{
    if (dev->scanner) {
        yr_scanner_destroy(dev->scanner);
    }

    macchiato_log("scanner reset\n");
    macchiato_irq_raise(dev, MACCHIATO_IRQ_DONE);

    return;
}

void add_rule(MacchiatoState *dev, hwaddr addr, size_t size)
{
    if (macchiato_dma_read(dev, addr, size)) {
        macchiato_log("failed to add rule: unable to fetch rule\n");
        goto failed;
    }

    /* dma read ensures enough space */
    dev->buffer[size] = 0;

    if (yr_compiler_add_string(dev->compiler, (char *)dev->buffer, NULL)) {
        macchiato_log("failed to add rule: unable to add string to compiler\n");
        goto failed;
    }

    macchiato_irq_raise(dev, MACCHIATO_IRQ_DONE);
    return;

failed:
    macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
    return;
}

/*
void scan_buffer(MacchiatoState *dev, hwaddr addr, size_t size) {

}
*/

// void get_results();
