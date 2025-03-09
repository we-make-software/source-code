#ifndef TheRequirements_H
#define TheRequirements_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#define Setup(init, exit) \
    static int __init init_module_func(void) { init return 0;} \
    static void __exit cleanup_module_func(void) { exit(); } \
    module_init(init_module_func); \
    module_exit(cleanup_module_func);

#endif // TheRequirements_H