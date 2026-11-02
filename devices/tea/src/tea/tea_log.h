#ifndef TEA_LOG_H
#define TEA_LOG_H

#include "qemu/log.h"
#include "tea.h"

#define tea_log(...) qemu_log(TEA_HW_NAME ": " __VA_ARGS__)

#endif /* TEA_LOG_H */
