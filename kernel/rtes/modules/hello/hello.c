#include "linux/module.h"
#include "linux/kernel.h"
int init_module(void) {
	printk(KERN_INFO "Hello, world! Kernel-space -- the land of the free and the home of the brave\n");
	return 0;
}
void cleanup_module(void) {
	printk(KERN_INFO "Good Bye!\n");
}

