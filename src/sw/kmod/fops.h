#ifndef _CAFE_FOPS_H_
#define _CAFE_FOPS_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

/* set file_operations struct fields */
void cafe_init_fops(struct file_operations *fops);

int cafe_mmap(struct file *f, struct vm_area_struct *vma);

long cafe_ioctl(struct file *f, unsigned int cmd, unsigned long arg);

#endif /* _CAFE_FOPS_H_ */
