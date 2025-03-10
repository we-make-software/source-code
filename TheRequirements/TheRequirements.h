#ifndef TheRequirements_H
#define TheRequirements_H
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#define Setup(description, init, exit) \
    MODULE_DESCRIPTION(description); \
    MODULE_LICENSE("GPL"); \
    MODULE_AUTHOR("We-Make-Software.Com"); \
    static int __init SetupInitProject(void) { \
        init; \
        return 0; \
    } \
    static void __exit SetupExitProject(void) { \
        exit; \
    } \
    module_init(SetupInitProject); \
    module_exit(SetupExitProject);

#endif