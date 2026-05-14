/* TODO  */
/*
 * test a payload bigger than internal buffer
 *
 * new ops
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

    /* no need for dbuf_off because its always consumed whole */
    size_t ubuf_off; /* offset inside user buffer */
    size_t file_off; /* offset inside scanned file */

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

static inline size_t min_size(size_t a, size_t b)
{
    return (a < b) ? a : b;
}

static uint8_t fetch_blk(struct iterator_context *ctx)
{
    MacchiatoState *dev = ctx->dev;
    uint64_t ubuf_sz = dev->reg[MACCHIATO_REG_SIZE];

    ctx->block.base = ctx->ubuf_off; // not for the whole file

    macchiato_log("fetch_blk() :: called!\n");

    /* may be good... */
    // assert(ctx->ubuf_off <= ubuf_sz);

    /* got to the end of the ubuf, set it and return */
    if (ctx->file_off >= ubuf_sz) {
        ctx->last = 1;
        return ctx->valid = 0;
    }

    /* TODO does this happen? */
    /* it shouldnt, added a log */
    if (ctx->ubuf_off >= MACCHIATO_DEV_BUF_SZ) {
        macchiato_log(
            "fetch_blk() :: ctx->ubuf_off >= MACCHIATO_DEV_BUF_SZ BAD!\n");
        return ctx->valid = 0;
    }

    /* how much there is left in the file to read
     * TODO: review this */
    uint64_t remaining = ubuf_sz - ctx->file_off;

    /* choose to copy as much as possible */
    ctx->block.size = min_size(remaining, MACCHIATO_DEV_BUF_SZ);

    macchiato_log("fetch_blk() :: ctx=%p\n", ctx);
    macchiato_log("fetch_blk() :: ctx->ubuf_off=%zu\n", ctx->ubuf_off);
    macchiato_log("fetch_blk() :: ctx->block.size=%zu\n", ctx->block.size);

    /* copy it */
    macchiato_log("fetch_blk() :: starting dma read... \n");

    /* check for fail? */
    macchiato_dma_read(dev, dev->reg[MACCHIATO_REG_ADDR] + ctx->ubuf_off,
                       ctx->block.size);
    macchiato_log("finished dma read!\n");

    /* advance the pointers */
    ctx->ubuf_off += ctx->block.size;
    ctx->file_off += ctx->block.size;

    ctx->valid = 1;
    ctx->buf = dev->buffer;

    macchiato_log("fetch_blk() :: returning valid block\n");
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
    if (!scan) {
        return;
    }

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

// T: what else will it return? need this comment? should error handle? faith?
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

    /* not enough space in ubuf */
    if (dev->reg[MACCHIATO_REG_SIZE] < src->serialsz) {
        return -1;
    }

    /* write the linked list "head" (scan) */
    off += write_serialized_scan(dev, dest, src);
    r = src->head;

    while (r && off + r->serialsz <= src->serialsz) {
        /* write a component of the list (rule) */
        off += write_serialized_rule(dev, dest + off, r);

        /* go for the next */
        r = r->next;
    }

    return 0;
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
 *         -EAGAIN if asked for results */
static int get_next_buf(struct iterator_context *ctx)
{
    MacchiatoState *dev = ctx->dev;
    macchiato_log("get_next_buf() :: asking for a new user buf\n");

    /* report total findings size */
    dev->reg[MACCHIATO_REG_SIZE] = dev->scan->serialsz;

    macchiato_irq_raise(dev, MACCHIATO_IRQ_NEXT_BLOCK);
    wait_for_irq(dev, MACCHIATO_IRQ_NEXT_BLOCK);

    if (dev->reg[MACCHIATO_REG_CMD] == MACCHIATO_OP_GET_RESULTS) {
        macchiato_log(
            "get_next_buf() :: user asked for results, returning...\n");
        return -EAGAIN;
    }

    macchiato_log("get_next_buf() :: user refilled buffer, ok\n");
    return 0;
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

    macchiato_log("fetch_next_blk() :: fetching block and returning it...\n");
    return fetch_blk(ctx);
}

/* ask the driver for a block, copy to device's internal buffer via dma, update
 * iterator context accordingly */
static uint8_t fetch_first_blk(struct iterator_context *ctx)
{
    MacchiatoState *dev = ctx->dev;

    macchiato_log("fetch_first_blk() :: called!\n");

    /* if not on the first block ask for it */
    if (ctx->file_off > 0) {
        /* report total findings size */
        dev->reg[MACCHIATO_REG_SIZE] = dev->scan->serialsz;

        macchiato_irq_raise(dev, MACCHIATO_IRQ_FIRST_BLOCK);
        wait_for_irq(dev, MACCHIATO_IRQ_FIRST_BLOCK);
    }

    ctx->ubuf_off = 0; /* go to the start of user buffer */
    ctx->file_off = 0; /* go to the start of file (scan) */
    macchiato_log(
        "fetch_first_blk() :: reset pointers (file_off & ubuf_off)\n");

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

/* okay */
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

/* should return error no? */
void reset_scanner(MacchiatoState *dev)
{

    macchiato_log("scanner reset called!\n");

    /* free stuff */
    destroy_scanner(dev);

    /* setup it back up */
    if (setup_scanner(dev) < 0) {
        return;
    }

    /* reset iterctx and friends */
    iterctx.dev = dev;
    iterctx.last = 0;
    iterctx.valid = 1;
    iterctx.new_scan = 1;
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

/* TODO won't work with userspace buffered scan */
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

/* okay */
void get_results(MacchiatoState *dev)
{
    if (machyara_serialize(dev) < 0) {
        macchiato_log("failed serialize results: dont fit\n");
        macchiato_irq_raise(dev, MACCHIATO_IRQ_FAILED);
        return;
    }

    macchiato_irq_raise(dev, MACCHIATO_IRQ_DONE);
}
