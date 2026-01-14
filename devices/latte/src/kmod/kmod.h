#ifndef _LATTE_KMOD_H_
#define _LATTE_KMOD_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

/* support 64 bit addresses */
#define LATTE_DMA_BIT_MASK DMA_BIT_MASK(64)

MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("Driver for a lattemachine device");
MODULE_AUTHOR("phfr24 <phfr24@inf.ufpr.br>");
MODULE_AUTHOR("dcr23 <dcr23@inf.ufpr.br>");

#endif /* _LATTE_KMOD_H_ */
