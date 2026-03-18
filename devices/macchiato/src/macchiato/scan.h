#ifndef MACCHIATO_SCAN_H
#define MACCHIATO_SCAN_H

#include "dma.h"
#include "irq.h"
#include "macchiato.h"
#include "macchiato_device.h"
#include "macchiato_log.h"

struct iterator_context {
    uint8_t valid; /* 0 = consumed (already scanned or just invalid) */
    uint8_t last;  /* 1 = last block for the scan, don't ask driver for more! */
    uint8_t *buf;  /* ptr to device internal dma buffer */
    size_t dmaoff; /* position on user buffer */
    size_t totaloff;     /* offset into the scan */
    MacchiatoState *dev; /* reference to device */
    YR_MEMORY_BLOCK block;
};

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

void reset_compiler(MacchiatoState *dev);

void reset_scanner(MacchiatoState *dev);

void add_rule(MacchiatoState *dev, hwaddr addr, size_t size);

void scan_buffer(MacchiatoState *dev);

void get_results(MacchiatoState *dev);

#endif
