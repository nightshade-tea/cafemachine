#ifndef _CAFE_CHRDEV_H_
#define _CAFE_CHRDEV_H_

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>

int cafe_chrdev_init(void);

void cafe_chrdev_deinit(void);

int cafe_chrdev_create(struct pci_dev *pdev);

void cafe_chrdev_destroy(struct pci_dev *pdev);

#endif /* _CAFE_CHRDEV_H_ */
