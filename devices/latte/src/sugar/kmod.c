#include "kmod.h"

#include <asm/text-patching.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>

enum hook_id { HOOK_GETDENTS64, HOOKS_CNT };

static struct hook {

	/* pointer to call/jmp instruction */
	unsigned char *instr;

	/* pointer to rel32 argument in call/jmp instruction */
	s32 *rel32;

	/* absolute addresses */
	sys_call_ptr_t target;
	sys_call_ptr_t trampoline;

	/* near, relative to addr (rel32 argument) */
	s32 rel_target;
	s32 rel_trampoline;

} hooks[HOOKS_CNT];

static long getdents64(const struct pt_regs *regs)
{
	printk("getdents64() trampoline called!!\n");
	return hooks[HOOK_GETDENTS64].target(regs);
}

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

static int init_hooks(void)
{
	unsigned char *x64_sys_call;
	unsigned long x64_sys_call_sz;

	if (!(x64_sys_call = ksymbol_addr("x64_sys_call")))
		return -ENOENT;

	if (!(x64_sys_call_sz = ksymbol_size(x64_sys_call)))
		return -ENODATA;

	printk("x64_sys_call = [0x%px .. 0x%px] (%lu bytes)\n", x64_sys_call,
	       (void *)((unsigned long)x64_sys_call + x64_sys_call_sz - 1),
	       x64_sys_call_sz);

	/* set targets and trampolines */

	if (!(hooks[HOOK_GETDENTS64].target =
		      ksymbol_addr("__x64_sys_getdents64")))
		return -ENOSYS;

	hooks[HOOK_GETDENTS64].trampoline = getdents64;

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
			if (target == (unsigned long)hooks[id].target) {
				printk("found target for hook_id=%d\n", id);
				hooks[id].instr = instr;
				hooks[id].rel32 = rel32;
				hooks[id].rel_target = *rel32;
				hooks[id].rel_trampoline =
					(unsigned long)hooks[id].trampoline -
					offset;
			}
		}
	}

	printk("finished hooks[] initialization\n");

	return 0;
}

static int enable_hooks(void)
{
	int err;

	for (enum hook_id id = 0; id < HOOKS_CNT; id++) {
		if (hooks[id].rel32) {
			err = ktext_write(hooks[id].rel32,
					  &hooks[id].rel_trampoline,
					  sizeof(hooks[id].rel_trampoline));

			if (!err)
				printk("hook_id=%d hooked!\n", id);
			else
				pr_err("ktext_write() failed: %pe\n",
				       ERR_PTR(err));
		}
	}

	return 0;
}

static void disable_hooks(void)
{
	int err;

	for (enum hook_id id = 0; id < HOOKS_CNT; id++) {
		if (hooks[id].rel32) {
			ktext_write(hooks[id].rel32, &hooks[id].rel_target,
				    sizeof(hooks[id].rel_target));

			if (!err)
				printk("hook_id=%d unhooked!\n", id);
			else
				pr_err("ktext_write() failed: %pe\n",
				       ERR_PTR(err));
		}
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
