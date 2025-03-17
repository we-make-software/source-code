#include "../TheRequirements/TheRequirements.h"
#include <linux/module.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>

static ktime_t start=NULL, end=NULL;
void StartTest(void) {
    start = ktime_get(); 
    printk(KERN_INFO "Test started at: %lld ns\n", ktime_to_ns(start));
}
EXPORT_SYMBOL(EndTest);
void EndTest(void) {
    if(!start){
        printk(KERN_INFO "Test has not started yet\n");
        return;
    }
    end = ktime_get();
    printk(KERN_INFO "Test ended at: %lld ns\n", ktime_to_ns(end));
    printk(KERN_INFO "Test execution time: %lld ns\n", ktime_to_ns(ktime_sub(end, start)));
}

EXPORT_SYMBOL(StartTest);

Setup("Kernel Test", {}, {})