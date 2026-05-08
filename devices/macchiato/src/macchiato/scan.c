/* TODO  */
/*
 * test a payload bigger than internal buffer
 *
 * review dmaoff and totaloff,, check buffered read stuff
 * */

#include "scan.h"

/* types  ------------------------------------------------------------------  */

struct iterator_context {
    uint8_t new_scan; /* scan_buffer callback aux variable used to know if we
                         need to start from a new scan from scratch or if we are
                         continuing a scan  */

    uint8_t valid; /* 0 = consumed (already scanned or just invalid) */
    uint8_t last;  /* 1 = last block for the scan, don't ask driver for more! */
    uint8_t *buf;  /* ptr to device internal dma buffer */

    size_t dmaoff;   /* offset inside user buffer */
    size_t totaloff; /* offset inside scanned file */

    MacchiatoState *dev;
    YR_MEMORY_BLOCK block;
};

/* serialization stuff  */

struct machyara_rule {
    uint64_t serialsz; /* serialized size of this rule */
    uint8_t matched;   /* zero -> not matched, not zero -> matched */
    struct machyara_rule *next;
    char identifier[]; /* rule name string */
};

struct machyara_scan {
    uint64_t serialsz; /* serialized size of rule chain (dest buffer size) */
    struct machyara_rule *head;
    struct machyara_rule *tail;
};

static const uint8_t *get_blk_addr(YR_MEMORY_BLOCK *self);
static YR_MEMORY_BLOCK *get_first_blk(YR_MEMORY_BLOCK_ITERATOR *self);
static YR_MEMORY_BLOCK *get_next_blk(YR_MEMORY_BLOCK_ITERATOR *self);

/* -------------------------------------------------------------------------  */
/* static variables  -------------------------------------------------------  */

static struct iterator_context iterctx = {.block = {
                                              .context = &iterctx,
                                              .fetch_data = get_blk_addr,
                                          }};

static YR_MEMORY_BLOCK_ITERATOR iterator = {
    .context = &iterctx,
    .first = get_first_blk,
    .next = get_next_blk,
};

/* -------------------------------------------------------------------------  */
/* helpers  ----------------------------------------------------------------  */

static uint8_t fetch_blk(struct iterator_context *ctx)
{
    MacchiatoState *dev = ctx->dev;
    ctx->block.base = ctx->dmaoff; // not for the whole file

    macchiato_log("fetch_blk() ...\n");

    if (ctx->totaloff >= dev->reg[MACCHIATO_REG_SIZE]) {
        ctx->last = 1;
        return ctx->valid = 0;
    }

    /* TODO does this happen?  */
    if (ctx->dmaoff >= MACCHIATO_INTERNAL_BUFFER) {
        return ctx->valid = 0;
    }

    // figure out how much to copy
    if (dev->reg[MACCHIATO_REG_SIZE] - ctx->totaloff <
        MACCHIATO_INTERNAL_BUFFER) {
        ctx->block.size = dev->reg[MACCHIATO_REG_SIZE] - ctx->dmaoff;
    } else {
        ctx->block.size = MACCHIATO_INTERNAL_BUFFER;
    }

    macchiato_log("fetch_blk() :: ctx=%p\n", ctx);
    macchiato_log("fetch_blk() :: ctx->dmaoff=%zu\n", ctx->dmaoff);
    macchiato_log("fetch_blk() :: ctx->block.size=%zu\n", ctx->block.size);

    // copy
    macchiato_log("fetch_blk() :: starting dma read... \n");
    macchiato_dma_read(dev, dev->reg[MACCHIATO_REG_ADDR] + ctx->dmaoff,
                       ctx->block.size);
    macchiato_log("done\n");

    ctx->dmaoff += ctx->block.size;
    ctx->totaloff += ctx->block.size;

    ctx->valid = 1;
    ctx->buf = dev->buffer;

    macchiato_log("fetch_blk() :: returning a valid block\n");

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

    if (dev->reg[MACCHIATO_REG_SIZE] < src->serialsz) {
        return -1;
    }

    off += write_serialized_scan(dev, dest + off, src);
    r = src->head;

    while (r && off + r->serialsz <= src->serialsz) {
        off += write_serialized_rule(dev, dest + off, r);
        r = r->next;
    }

    return 0;
}

/* callbacks ----------------------------------------------------------------*/

static int scanner_callback(YR_SCAN_CONTEXT *context, int message,
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
        macchiato_log("scan finished\n");
        break;

    case CALLBACK_MSG_CONSOLE_LOG:
        macchiato_log("console log: %s\n", (char *)message_data);
        break;
    }

    if (identifier) {
        machyara_scan_add_rule(dev->scan, identifier, matched);
        macchiato_log("rule=%s -> matched=%u\n", identifier, matched);
    }

    return CALLBACK_CONTINUE;
}

/* ask the driver for a block, copy to device's internal buffer via dma, update
 * iterator context accordingly */
static uint8_t fetch_next_blk(struct iterator_context *ctx)
{
    MacchiatoState *dev = ctx->dev;

    macchiato_log("fetch nextblock called\n");
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

    macchiato_log("fetch next block returnidndnidn\n");
    return fetch_blk(ctx);
}

/* ask the driver for a block, copy to device's internal buffer via dma, update
 *  * iterator context accordingly */
static uint8_t fetch_first_blk(struct iterator_context *ctx)
{
    MacchiatoState *dev = ctx->dev;

    macchiato_log("fetch first block called\n");
    /*if  not on  the first blk fetch it */
    if (ctx->totaloff >= dev->reg[MACCHIATO_REG_SIZE]) {
        // report total findings size
        dev->reg[MACCHIATO_REG_SIZE] = dev->scan->serialsz;

        macchiato_irq_raise(dev, MACCHIATO_IRQ_FIRST_BLOCK);
        wait_for_irq(dev, MACCHIATO_IRQ_FIRST_BLOCK);
    }

    ctx->dmaoff = 0;   // go to the beggining of user buffer
    ctx->totaloff = 0; // go to the beggining of scan

    macchiato_log("fetch first block returnidndnidn\n");
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

    macchiato_log("get first block called\n");

    if (ctx->valid || (!ctx->last && fetch_first_blk(ctx))) {
        ctx->valid = 0;
        return &ctx->block;
    }
    macchiato_log("get first block returning null\n");

    return NULL;
}

/* okay  */
static YR_MEMORY_BLOCK *get_next_blk(YR_MEMORY_BLOCK_ITERATOR *self)
{
    struct iterator_context *ctx = self->context;

    macchiato_log("get next block called\n");

    if (ctx->valid || (!ctx->last && fetch_next_blk(ctx))) {
        ctx->valid = 0;
        macchiato_log("get next block returning valid block\n");
        return &ctx->block;
    }

    macchiato_log("get next block returning null\n");

    return NULL;
}

/* operations ---------------------------------------------------------------*/

/* okay  */
void reset_compiler(MacchiatoState *dev)
{
    if (dev->compiler) {
        yr_compiler_destroy(dev->compiler);
    }

    if (yr_compiler_create(&dev->compiler)) {
        macchiato_log("failed to create yara compiler\n");
        macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
        return;
    }

    macchiato_log("compiler reset\n");
    macchiato_irq_raise(dev, MACCHIATO_IRQ_DONE);

    return;
}

void reset_scanner(MacchiatoState *dev)
{

    /* free stuff  */

    if (dev->scanner) {
        yr_scanner_destroy(dev->scanner);
        dev->scanner = NULL;
    }

    if (dev->rules) {
        yr_rules_destroy(dev->rules);
        dev->rules = NULL;
    }

    if (dev->scan) {
        machyara_scan_destroy(dev->scan);
        dev->scan = NULL;
    }

    /* check prerequisites  */

    if (!dev->compiler) {
        macchiato_log("failed to reset scanner: compiler is null\n");
        goto err_no_compiler;
    }

    /* setup stuff */

    if (!(dev->scan = machyara_scan_create())) {
        macchiato_log(
            "failed to reset scanner: unable to allocate results struct\n");
        goto err_scan_create;
    }

    if (yr_compiler_get_rules(dev->compiler, &dev->rules)) {
        macchiato_log("failed to setup scanner: unable to get rules\n");
        goto err_get_rules;
    }

    if (yr_scanner_create(dev->rules, &dev->scanner)) {
        macchiato_log("failed to setup scanner: unable to create scanner\n");
        goto err_scanner_create;
    }

    yr_scanner_set_callback(dev->scanner, scanner_callback, NULL);

    /* TODO reset iterctx and friends  */
    iterctx.dev = dev;
    iterctx.new_scan = 1;

    macchiato_log("scanner reset\n");
    macchiato_irq_raise(dev, MACCHIATO_IRQ_DONE);

    return;

err_scanner_create:
    yr_rules_destroy(dev->rules);
    dev->rules = NULL;

err_get_rules:
    machyara_scan_destroy(dev->scan);
    dev->scan = NULL;

err_scan_create:
err_no_compiler:

    macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
    return;
}

/* okay  */
void add_rule(MacchiatoState *dev)
{
    size_t size = dev->reg[MACCHIATO_REG_SIZE];
    hwaddr addr = dev->reg[MACCHIATO_REG_ADDR];

    if (macchiato_dma_read(dev, addr, size)) {
        macchiato_log("failed to add rule: dma read failed\n");
        goto raise_failed;
    }

    /* dma read ensures enough space + 1 */
    dev->buffer[size] = 0; /* null terminator for str  */

    macchiato_log("rulestr='%s'\n", (char *)dev->buffer);

    if (yr_compiler_add_string(dev->compiler, (char *)dev->buffer, NULL)) {
        macchiato_log("failed to add rule: yr_compiler_add_string failed\n");
        goto raise_failed;
    }

    macchiato_irq_raise(dev, MACCHIATO_IRQ_DONE);
    return;

raise_failed:
    macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
    return;
}

/* TODO won't work with userspace buffered scan  */
void scan_buffer(MacchiatoState *dev)
{
    if (!iterctx.new_scan) {
        return;
    }

    iterctx.new_scan = 0;

    if (yr_scanner_scan_mem_blocks(dev->scanner, &iterator)) {
        iterctx.new_scan = 1;
        macchiato_log("failed to scan buffer: scan failed\n");
        macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
        return;
    }

    macchiato_log("scan finished\n");
    macchiato_irq_raise(dev, MACCHIATO_IRQ_DONE);
    return;
}

/* okay  */
void get_results(MacchiatoState *dev)
{
    if (machyara_serialize(dev) < 0) {
        macchiato_log("failed serialize results: dont fit\n");
        macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
        return;
    }

    macchiato_irq_raise(dev, MACCHIATO_IRQ_DONE);
}
