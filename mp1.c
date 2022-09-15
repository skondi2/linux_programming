#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include "mp1_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1

ssize_t proc_read_callback(struct file* file, char __user *buf, size_t size, loff_t* pos) {
   return 0;
}

ssize_t proc_write_callback(struct file* file, const char __user *buf, size_t size, loff_t* pos) {
   return 0;
}

struct proc_dir_entry* proc_dir;
struct proc_dir_entry* proc_file;

const struct proc_ops proc_fops = {
   .proc_read = proc_read_callback,
   .proc_write = proc_write_callback,
};

// mp1_init - Called when module is loaded
int __init mp1_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE LOADING\n");
   #endif
      
   proc_dir = proc_mkdir("mp1", NULL);
   if (!proc_dir) {
      printk(KERN_ALERT "Failed to create mp1 directory\n");
   }
   proc_file = proc_create("status", 0666, proc_dir, &proc_fops); 
   if (!proc_file) {
      printk(KERN_ALERT "Failed to create status file\n");
   }

   printk(KERN_ALERT "MP1 MODULE LOADED\n");
   return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp1_exit(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
   #endif
   remove_proc_entry("status", proc_dir);
   remove_proc_entry("mp1", NULL);
   printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
