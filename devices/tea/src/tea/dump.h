#ifndef TEA_DUMP_H
#define TEA_DUMP_H

#include "tea_device.h"

int tea_dump_mem(TeaState *dev, const char basename[TEA_MAX_BASENAME_LEN],
                 hwaddr first, hwaddr last);

#endif /* TEA_DUMP_H */
