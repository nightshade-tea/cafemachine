#ifndef MACCHIATO_SCAN_H
#define MACCHIATO_SCAN_H

#include "dma.h"
#include "irq.h"
#include "macchiato.h"
#include "macchiato_device.h"
#include "macchiato_log.h"
#include "serialize.h"

void reset_compiler(MacchiatoState *dev);

void reset_scanner(MacchiatoState *dev);

void add_rule(MacchiatoState *dev);

void begin_scan(MacchiatoState *dev);

void get_results(MacchiatoState *dev);

#endif
