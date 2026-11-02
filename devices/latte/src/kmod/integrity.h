#ifndef _LATTE_INTEGRITY_H_
#define _LATTE_INTEGRITY_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

/* call once at driver initialization */
int latte_integrity_init(void);

/* call once at device initialization */
int latte_register_protected_regions(struct pci_dev *pdev);

#endif /* _LATTE_INTEGRITY_H_ */
