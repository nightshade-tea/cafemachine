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
        pr_err("cafe_minor_to_dev() returned NULL! minor does not correspond"
                "to any device\n");
        err = -ENODEV;
        goto err_exit;
    }

    dev = &pdev->dev;
    data = pci_get_drvdata(pdev);

    if ((len = vma->vm_end - vma->vm_start) > data->bar.len) {
        dev_err(dev, "cafe_mmap() out of bounds request\n");
        err = -EINVAL;
        goto err_exit;
    }

    if ((err = vm_iomap_memory(vma, data->bar.start, len))) {
        dev_err(dev, "vm_iomap_memory() failed: %pe\n", ERR_PTR(err));
        goto err_exit;
    }

    return 0;

err_exit:

    pr_err("cafe_mmap() failed: %pe\n", ERR_PTR(err));
    return err;
}
