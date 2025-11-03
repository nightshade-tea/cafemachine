#ifndef TEA_UTILS_H
#define TEA_UTILS_H

#include "tea_device.h"
#include "tea_log.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

void mount_filename(const char basename[TEA_MAX_BASENAME_LEN], char filename[]);

#endif /* TEA_UTILS_H */
