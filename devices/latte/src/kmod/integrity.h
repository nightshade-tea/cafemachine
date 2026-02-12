#ifndef _LATTE_INTEGRITY_H_
#define _LATTE_INTEGRITY_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

int latte_integrity_init(void);
int latte_register_protected_regions(struct pci_dev *pdev);

#endif /* _LATTE_INTEGRITY_H_ */
