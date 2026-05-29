#include "chardev.h"
#include "data.h"
#include "macchiato.h"
#include <linux/cdev.h>
#include <linux/fs.h>

static struct class *class;
static dev_t base_devt;
static DEFINE_XARRAY_ALLOC(minors_xa);

static inline struct pci_dev *minor_pdev(int minor)
{
	return xa_load(&minors_xa, minor);
}

static inline long send_cmd(char *func, macchiato_dev_data *data,
			    enum macchiato_op op)
{
	printk("%s(): reinit_completion()...\n", func);
	reinit_completion(&data->sync);

	printk("%s(): sending command...\n", func);
	macchiato_reg_write_d(data, MACCHIATO_REG_CMD, op);

	printk("%s(): waiting for completion...\n", func);
	wait_for_completion(&data->sync);

	return data->last_hwirq;
}

/* -------------------------------------------------------------------------- */

static long reset_compiler(struct macchiato_dev_data *data)
{
	send_cmd("reset_compiler", data, MACCHIATO_OP_RESET_COMPILER);
	printk("reset_compiler(): done!\n");
	return 0;
}

static long reset_scanner(struct macchiato_dev_data *data)
{
	send_cmd("reset_scanner", data, MACCHIATO_OP_RESET_SCANNER);
	printk("reset_scanner(): done!\n");
	return 0;
}

static int open(struct inode *inode, struct file *file)
{
	struct pci_dev *pdev;
	struct macchiato_dev_data *data;

	if (!(pdev = minor_pdev(iminor(inode))))
		return -ENODEV;

	data = pci_get_drvdata(pdev);

	printk("open(): grabbing char device lock...\n");
	if (test_and_set_bit_lock(MACCHIATO_LOCK_FD, data->lock))
		return -EBUSY;

	file->private_data = data;

	printk("open(): reset_compiler()...\n");
	reset_compiler(data);

	printk("open(): reset_scanner()...\n");
	reset_scanner(data);

	printk("open(): done!\n");
	return nonseekable_open(inode, file);
}

static int release(struct inode *inode, struct file *file)
{
	struct macchiato_dev_data *data;
	struct pci_dev *pdev;

	data = file->private_data;
	pdev = data->pdev;

	if (data->dmabuf.size) {
		printk("release(): freeing dmabuf...\n");
		dma_free_coherent(&pdev->dev, data->dmabuf.size,
				  data->dmabuf.kvaddr, data->dmabuf.paddr);
		data->dmabuf.size = 0;
	}

	printk("release(): releasing char device lock...\n");
	clear_bit_unlock(MACCHIATO_LOCK_FD, data->lock);

	printk("release(): done!\n");
	return 0;
}

static long add_rules(struct macchiato_dev_data *data, unsigned long arg)
{
	printk("add_rules called...\n");

	if (!data->dmabuf.size)
		return -EFAULT;

	if (arg > data->dmabuf.size)
		return -EINVAL;

	macchiato_reg_write_d(data, MACCHIATO_REG_ADDR, data->dmabuf.paddr);
	macchiato_reg_write_d(data, MACCHIATO_REG_SIZE, arg);

	if (send_cmd("add_rules", data, MACCHIATO_OP_ADD_RULE) ==
	    MACCHIATO_IRQ_FAILED)
		return -ENOSPC; /* just want an unique errno so i can track it
				   without printing stuff  */

	printk("add_rules(): done!\n");
	return 0;
}

static long scan_buffer(struct macchiato_dev_data *data, unsigned long arg)
{

	if (!data->dmabuf.size)
		return -EFAULT;

	if (arg > data->dmabuf.size)
		return -EINVAL;

	macchiato_reg_write_d(data, MACCHIATO_REG_ADDR, data->dmabuf.paddr);
	macchiato_reg_write_d(data, MACCHIATO_REG_SIZE, arg);

	switch (send_cmd("scan_buffer", data, MACCHIATO_OP_SCAN_BUFFER)) {
	case MACCHIATO_IRQ_FAILED:
		return -ENOSPC;

	case MACCHIATO_IRQ_FIRST_BUF:
		return -EAGAIN;
	}

	printk("scan_buffer(): done!\n");
	return 0;
}

static long get_results(struct macchiato_dev_data *data)
{
	unsigned long serialsz;

	if (!data->dmabuf.size)
		return -EFAULT;

	serialsz = macchiato_reg_read_d(data, MACCHIATO_REG_SIZE);

	if (data->dmabuf.size < serialsz)
		return -ENOSPC;

	macchiato_reg_write_d(data, MACCHIATO_REG_ADDR, data->dmabuf.paddr);
	macchiato_reg_write_d(data, MACCHIATO_REG_SIZE, data->dmabuf.size);

	if (send_cmd("get_results", data, MACCHIATO_OP_GET_RESULTS) ==
	    MACCHIATO_IRQ_FAILED)
		return -ENOSPC;

	printk("get_results(): done!\n");
	return 0;
}

static long ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct macchiato_dev_data *data;

	data = file->private_data;

	printk("ioctl( cmd=%u arg=%lu )\n", cmd, arg);

	switch (cmd) {
	case MACCHIATO_IOC_RESET_COMPILER:
		printk("ioctl(): reset_compiler()\n");
		return reset_compiler(data);

	case MACCHIATO_IOC_RESET_SCANNER:
		printk("ioctl(): reset_scanner()\n");
		return reset_scanner(data);

	case MACCHIATO_IOC_ADD_RULE:
		printk("ioctl(): add_rules()\n");
		return add_rules(data, arg);

	case MACCHIATO_IOC_SCAN_BUFFER:
		printk("ioctl(): scan_buffer()\n");
		return scan_buffer(data, arg);

	case MACCHIATO_IOC_GET_RESULTS:
		printk("ioctl(): get_results()\n");
		return get_results(data);
	}

	printk("ioctl(): -ENOTTY\n");
	return -ENOTTY;
}

/*
static ssize_t write(struct file *file, const char __user *ubuf, size_t size,
		     loff_t *off) {
  struct macchiato_dev_data *data;

  data = file->private_data;

  writeq(virt_to_phys((void *)ubuf),
	 (char *)data->bar[MACCHIATO_BAR_ID_CTL].mmio + MACCHIATO_REG_ADDR * 8);
  writeq(size,
	 (char *)data->bar[MACCHIATO_BAR_ID_CTL].mmio + MACCHIATO_REG_SIZE * 8);

  reinit_completion(&data->sync);
  writeq(MACCHIATO_OP_SCAN_BUFFER,
	 (char *)data->bar[MACCHIATO_BAR_ID_CTL].mmio + MACCHIATO_REG_CMD * 8);
  wait_for_completion(&data->sync);

  switch (data->last_hwirq) {
    case MACCHIATO_IRQ_FAILED:
      return -EINVAL;

    case MACCHIATO_IRQ_FIRST_BUF:
      return -EAGAIN;
  }

  return size;
}

static ssize_t read(struct file *file, char __user *ubuf, size_t size,
		    loff_t *off) {
  struct macchiato_dev_data *data;
  unsigned long serialsz;

  data = file->private_data;
  serialsz = readq((char *)data->bar[MACCHIATO_BAR_ID_CTL].mmio +
      MACCHIATO_REG_SIZE * 8);

  if (size < serialsz)
    return -ENOSPC;

  writeq(virt_to_phys((void *)ubuf),
	 (char *)data->bar[MACCHIATO_BAR_ID_CTL].mmio + MACCHIATO_REG_ADDR * 8);

  reinit_completion(&data->sync);
  writeq(MACCHIATO_OP_GET_RESULTS,
	 (char *)data->bar[MACCHIATO_BAR_ID_CTL].mmio + MACCHIATO_REG_CMD * 8);
  wait_for_completion(&data->sync);

  if (data->last_hwirq == MACCHIATO_IRQ_FAILED)
    return -EINVAL;

  return serialsz;
}
*/

/* allocate dmabuf, map it to userspace  */
static int mmap(struct file *file, struct vm_area_struct *vma)
{
	struct macchiato_dev_data *data;
	struct pci_dev *pdev;
	size_t len;
	int err;

	data = file->private_data;
	pdev = data->pdev;

	if (data->dmabuf.size)
		return -ENFILE;

	len = vma->vm_end - vma->vm_start; /* [vm_start; vm_end)  */
	printk("mmap( len = %zu ): allocating memory...\n", len);
	data->dmabuf.kvaddr = dma_alloc_coherent(
		&pdev->dev, len, &data->dmabuf.paddr, GFP_KERNEL);

	if (!data->dmabuf.kvaddr)
		return -ENOMEM;

	printk("mmap(): mapping memory...\n");
	err = dma_mmap_coherent(&pdev->dev, vma, data->dmabuf.kvaddr,
				data->dmabuf.paddr, len);

	if (err) {
		dma_free_coherent(&pdev->dev, len, data->dmabuf.kvaddr,
				  data->dmabuf.paddr);
		return err;
	}

	data->dmabuf.size = len;
	printk("mmap(): done!\n");
	return 0;
}

/* -------------------------------------------------------------------------- */

static const struct file_operations f_ops = {
	.owner = THIS_MODULE,
	.open = open,
	.release = release,
	.unlocked_ioctl = ioctl,
	.mmap = mmap,
};

/* create a character device file for a macchiato device */
int macchiato_chardev_create(struct pci_dev *pdev)
{
	struct macchiato_dev_data *data;
	struct device *dev;
	void *errp;
	int err, minor;

	dev = &pdev->dev;
	data = pci_get_drvdata(pdev);

	if ((err = xa_alloc(&minors_xa, &minor, pdev, XA_LIMIT(0, MINORMASK),
			    GFP_KERNEL))) {
		dev_err(dev, "xa_alloc() failed: %pe\n", ERR_PTR(err));
		goto err_xa_alloc;
	}

	data->devt = MKDEV(MAJOR(base_devt), minor);

	cdev_init(&data->cdev, &f_ops);
	data->cdev.owner = THIS_MODULE;

	if ((err = cdev_add(&data->cdev, data->devt, 1))) {
		dev_err(dev, "cdev_add() failed: %pe\n", ERR_PTR(err));
		goto err_cdev_add;
	}

	/* create the device file in /dev */
	if (IS_ERR(errp = device_create(class, dev, data->devt, NULL,
					MACCHIATO_HW_NAME "%d",
					MINOR(data->devt)))) {
		dev_err(dev, "device_create() failed: %pe\n", errp);
		err = PTR_ERR(errp);
		goto err_device_create;
	}

	return 0;

err_device_create:
	cdev_del(&data->cdev);

err_cdev_add:
	xa_erase(&minors_xa, MINOR(data->devt));

err_xa_alloc:
	return err;
}

void macchiato_chardev_delete(struct pci_dev *pdev)
{
	struct macchiato_dev_data *data;

	data = pci_get_drvdata(pdev);
	device_destroy(class, data->devt);
	cdev_del(&data->cdev);
	xa_erase(&minors_xa, MINOR(data->devt));
}

int macchiato_chardev_init(void)
{
	int err;

	if ((err = alloc_chrdev_region(&base_devt, 0, MINORMASK,
				       MACCHIATO_HW_NAME))) {
		pr_err("alloc_chrdev_region() failed: %pe\n", ERR_PTR(err));
		goto err_alloc_chrdev_region;
	}

	if (IS_ERR(class = class_create(MACCHIATO_HW_NAME))) {
		pr_err("class_create() failed: %pe\n", class);
		err = PTR_ERR(class);
		goto err_class_create;
	}

	return 0;

err_class_create:
	unregister_chrdev_region(base_devt, MINORMASK);

err_alloc_chrdev_region:
	return err;
}

void macchiato_chardev_deinit(void)
{
	class_destroy(class);
	unregister_chrdev_region(base_devt, MINORMASK);
	xa_destroy(&minors_xa);
}
