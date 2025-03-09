#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("We-Make-Software.Com");
MODULE_DESCRIPTION("A simple kernel module test");

static int __init mymodule_init(void) {
    printk(KERN_INFO "We-Make-Software.Com Kernel Module Loaded!\n");
    return 0;
}

static void __exit mymodule_exit(void) {
    printk(KERN_INFO "We-Make-Software.Com Kernel Module Unloaded!\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);
