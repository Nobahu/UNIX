#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nobahu");
MODULE_DESCRIPTION("Tomsk State University Module");
MODULE_VERSION("1.0");

static int __init tsuInit(void)
{
	printk(KERN_INFO "Welcome to the Tomsk State University\n");
	return 0;
}

static void __exit tsuExit(void)
{
	printk(KERN_INFO "Tomsk State University forever!\n");
}

module_init(tsuInit);
module_exit(tsuExit);

