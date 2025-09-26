#ifndef _CAFE_KMOD_H_
#define _CAFE_KMOD_H_

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>

#define DEV_MINOR 0

MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("Driver for a cafemachine device");
MODULE_AUTHOR("phfr24 <phfr24@inf.ufpr.br>");
MODULE_AUTHOR("dcr23 <dcr23@inf.ufpr.br>");

#endif /* _CAFE_KMOD_H_ */
