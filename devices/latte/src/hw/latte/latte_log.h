#ifndef LATTE_LOG_H
#define LATTE_LOG_H

#include "latte.h"
#include "qemu/log.h"

#define latte_log(...) qemu_log(LATTE_HW_NAME ": " __VA_ARGS__)

#endif /* LATTE_LOG_H */
