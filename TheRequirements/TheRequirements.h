#ifndef TheRequirements_H
#define TheRequirements_H
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
 //Sad wee can't do WTFPL
#define Setup(description, init, exit) \
    MODULE_DESCRIPTION(description); \
    MODULE_LICENSE("GPL"); \
    MODULE_AUTHOR("We-Make-Software.Com"); \
    static int __init SetupInitProject(void) { \
        printk(KERN_INFO "%s: started\n", description); \
        init; \
        return 0; \
    } \
    static void __exit SetupExitProject(void) { \
        printk(KERN_INFO "%s: ending\n", description); \
        exit; \
    } \
    module_init(SetupInitProject); \
    module_exit(SetupExitProject);
#define U48_To_U64(value)((u64)(value[0])<<40|(u64)(value[1])<<32|(u64)(value[2])<<24|(u64)(value[3])<<16|(u64)(value[4])<<8|(u64)(value[5])&0xFFFFFFFFFFFF)
#endif