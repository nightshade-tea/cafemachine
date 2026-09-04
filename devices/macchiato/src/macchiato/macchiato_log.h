#ifndef MACCHIATO_LOG_H
#define MACCHIATO_LOG_H

#include "macchiato.h"
#include "qemu/log.h"

#define macchiato_log(...) qemu_log(MACCHIATO_HW_NAME ": " __VA_ARGS__)

#endif /* MACCHIATO_LOG_H */
