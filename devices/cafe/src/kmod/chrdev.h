#ifndef _CAFE_CHRDEV_H_
#define _CAFE_CHRDEV_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

int cafe_chrdev_init(void);

void cafe_chrdev_deinit(void);

/* create /dev/cafe file for a device */
int cafe_chrdev_create(struct pci_dev *pdev);

void cafe_chrdev_destroy(struct pci_dev *pdev);

struct pci_dev *cafe_minor_to_dev(int minor);

#endif /* _CAFE_CHRDEV_H_ */
