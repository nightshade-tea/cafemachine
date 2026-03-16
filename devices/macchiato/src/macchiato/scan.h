#ifndef MACCHIATO_SCAN_H
#define MACCHIATO_SCAN_H

#include "macchiato.h"
#include "macchiato_device.h"

struct iterator_context {
    uint8_t valid; /* 0 = consumed (already scanned or just invalid) */
    uint8_t last;  /* 1 = last block for the scan, don't ask driver for more! */
    uint8_t *buf;  /* ptr to device internal dma buffer */
    YR_MEMORY_BLOCK block;
};

#endif
