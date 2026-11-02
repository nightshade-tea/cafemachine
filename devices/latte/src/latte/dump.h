#ifndef LATTE_DUMP_H
#define LATTE_DUMP_H

#include "latte_device.h"

/* dump memory from first until an error occurs */
int latte_dump_mem_strict(LatteState *dev, char *buf, hwaddr first, hwaddr last,
                          uint8_t dump_pages, uint32_t tolerance);

/* dump memory from first to last, ignoring errors */
int latte_dump_mem_relaxed(LatteState *dev, char *buf, hwaddr first,
                           hwaddr last, uint8_t dump_pages);

#endif /* LATTE_DUMP_H */
