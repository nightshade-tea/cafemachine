#ifndef _MACCHIATO_CHARDEV_H_
#define _MACCHIATO_CHARDEV_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

/* driver initialization */
int macchiato_chardev_init(void);
void macchiato_chardev_deinit(void);

/* device initialization */
int macchiato_chardev_create(struct pci_dev *pdev);
void macchiato_chardev_delete(struct pci_dev *pdev);

#endif /* _MACCHIATO_CHARDEV_H_ */
