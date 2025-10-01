#ifndef CAFE_LOG_H
#define CAFE_LOG_H

#include "cafe.h"

#define cafe_log(...) qemu_log(CAFE_HW_NAME ": " __VA_ARGS__)

#endif /* CAFE_LOG_H */
