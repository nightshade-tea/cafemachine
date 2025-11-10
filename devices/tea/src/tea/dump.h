#ifndef TEA_DUMP_H
#define TEA_DUMP_H

#include "tea_device.h"

/* dump memory from first until an error occurs */
int tea_dump_mem_strict(TeaState *dev,
                        const char basename[TEA_MAX_BASENAME_LEN], hwaddr first,
                        uint8_t dump_pages, uint32_t tolerance);

/* dump memory from first to last, ignoring errors */
int tea_dump_mem_relaxed(TeaState *dev,
                         const char basename[TEA_MAX_BASENAME_LEN],
                         hwaddr first, hwaddr last, uint8_t dump_pages);

/* write basename + extension to filename, handling edge cases */
void assemble_filename(
    const char basename[TEA_MAX_BASENAME_LEN],
    char filename[TEA_MAX_BASENAME_LEN + sizeof TEA_DUMP_EXT]);

#endif /* TEA_DUMP_H */
