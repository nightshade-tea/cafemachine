#include "fops.h"
#include "cafe.h"
#include "chrdev.h"
#include "data.h"
#include "kmod.h"
#include <linux/ioport.h>

static void cafe_dump_mem(struct pci_dev *pdev, unsigned long filename) {
  struct device *dev;
  struct cafe_dev_data *data;
  phys_addr_t ram_size;

  dev = &pdev->dev;
  data = pci_get_drvdata(pdev);
  ram_size = max_ram_addr;

  dev_info(dev, "cafe_dump_mem(): writing %llu bytes to %s\n", ram_size,
           (char *)&filename);

  writeq(CAFE_DMA_BUF_SZ, data->bar.mmio + CAFE_DMA_SZ * 8);
  writeq(filename, data->bar.mmio + CAFE_DUMP_FILENAME * 8);

  for (uint64_t i = 0; i < (ram_size / CAFE_DMA_BUF_SZ); i++) {
    writeq(i * CAFE_DMA_BUF_SZ, data->bar.mmio + CAFE_DMA_SRC * 8);
    writeq(CAFE_DMA_READ, data->bar.mmio + CAFE_CMD * 8);

    wait_for_completion(&data->devop_done[CAFE_WAIT_DMA]);
    reinit_completion(&data->devop_done[CAFE_WAIT_DMA]);

    writeq(CAFE_DUMP_DMA_BUF, data->bar.mmio + CAFE_CMD * 8);

    wait_for_completion(&data->devop_done[CAFE_WAIT_DMA]);
    reinit_completion(&data->devop_done[CAFE_WAIT_DMA]);
  }

  // todo write a function bruh
  /* sapataria */
  writeq(ram_size % CAFE_DMA_BUF_SZ, data->bar.mmio + CAFE_DMA_SZ * 8);
  writeq(ram_size - (ram_size % CAFE_DMA_BUF_SZ),
         data->bar.mmio + CAFE_DMA_SRC * 8);
  writeq(CAFE_DMA_READ, data->bar.mmio + CAFE_CMD * 8);

  wait_for_completion(&data->devop_done[CAFE_WAIT_DMA]);
  reinit_completion(&data->devop_done[CAFE_WAIT_DMA]);

  writeq(CAFE_DUMP_DMA_BUF, data->bar.mmio + CAFE_CMD * 8);

  wait_for_completion(&data->devop_done[CAFE_WAIT_DMA]);
  reinit_completion(&data->devop_done[CAFE_WAIT_DMA]);
  /* ------------------------ */

  return;
}

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

long cafe_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
  struct pci_dev *pdev;
  struct device *dev;
  struct cafe_dev_data *data;

  if (cmd >= CAFE_IOCTL_CNT)
    return -EINVAL;

  if (!(pdev = cafe_minor_to_dev(iminor(f->f_inode)))) {
    pr_err("cafe_minor_to_dev() returned NULL! minor does not correspond"
           "to any device\n");
    return -ENODEV;
  }

  dev = &pdev->dev;
  data = pci_get_drvdata(pdev);

  switch (cmd) {
  case CAFE_IOCTL_DUMP_MEM:
    cafe_dump_mem(pdev, arg);
    break;
  }

  return 0;
}

void cafe_init_fops(struct file_operations *fops) {
  fops->owner = THIS_MODULE;
  fops->mmap = cafe_mmap;
  fops->unlocked_ioctl = cafe_ioctl;
}
