#include "integrity.h"
#include "data.h"
#include "latte.h"

#include <linux/kprobes.h>

static typeof(kallsyms_lookup_name) *__kallsyms_lookup_name;
static typeof(x64_sys_call) *__x64_sys_call;

int latte_integrity_init(void)
{
	struct kprobe kp = {
		.symbol_name = "kallsyms_lookup_name",
	};

	int err;

	if ((err = register_kprobe(&kp)))
		return err;

	__kallsyms_lookup_name = (void *)kp.addr;
	unregister_kprobe(&kp);

	if (!(__x64_sys_call = (void *)__kallsyms_lookup_name("x64_sys_call")))
		return -ENOENT;

	return 0;
}
