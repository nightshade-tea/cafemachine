#include "scan.h"

static int macchiato_yara_callback(YR_SCAN_CONTEXT *context, int message,
                                   void *message_data, void *user_data);

static YR_MEMORY_BLOCK_ITERATOR iterator;
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

/* helpers ----------------------------------------------------------------*/

static int setup_scanner(MacchiatoState *dev)
{
    if (yr_compiler_get_rules(dev->compiler, &dev->rules)) {
        macchiato_log("failed to setup scanner: unable to get rules\n");
        return 1;
    }

    if (yr_scanner_create(dev->rules, &dev->scanner)) {
        macchiato_log("failed to setup scanner: unable to create scanner\n");
        return 1;
    }

    yr_scanner_set_callback(dev->scanner, macchiato_yara_callback, NULL);

    return 0;
}

/* callbacks ----------------------------------------------------------------*/

static int macchiato_yara_callback(YR_SCAN_CONTEXT *context, int message,
                                   void *message_data, void *user_data)
{
    const char *identifier = NULL;
    uint8_t matched = 0;

    switch (message) {
    case CALLBACK_MSG_RULE_MATCHING:
        matched = 1;
        /* fall through */

    case CALLBACK_MSG_RULE_NOT_MATCHING:
        identifier = ((YR_RULE *)message_data)->identifier;
        break;

    case CALLBACK_MSG_SCAN_FINISHED:
        puts("scan finished");
        break;

    case CALLBACK_MSG_CONSOLE_LOG:
        printf("console log: %s\n", (char *)message_data);
        break;
    }

    if (identifier) {
        // machyara_scan_add_rule(scan, identifier, matched);
        printf("rule=%s -> matched=%u\n", identifier, matched);
    }

    return CALLBACK_CONTINUE;
}

/* operations ---------------------------------------------------------------*/

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

void scan_buffer(MacchiatoState *dev)
{
    if (setup_scanner(dev)) {
        macchiato_log("failed to scan buffer: unable to setup scanner");
        goto failed;
    }

    if (yr_scanner_scan_mem_blocks(dev->scanner, &iterator)) {
        macchiato_log("failed to scan buffer: scan failed");
        goto failed;
    }

    macchiato_log("scan finished\n");
    macchiato_irq_raise(dev, MACCHIATO_IRQ_DONE);
failed:
    macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
    return;
}

// void get_results();
