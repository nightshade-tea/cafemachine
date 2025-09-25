#ifndef _CAFE_FOPS_H_
#define _CAFE_FOPS_H_

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>

int cafe_mmap(struct file *f, struct vm_area_struct *vma);

#endif /* _CAFE_FOPS_H_ */
