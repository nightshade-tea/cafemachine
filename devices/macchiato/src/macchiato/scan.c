/* TODO  */
/*
 * test a payload bigger than internal buffer
 *
 * new ops
 * */

#include "scan.h"

/* types  ------------------------------------------------------------------  */

struct iterator_context {

    uint8_t valid; /* 0 = consumed (already scanned or just invalid) */
    uint8_t last;  /* 1 = last block for the scan, don't ask driver for more! */
    uint8_t *buf;  /* ptr to device internal dma buffer */

    /* no need for dbuf_off because its always consumed whole */
    size_t ubuf_off; /* offset inside user buffer */
    size_t file_off; /* offset inside scanned file */

    MacchiatoState *dev;
    YR_MEMORY_BLOCK block;
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
    uint64_t ubuf_sz = dev->reg[MACCHIATO_REG_SIZE];

    ctx->block.base = ctx->file_off;

    macchiato_log("fetch_blk() :: called!\n");

    /* got to the end of the ubuf, set it and return */
    if (ctx->ubuf_off >= ubuf_sz) {
        ctx->last = 1;
        return ctx->valid = 0;
    }

    /* how much there is left in the userbuf to read */
    uint64_t remaining = ubuf_sz - ctx->ubuf_off;

    /* choose to copy as much as possible */
    ctx->block.size = MIN(remaining, MACCHIATO_DEV_BUF_SZ);

    macchiato_log("fetch_blk() :: ctx=%p\n", ctx);
    macchiato_log("fetch_blk() :: ctx->ubuf_off=%zu\n", ctx->ubuf_off);
    macchiato_log("fetch_blk() :: ctx->block.size=%zu\n", ctx->block.size);

    macchiato_log("fetch_blk() :: starting dma read... \n");

    /* copy it */
    if (macchiato_dma_read(dev, dev->reg[MACCHIATO_REG_ADDR] + ctx->ubuf_off,
                           ctx->block.size) != MEMTX_OK) {
        macchiato_log("fetch_blk() :: dma read failed!\n");
        macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
        return ctx->valid = 0;
    }
    macchiato_log("fetch_blk() :: finished dma read!\n");

    /* advance the pointers */
    ctx->ubuf_off += ctx->block.size;
    ctx->file_off += ctx->block.size;

    ctx->valid = 1;
    ctx->buf = dev->buffer;

    macchiato_log("fetch_blk() :: returning valid block\n");
    return ctx->valid;
}

/* callbacks ----------------------------------------------------------------*/

static int scanner_callback(YR_SCAN_CONTEXT *ctx, int msg, void *msg_data,
                            void *user_data)
{
    const char *identifier = NULL;
    uint8_t matched = 0;

    MacchiatoState *dev =
        ((struct iterator_context *)ctx->iterator->context)->dev;

    switch (msg) {
    case CALLBACK_MSG_RULE_MATCHING:
        matched = 1;
        /* fall through */

    case CALLBACK_MSG_RULE_NOT_MATCHING:
        identifier = ((YR_RULE *)msg_data)->identifier;
        break;

    case CALLBACK_MSG_SCAN_FINISHED:
        sem_post(&dev->scan_finished);
        macchiato_log("scan finished!\n");
        break;

    case CALLBACK_MSG_CONSOLE_LOG:
        macchiato_log("console log: %s\n", (char *)msg_data);
        break;
    }

    if (identifier) {
        machyara_scan_add_rule(dev->scan, identifier, matched);
        macchiato_log("rule=%s -> matched=%u\n", identifier, matched);
    }

    return CALLBACK_CONTINUE;
}

/* returns  0 if got new user buf
 *          ESHUTDOWN if asked for results
 *         -EINVAL if received cmd is not supported */
static int get_next_buf(struct iterator_context *ctx)
{
    MacchiatoState *dev = ctx->dev;
    macchiato_log("get_next_buf() :: asking for a new user buf\n");

    macchiato_irq_raise(dev, MACCHIATO_IRQ_NEXT_BUF);
    wait_for_irq(dev, MACCHIATO_IRQ_NEXT_BUF);

    switch (dev->reg[MACCHIATO_REG_CMD]) {
    case MACCHIATO_OP_CONTINUE_SCAN:
        macchiato_log("get_next_buf() :: user refilled buffer, ok\n");
        return 0;

    case MACCHIATO_OP_END_SCAN:
        macchiato_log("get_next_buf() :: user asked for end, returning...\n");
        return -ESHUTDOWN;

    default:
        macchiato_log("get_next_buf() :: user called invalid cmd (%s)...\n",
                      macchiato_op_str(dev->reg[MACCHIATO_REG_CMD]));
        macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
        return -EINVAL;
    }
}

/* ubuf_off aware */
static uint8_t fetch_next_blk(struct iterator_context *ctx)
{
    MacchiatoState *dev = ctx->dev;
    macchiato_log("fetch_next_blk() :: called!\n");

    /* if consumed the whole user buffer ask for more */
    if (ctx->ubuf_off >= dev->reg[MACCHIATO_REG_SIZE]) {
        if (get_next_buf(ctx) < 0) {
            /* scan done */
            ctx->last = 1;
            return ctx->valid = 0;
        }

        ctx->ubuf_off = 0;
    }

    macchiato_log("fetch_next_blk() :: fetching block...\n");

    return fetch_blk(ctx);
}

static int get_first_buf(struct iterator_context *ctx)
{
    MacchiatoState *dev = ctx->dev;
    macchiato_log("get_first_buf() :: asking for a new user buf\n");

    macchiato_irq_raise(dev, MACCHIATO_IRQ_FIRST_BUF);
    wait_for_irq(dev, MACCHIATO_IRQ_FIRST_BUF);

    switch (dev->reg[MACCHIATO_REG_CMD]) {
    case MACCHIATO_OP_REWIND_SCAN:
        macchiato_log("get_first_buf() :: user rewinded buf, nice!\n");
        return 0;

    case MACCHIATO_OP_END_SCAN:
        macchiato_log("get_first_buf() :: user asked for end, returning...\n");
        return -ESHUTDOWN;

    default:
        macchiato_log("get_first_buf() :: user called invalid cmd...\n");
        macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
        return -EINVAL;
    }
}

/* ask the driver for a block, copy to device's internal buffer via dma, update
 * iterator context accordingly */
static uint8_t fetch_first_blk(struct iterator_context *ctx)
{
    macchiato_log("fetch_first_blk() :: called!\n");

    /* if not on the first ubuf ask for it */
    if (ctx->file_off > ctx->ubuf_off) {
        if (get_first_buf(ctx) < 0) {
            /* scan done */
            ctx->last = 1;
            return ctx->valid = 0;
        }
    }

    ctx->ubuf_off = 0; /* go to the start of user buffer */
    ctx->file_off = 0; /* go to the start of file (scan) */

    macchiato_log("fetch_first_blk() :: fetching block and returning it...\n");
    return fetch_blk(ctx);
}

/* yara calls this function once to get the memory block's first address */
static const uint8_t *get_blk_addr(YR_MEMORY_BLOCK *self)
{
    struct iterator_context *ctx = self->context;
    return ctx->buf;
}

static YR_MEMORY_BLOCK *get_blk(YR_MEMORY_BLOCK_ITERATOR *self,
                                uint8_t (*fetch)(struct iterator_context *))
{
    struct iterator_context *ctx = self->context;

    /* there was a if here checking if ctx->valid and returning it
     * do we want that? it also should never happen, so removed it. */

    if (!ctx->last) {
        uint8_t got_valid = fetch(ctx);
        if (got_valid) {
            ctx->valid = 0;
            return &ctx->block;
        }
    }

    /* if current is not valid nor the last, there
     * are no more blocks to get, so just return NULL */
    return NULL;
}

static YR_MEMORY_BLOCK *get_first_blk(YR_MEMORY_BLOCK_ITERATOR *self)
{
    /* if we want first, last should not matter
     * if there is no first, fetch returns NULL */
    struct iterator_context *ctx = self->context;
    ctx->last = 0;
    return get_blk(self, fetch_first_blk);
}

static YR_MEMORY_BLOCK *get_next_blk(YR_MEMORY_BLOCK_ITERATOR *self)
{
    return get_blk(self, fetch_next_blk);
}

/* operations ---------------------------------------------------------------*/

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

static void destroy_scanner(MacchiatoState *dev)
{
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
}

/* returns 0 if setup ok
 *        -1 if error, and nothing was done */
static int setup_scanner(MacchiatoState *dev)
{
    /* check prerequisites  */

    if (!dev->compiler) {
        macchiato_log("failed to setup scanner: compiler is null\n");
        goto err_no_compiler;
    }

    /* create stuff */

    if (!(dev->scan = machyara_scan_create())) {
        macchiato_log(
            "failed to setup scanner: unable to allocate results struct\n");
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

    return 0;

err_scanner_create:
    yr_rules_destroy(dev->rules);
    dev->rules = NULL;

err_get_rules:
    machyara_scan_destroy(dev->scan);
    dev->scan = NULL;

err_scan_create:
err_no_compiler:

    macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
    return -1;
}

/* just destroy and reset
 * begin_scan creates a new scanner and consumes it */
void reset_scanner(MacchiatoState *dev)
{
    macchiato_log("scanner reset called!\n");

    /* free stuff */
    destroy_scanner(dev);

    /* reset iterctx and friends */
    iterctx.dev = dev;
    iterctx.last = 0;
    iterctx.valid = 1;
    iterctx.ubuf_off = 0;
    iterctx.file_off = 0;

    macchiato_log("scanner reset\n");
    macchiato_irq_raise(dev, MACCHIATO_IRQ_DONE);

    return;
}

/* okay */
void add_rule(MacchiatoState *dev)
{
    size_t ubuf_sz = dev->reg[MACCHIATO_REG_SIZE];
    hwaddr ubuf_addr = dev->reg[MACCHIATO_REG_ADDR];

    if (macchiato_dma_read(dev, ubuf_addr, ubuf_sz)) {
        macchiato_log("failed to add rule: dma read failed\n");
        goto raise_failed;
    }

    /* dma read ensures enough space + 1 */
    dev->buffer[ubuf_sz] = 0; /* null terminator for str */

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

void begin_scan(MacchiatoState *dev)
{
    int err;

    if (setup_scanner(dev) < 0) {
        return; /* already raised IRQ_FAILED */
    }

    if ((err = yr_scanner_scan_mem_blocks(dev->scanner, &iterator))) {
        macchiato_log(
            "failed to scan buffer: yr_scanner_scan_mem_blocks failed err=%d\n",
            err);
        macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
        return;
    }

    macchiato_log(
        "scan finished :: yr_scanner_scan_mem_blocks returned success\n");
}

void end_scan(MacchiatoState *dev)
{
    macchiato_log("end_scan() :: waiting for mutex\n");
    sem_wait(&dev->scan_finished);
    dev->reg[MACCHIATO_REG_SIZE] = dev->scan->serialsz;
    macchiato_irq_raise(dev, MACCHIATO_IRQ_DONE);
    macchiato_log("end_scan() :: done\n");
}

void get_results(MacchiatoState *dev)
{
    macchiato_log("get_results() :: called\n");

    if (MACCHIATO_DEV_BUF_SZ < dev->scan->serialsz) {
        macchiato_log(
            "get_results() :: serialized scan does not fit internal buf\n");
        goto raise_failed;
    }

    if (dev->reg[MACCHIATO_REG_SIZE] < dev->scan->serialsz) {
        macchiato_log(
            "get_results() :: serialized scan does not fit user buf\n");
        goto raise_failed;
    }

    if (machyara_serialize(dev->buffer, dev->scan) < 0) {
        macchiato_log("get_results() :: machyara_serialize() failed\n");
        goto raise_failed;
    }

    if (macchiato_dma_write(dev, dev->reg[MACCHIATO_REG_ADDR], dev->buffer,
                            dev->scan->serialsz) != MEMTX_OK) {
        macchiato_log("get_results() :: macchiato_dma_write() failed\n");
        goto raise_failed;
    }

    macchiato_irq_raise(dev, MACCHIATO_IRQ_DONE);
    macchiato_log("get_results() :: done\n");

    return;

raise_failed:
    macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
    return;
}
