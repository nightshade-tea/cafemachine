#include "integrity.h"
#include "data.h"
#include "irq.h"
#include "latte.h"
#include "mmio.h"

#include <linux/kprobes.h>

enum protected_region_id {
	REGION_X64_SYS_CALL,
	/* add more regions ... */
	PROTECTED_REGIONS_CNT
};

static struct memory_region {
	void *addr;
	size_t size;
} protected_regions[PROTECTED_REGIONS_CNT];

static struct {
	typeof(kallsyms_lookup_name) *kallsyms_lookup_name;
	typeof(kallsyms_lookup_size_offset) *kallsyms_lookup_size_offset;
	typeof(x64_sys_call) *x64_sys_call;
} sym;

static int sym_init(void)
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

static int symbol_memory_region(void *addr, struct memory_region *mr)
{
	unsigned long symbolsize, offset;

	if (!sym.kallsyms_lookup_size_offset((unsigned long)addr, &symbolsize,
					     &offset))
		return -ENOENT;

	mr->addr = (void *)((unsigned long)addr - offset);
	mr->size = symbolsize;

	return 0;
}

static int protected_regions_init(void)
{
	int err;

	if ((err = symbol_memory_region(
		     (void *)sym.x64_sys_call,
		     &protected_regions[REGION_X64_SYS_CALL])))
		return err;

	/* add more regions ... */

	return 0;
}

int latte_integrity_init(void)
{
	int err;

	if ((err = sym_init()))
		return err;

	if ((err = protected_regions_init()))
		return err;

	return 0;
}

int latte_register_protected_regions(struct pci_dev *pdev)
{
	for (enum protected_region_id i = 0; i < PROTECTED_REGIONS_CNT; i++) {

		/* send command to register protected memory region */
		latte_reg_write(pdev, LATTE_ADDR,
				__pa_symbol(protected_regions[i].addr));
		latte_reg_write(pdev, LATTE_REGION_SZ,
				protected_regions[i].size);
		latte_reg_write(pdev, LATTE_CMD, LATTE_REGISTER_MR);

		/* wait for command completion */
		latte_sync(pdev, LATTE_WAIT_NOTIFY);

		/* we should add an error mechanism ... */
	}

	return 0;
}
