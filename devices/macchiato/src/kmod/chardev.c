#include "chardev.h"
#include "data.h"
#include "macchiato.h"
#include "mmio.h"
#include <linux/cdev.h>
#include <linux/fs.h>

static struct class *class;
static dev_t base_devt;
static DEFINE_XARRAY_ALLOC(minors_xa);

static inline struct pci_dev *minor_pdev(int minor)
{
	return xa_load(&minors_xa, minor);
}

#define fprk(func, fmt, ...) printk("%s(): " fmt "\n", func, ##__VA_ARGS__)

#define prk(fmt, ...) fprk(__func__, fmt, ##__VA_ARGS__)

#define send_cmd(data, op) (send_cmd)(__func__, data, op)

static inline long(send_cmd)(const char *func, struct macchiato_dev_data *data,
			     enum macchiato_op op)
{
	fprk(func, "reinit_completion()...");
	reinit_completion(&data->sync);

	fprk(func, "sending command...");
	macchiato_reg_write_d(data, MACCHIATO_REG_CMD, op);

	fprk(func, "waiting for completion...");
	wait_for_completion(&data->sync);

	return data->last_hwirq;
}

/* -------------------------------------------------------------------------- */

static long reset_compiler(struct macchiato_dev_data *data)
{
	send_cmd(data, MACCHIATO_OP_RESET_COMPILER);
	prk("done!");
	return 0;
}

static long reset_scanner(struct macchiato_dev_data *data)
{
	send_cmd(data, MACCHIATO_OP_RESET_SCANNER);
	prk("done!");
	return 0;
}

static long end_scan(struct macchiato_dev_data *data)
{
	prk("called...");

	reinit_completion(&data->sync);

	prk("lowering %s...", macchiato_irq_str(MACCHIATO_IRQ_NEXT_BUF));
	macchiato_reg_write_d(data, MACCHIATO_REG_CMD, MACCHIATO_OP_END_SCAN);
	macchiato_reg_write_d(data, MACCHIATO_REG_LOWER_IRQ,
			      MACCHIATO_IRQ_NEXT_BUF);

	wait_for_completion(&data->sync);

	if (data->last_hwirq != MACCHIATO_IRQ_DONE) {
		prk("got irq %s!", macchiato_irq_str(data->last_hwirq));
		return -EIO;
	}

	prk("done!");
	return 0;
}

static int open(struct inode *inode, struct file *file)
{
	struct pci_dev *pdev;
	struct macchiato_dev_data *data;

	if (!(pdev = minor_pdev(iminor(inode))))
		return -ENODEV;

	data = pci_get_drvdata(pdev);

	prk("grabbing char device lock...");
	if (test_and_set_bit_lock(MACCHIATO_LOCK_FD, data->lock))
		return -EBUSY;

	file->private_data = data;

	prk("reset_compiler()...");
	reset_compiler(data);

	prk("reset_scanner()...");
	reset_scanner(data);

	prk("done!");
	return nonseekable_open(inode, file);
}

static int release(struct inode *inode, struct file *file)
{
	struct macchiato_dev_data *data;
	struct pci_dev *pdev;

	data = file->private_data;
	pdev = data->pdev;

	if (data->dmabuf.size) {
		prk("freeing dmabuf...");
		dma_free_coherent(&pdev->dev, data->dmabuf.size,
				  data->dmabuf.kvaddr, data->dmabuf.paddr);
		data->dmabuf.size = 0;
	}

	prk("releasing char device lock...");
	clear_bit_unlock(MACCHIATO_LOCK_FD, data->lock);

	prk("done!");
	return 0;
}

static long add_rules(struct macchiato_dev_data *data, unsigned long arg)
{
	prk("called...");

	if (!data->dmabuf.size)
		return -EFAULT;

	if (arg > data->dmabuf.size)
		return -EINVAL;

	macchiato_reg_write_d(data, MACCHIATO_REG_ADDR, data->dmabuf.paddr);
	macchiato_reg_write_d(data, MACCHIATO_REG_SIZE, arg);

	if (send_cmd(data, MACCHIATO_OP_ADD_RULE) == MACCHIATO_IRQ_FAILED)
		return -ENOSPC; /* just want an unique errno so i can track it
				   without printing stuff  */

	prk("done!");
	return 0;
}

static long begin_scan(struct macchiato_dev_data *data, unsigned long arg)
{
	enum macchiato_irq irq;
	prk("called...");

	if (!data->dmabuf.size)
		return -EFAULT;

	if (arg > data->dmabuf.size)
		return -EINVAL;

	macchiato_reg_write_d(data, MACCHIATO_REG_ADDR, data->dmabuf.paddr);
	macchiato_reg_write_d(data, MACCHIATO_REG_SIZE, arg);

	irq = send_cmd(data, MACCHIATO_OP_BEGIN_SCAN);
	prk("got irq %s!", macchiato_irq_str(irq));

	switch (irq) {
	case MACCHIATO_IRQ_DONE:
		return 0;

	case MACCHIATO_IRQ_FAILED:
		return -EAGAIN;

	case MACCHIATO_IRQ_NEXT_BUF:
		return -ENODATA;

	case MACCHIATO_IRQ_FIRST_BUF:
		return -ESPIPE;

	default:
		return -EIO;
	}
}

static long scan_op(const char *func, enum macchiato_op op,
		    enum macchiato_irq lower, struct macchiato_dev_data *data,
		    unsigned long arg)
{
	fprk(func, "called...");

	if (!data->dmabuf.size)
		return -EFAULT;

	if (arg > data->dmabuf.size)
		return -EINVAL;

	reinit_completion(&data->sync);

	macchiato_reg_write_d(data, MACCHIATO_REG_ADDR, data->dmabuf.paddr);
	macchiato_reg_write_d(data, MACCHIATO_REG_SIZE, arg);
	macchiato_reg_write_d(data, MACCHIATO_REG_CMD, op);
	fprk(func, "lowering irq %s...", macchiato_irq_str(lower));
	macchiato_reg_write_d(data, MACCHIATO_REG_LOWER_IRQ, lower);

	wait_for_completion(&data->sync);
	fprk(func, "got irq %s!", macchiato_irq_str(data->last_hwirq));

	switch (data->last_hwirq) {
	case MACCHIATO_IRQ_DONE:
		return 0;

	case MACCHIATO_IRQ_FAILED:
		return -EAGAIN;

	case MACCHIATO_IRQ_NEXT_BUF:
		return -ENODATA;

	case MACCHIATO_IRQ_FIRST_BUF:
		return -ESPIPE;

	default:
		return -EIO;
	}
}

static long continue_scan(struct macchiato_dev_data *data, unsigned long arg)
{
	return scan_op(__func__, MACCHIATO_OP_CONTINUE_SCAN,
		       MACCHIATO_IRQ_NEXT_BUF, data, arg);
}

static long rewind_scan(struct macchiato_dev_data *data, unsigned long arg)
{
	return scan_op(__func__, MACCHIATO_OP_REWIND_SCAN,
		       MACCHIATO_IRQ_FIRST_BUF, data, arg);
}

static long get_results(struct macchiato_dev_data *data)
{
	unsigned long serialsz;
	prk("called...");

	if (!data->dmabuf.size)
		return -EFAULT;

	serialsz = macchiato_reg_read_d(data, MACCHIATO_REG_SIZE);

	if (data->dmabuf.size < serialsz)
		return -ENOSPC;

	macchiato_reg_write_d(data, MACCHIATO_REG_ADDR, data->dmabuf.paddr);
	macchiato_reg_write_d(data, MACCHIATO_REG_SIZE, data->dmabuf.size);

	if (send_cmd(data, MACCHIATO_OP_GET_RESULTS) == MACCHIATO_IRQ_FAILED)
		return -ENOSPC;

	prk("done!");
	return 0;
}

static long ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct macchiato_dev_data *data;

	data = file->private_data;

	printk("ioctl( cmd=%u arg=%lu )\n", cmd, arg);

	switch (cmd) {
	case MACCHIATO_IOC_RESET_COMPILER:
		return reset_compiler(data);

	case MACCHIATO_IOC_RESET_SCANNER:
		return reset_scanner(data);

	case MACCHIATO_IOC_ADD_RULE:
		return add_rules(data, arg);

	case MACCHIATO_IOC_BEGIN_SCAN:
		return begin_scan(data, arg);

	case MACCHIATO_IOC_CONTINUE_SCAN:
		return continue_scan(data, arg);

	case MACCHIATO_IOC_REWIND_SCAN:
		return rewind_scan(data, arg);

	case MACCHIATO_IOC_END_SCAN:
		return end_scan(data);

	case MACCHIATO_IOC_GET_RESULTS:
		return get_results(data);
	}

	prk("-ENOTTY");
	return -ENOTTY;
}

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
	prk("dma_alloc_coherent(len = %zu)...", len);
	data->dmabuf.kvaddr = dma_alloc_coherent(
		&pdev->dev, len, &data->dmabuf.paddr, GFP_KERNEL);

	if (!data->dmabuf.kvaddr)
		return -ENOMEM;

	prk("dma_mmap_coherent()...");
	err = dma_mmap_coherent(&pdev->dev, vma, data->dmabuf.kvaddr,
				data->dmabuf.paddr, len);

	if (err) {
		dma_free_coherent(&pdev->dev, len, data->dmabuf.kvaddr,
				  data->dmabuf.paddr);
		return err;
	}

	data->dmabuf.size = len;
	prk("done!");
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
