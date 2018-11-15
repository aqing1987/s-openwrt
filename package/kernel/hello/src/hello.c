#include <linux/kernel.h> /* Needed for KERN_INFO */
#include <linux/init.h> /* Needed for the macros */
#include <linux/module.h> /* Needed by all modules */

static int __init hello_init(void)
{
    printk(KERN_INFO "hello world enter\n");
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello world exit\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("Neil Chen <jingsi.chen@qq.com>");

/* Get rid of taint message by declaring code as GPL */
MODULE_LICENSE("GPL");
