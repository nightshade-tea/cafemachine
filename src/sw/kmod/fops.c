#include "fops.h"
#include "chrdev.h"
#include "data.h"
#include "hw/cafe.h"

int cafe_mmap(struct file *f, struct vm_area_struct *vma) {
    struct pci_dev *pdev;
    struct device *dev;
    struct cafe_dev_data *data;
    unsigned long len;
    int err;

    if (!(pdev = cafe_minor_to_dev(iminor(f->f_inode)))) {
        pr_err("cafe_minor_to_dev() returned NULL\n");
        return -EIO;
    }

    dev = &pdev->dev;
    data = pci_get_drvdata(pdev);

    if ((len = vma->vm_end - vma->vm_start) > data->bar.len) {
        dev_err(dev, "cafe_mmap() out of bounds request\n");
        return -EIO;
    }

    if ((err = vm_iomap_memory(vma, data->bar.start, len))) {
        dev_err(dev, "vm_iomap_memory() failed: %d\n", err);
        return err;
    }

    return 0;
}
