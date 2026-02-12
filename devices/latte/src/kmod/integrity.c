#include "integrity.h"
#include "data.h"
#include "latte.h"

#include <linux/kprobes.h>

/* unexported symbols */
static struct {
	typeof(kallsyms_lookup_name) *kallsyms_lookup_name;
	typeof(kallsyms_lookup_size_offset) *kallsyms_lookup_size_offset;
	typeof(x64_sys_call) *x64_sys_call;
} sym;

int latte_integrity_init(void)
{
	struct kprobe kp = {
		.symbol_name = "kallsyms_lookup_name",
	};

	int err;

	if ((err = register_kprobe(&kp)))
		return err;

	sym.kallsyms_lookup_name = (void *)kp.addr;
	unregister_kprobe(&kp);

	if (!(sym.kallsyms_lookup_size_offset =
		      (void *)sym.kallsyms_lookup_name(
			      "kallsyms_lookup_size_offset")))
		return -ENOENT;

	if (!(sym.x64_sys_call =
		      (void *)sym.kallsyms_lookup_name("x64_sys_call")))
		return -ENOENT;

	return 0;
}

int latte_register_protected_regions(struct pci_dev *pdev)
{
	struct latte_dev_data *data;
	struct device *dev;
	int err;

	dev = &pdev->dev;
	data = pci_get_drvdata(pdev);

	return 0;
}
