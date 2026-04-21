#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

// Initialization function (called when the module is loaded)
static int __init hello_init(void) {
  printk(KERN_INFO "Hello, world!\n");
  return 0;  // Return 0 means success
}

// Exit function (called when the module is removed)
static void __exit hello_exit(void) { printk(KERN_INFO "Goodbye, world!\n"); }

// Register the functions
module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Hello World Module");
MODULE_AUTHOR("Tecadmin.net");
