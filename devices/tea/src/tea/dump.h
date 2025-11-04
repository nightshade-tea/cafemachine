#ifndef TEA_DUMP_H
#define TEA_DUMP_H

#include "tea_device.h"

/* dump memory from first until an error occurs */
int tea_dump_mem_strict(TeaState *dev,
                        const char basename[TEA_MAX_BASENAME_LEN],
                        hwaddr first);

/* dump memory from first to last, ignoring errors */
int tea_dump_mem_relaxed(TeaState *dev,
                         const char basename[TEA_MAX_BASENAME_LEN],
                         hwaddr first, hwaddr last);

void assemble_filename(
    const char basename[TEA_MAX_BASENAME_LEN],
    char filename[TEA_MAX_BASENAME_LEN + sizeof TEA_DUMP_EXT]);

#endif /* TEA_DUMP_H */
