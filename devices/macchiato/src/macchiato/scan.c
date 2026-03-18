#include "scan.h"

static int macchiato_yara_callback(YR_SCAN_CONTEXT *context, int message,
                                   void *message_data, void *user_data);
static const uint8_t *get_blk_addr(YR_MEMORY_BLOCK *self);
static YR_MEMORY_BLOCK *get_first_blk(YR_MEMORY_BLOCK_ITERATOR *self);
static YR_MEMORY_BLOCK *get_next_blk(YR_MEMORY_BLOCK_ITERATOR *self);

static struct iterator_context iterctx = {.block = {
                                              .context = &iterctx,
                                              .fetch_data = get_blk_addr,
                                          }};

static YR_MEMORY_BLOCK_ITERATOR iterator = {
    .context = &iterctx,
    .first = get_first_blk,
    .next = get_next_blk,
};

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

static uint8_t fetch_blk(struct iterator_context *ctx)
{
    MacchiatoState *dev = ctx->dev;
    ctx->block.base = ctx->dmaoff; // not for the whole file

    // figure out how much to copy
    if (dev->reg[MACCHIATO_REG_SIZE] - ctx->dmaoff <
        MACCHIATO_INTERNAL_BUFFER) {
        ctx->block.size = dev->reg[MACCHIATO_REG_SIZE] - ctx->dmaoff;
    } else {
        ctx->block.size = MACCHIATO_INTERNAL_BUFFER;
    }

    // copy
    macchiato_dma_read(dev, dev->reg[MACCHIATO_REG_ADDR] + ctx->dmaoff,
                       ctx->block.size);

    ctx->dmaoff += ctx->block.size;
    ctx->totaloff += ctx->block.size;

    ctx->valid = 1;
    ctx->buf = dev->buffer;

    return ctx->valid;
}

/* results logging ----------------------------------------------------------*/

static struct machyara_scan *machyara_scan_create(void)
{
    struct machyara_scan *s;

    if (!(s = calloc(1, sizeof *s))) {
        return NULL;
    }

    s->serialsz = sizeof s->serialsz;

    return s;
}

static void machyara_scan_destroy(struct machyara_scan *scan)
{
    struct machyara_rule *r;

    while ((r = scan->head)) {
        scan->head = r->next;
        free(r);
    }

    free(scan);
}

static struct machyara_rule *rule_create(const char *identifier,
                                         uint8_t matched)
{
    struct machyara_rule *r;
    size_t fullsz;

    /* strlen + 1 to account for the null terminator */
    fullsz = sizeof *r + strlen(identifier) + 1;

    if (!(r = calloc(1, fullsz))) {
        return NULL;
    }

    r->serialsz = sizeof r->serialsz + sizeof r->matched + fullsz - sizeof *r;
    r->matched = matched;
    strcpy(r->identifier, identifier);

    return r;
}

static int machyara_scan_add_rule(struct machyara_scan *scan,
                                  const char *identifier, uint8_t matched)
{
    struct machyara_rule *r;

    if (!(r = rule_create(identifier, matched))) {
        return -ENOMEM;
    }

    if (!scan->head) {
        scan->head = scan->tail = r;
    } else {
        scan->tail = scan->tail->next = r;
    }

    scan->serialsz += r->serialsz;

    return 0;
}

/* results serialization ----------------------------------------------------*/

static size_t write_serialized_scan(MacchiatoState *dev, hwaddr dest,
                                    struct machyara_scan *src)
{
    macchiato_dma_write(dev, dest, &src->serialsz, sizeof src->serialsz);
    return sizeof src->serialsz;
}

/* caller: check if return value is equal to src->serialsz */
static size_t write_serialized_rule(MacchiatoState *dev, hwaddr dest,
                                    struct machyara_rule *src)
{
    size_t off = 0;

    macchiato_dma_write(dev, dest + off, &src->serialsz, sizeof src->serialsz);
    off += sizeof src->serialsz;

    macchiato_dma_write(dev, dest + off, &src->matched, sizeof src->matched);
    off += sizeof src->matched;

    macchiato_dma_write(dev, dest + off, src->identifier, src->serialsz - off);

    return src->serialsz;
}

static int machyara_serialize(MacchiatoState *dev)
{
    struct machyara_scan *src;
    struct machyara_rule *r;
    size_t off;
    hwaddr dest;

    src = dev->scan;
    dest = dev->reg[MACCHIATO_REG_ADDR];
    off = 0;

    off += write_serialized_scan(dev, dest + off, src);
    r = src->head;

    while (r && off + r->serialsz <= src->serialsz) {
        off += write_serialized_rule(dev, dest + off, r);
        r = r->next;
    }

    return 0;
}

/* callbacks ----------------------------------------------------------------*/

static int macchiato_yara_callback(YR_SCAN_CONTEXT *context, int message,
                                   void *message_data, void *user_data)
{
    const char *identifier = NULL;
    uint8_t matched = 0;

    MacchiatoState *dev =
        ((struct iterator_context *)context->iterator->context)->dev;

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
        machyara_scan_add_rule(dev->scan, identifier, matched);
        printf("rule=%s -> matched=%u\n", identifier, matched);
    }

    return CALLBACK_CONTINUE;
}

/* ask the driver for a block, copy to device's internal buffer via dma, update
 *  * iterator context accordingly */
static uint8_t fetch_next_blk(struct iterator_context *ctx)
{
    MacchiatoState *dev = ctx->dev;

    // if consumed the whole user buffer ask for more
    if (ctx->dmaoff >= dev->reg[MACCHIATO_REG_SIZE]) {

        // report total findings size
        dev->reg[MACCHIATO_REG_SIZE] = dev->scan->serialsz;

        macchiato_irq_raise(dev, MACCHIATO_IRQ_NEXT_BLOCK);
        wait_for_irq(dev, MACCHIATO_IRQ_NEXT_BLOCK);

        // check if continue scan or return results
        if (dev->reg[MACCHIATO_REG_CMD] == MACCHIATO_OP_GET_RESULTS) {
            ctx->valid = 0;
            ctx->last = 1;
            return ctx->valid;
        }

        ctx->dmaoff = 0; // go to the beggining of user buffer
    }

    return fetch_blk(ctx);
}

/* ask the driver for a block, copy to device's internal buffer via dma, update
 *  * iterator context accordingly */
static uint8_t fetch_first_blk(struct iterator_context *ctx)
{
    MacchiatoState *dev = ctx->dev;

    /*if  not on  the first blk fetch it */
    if (ctx->totaloff >= dev->reg[MACCHIATO_REG_SIZE]) {
        // report total findings size
        dev->reg[MACCHIATO_REG_SIZE] = dev->scan->serialsz;

        macchiato_irq_raise(dev, MACCHIATO_IRQ_FIRST_BLOCK);
        wait_for_irq(dev, MACCHIATO_IRQ_FIRST_BLOCK);
    }

    ctx->dmaoff = 0;   // go to the beggining of user buffer
    ctx->totaloff = 0; // go to the beggining of scan

    return fetch_blk(ctx);
}

/* yara calls this function once to get the memory block's first address */
static const uint8_t *get_blk_addr(YR_MEMORY_BLOCK *self)
{
    struct iterator_context *ctx = self->context;
    return ctx->buf;
}

static YR_MEMORY_BLOCK *get_first_blk(YR_MEMORY_BLOCK_ITERATOR *self)
{
    struct iterator_context *ctx = self->context;

    if (ctx->valid || (!ctx->last && fetch_first_blk(ctx))) {
        ctx->valid = 0;
        return &ctx->block;
    }

    return NULL;
}

static YR_MEMORY_BLOCK *get_next_blk(YR_MEMORY_BLOCK_ITERATOR *self)
{
    struct iterator_context *ctx = self->context;

    if (ctx->valid || (!ctx->last && fetch_next_blk(ctx))) {
        ctx->valid = 0;
        return &ctx->block;
    }

    return NULL;
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
        machyara_scan_destroy(dev->scan);
    }

    if (!(dev->scan = machyara_scan_create())) {
        macchiato_log(
            "failed to reset scanner: unable to allocate machyara struct\n");
        macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
    }

    macchiato_log("scanner reset\n");
    macchiato_irq_raise(dev, MACCHIATO_IRQ_DONE);

    return;
}

void add_rule(MacchiatoState *dev)
{
    size_t size = dev->reg[MACCHIATO_REG_SIZE];
    hwaddr addr = dev->reg[MACCHIATO_REG_ADDR];

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

    iterctx.dev = dev;

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

void get_results(MacchiatoState *dev)
{
    machyara_serialize(dev);
}
