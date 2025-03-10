#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

KSYMTAB_FUNC(ThePostOfficeSendPacket, "", "");

SYMBOL_CRC(ThePostOfficeSendPacket, 0xafa4ef4f, "");

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x99e92706, "kmalloc_caches" },
	{ 0xe3272708, "__kmalloc_cache_noprof" },
	{ 0x37a0cba, "kfree" },
	{ 0x5886c606, "sk_skb_reason_drop" },
	{ 0x92997ed8, "_printk" },
	{ 0xf3f7b6d0, "dev_remove_pack" },
	{ 0x683f8d9, "__netdev_alloc_skb" },
	{ 0xbd70c7f7, "skb_put" },
	{ 0x69acdf38, "memcpy" },
	{ 0xf5cb60e5, "__dev_queue_xmit" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x561705fd, "dev_add_pack" },
	{ 0xf9108d3, "module_layout" },
};

MODULE_INFO(depends, "");

