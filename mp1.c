#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include "mp1_given.h"
#include "linux/list.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1

// ---------------------------------------------------------------------------------------------------------------

struct process_list {
   struct list_head list;
   unsigned long pid;
   unsigned long cpu_time;
};
struct process_list registered_processes;

/*
   file: pointer to /mp1/status
   buf: the data we want to fill with
   size: number of bytes to read from the file
   pos: the offset in the file

   return the size of the buffer we filled
*/
ssize_t proc_read_callback(struct file* file, char __user *buf, size_t size, loff_t* pos) {
   /*if (*pos != 0) {
      printk(KERN_ALERT "proc_read_callback(): pos offset != 0\n");
      return 0;
   }
   
   char data[size + 1];
   memset(buf_cpy, 0, size + 1);
   // write to data the linked list node data
   int bytes_read = 0;
   struct process_list* tmp;
   struct list_head* curr_pos;
   list_for_each(curr_pos, &registered_processes.list) {
      tmp = list_entry(curr_pos, struct process_list, list);
      unsigned long pid = tmp->pid;
      unsigned long cpu_time = tmp->cpu_time;

      int curr_bytes_read = sprintf(data + bytes_read, "%lu: %lu\n", pid, cpu_time);
      if (curr_bytes_read <= 0) {
         printk(KERN_ALERT "proc_read_callback(): Failed to copy node to data buffer\n");
         return 0;
      }
      bytes_read += curr_bytes_read;
   }
   data[bytes_read] = '\0';

   int success = copy_to_user(buf, data, bytes_read);
   if (success != 0) {
      printk(KERN_ALERT "proc_read_callback(): Failed to copy data to user space\n");
      return 0;
   }

   if (bytes_read != size) {
      printk("proc_read_callback(): ")
   }

   return bytes_read; // return the number of bytes that were read
   */
}

// ---------------------------------------------------------------------------------------------------------------

/*
   file: pointer to /mp1/status
   buf: the data we want to write to the file (aka: the PID)
   size: number of bytes we want to write to the file
   pos: the offset in buf

   return the number of bytes we wrote to the file
*/
ssize_t proc_write_callback(struct file* file, const char __user *buf, size_t size, loff_t* pos) {
   if (*pos != 0) {
      printk(KERN_ALERT "proc_write_callback(): pos offset != 0\n");
      return 0;
   }
   
   // buf belongs in user space, need to make copy of it
   char buf_cpy[size + 1];
   memset(buf_cpy, 0, size + 1);
   int success = copy_from_user(buf_cpy, buf, size + 1);
   buf_cpy[size] = '\0';
   if (success != 0) {
      printk(KERN_ALERT "proc_write_callback(): Failed to copy buf to kernel space\n");
      return 0;
   }

   // convert pid to long
   unsigned long pid = 0;
   success = kstrtoul(buf_cpy, 10, &pid);
   if (success != 0) {
      printk(KERN_ALERT "proc_write_callback(): Failed to convert pid to unsigned long\n");
      return 0;
   }
   //printk("proc_write_callback(): pid is %lu\n", pid);

   // get current cpu usage
   unsigned long cpu_time = 0;
   success = get_cpu_use((int)pid, &cpu_time);
   if (success != 0) {
      printk(KERN_ALERT "proc_write_callback(): PID is invalid || CPU time not successfully returned\n");
      return 0;
   }
   //printk("proc_write_callback(): cpu_time is %lu\n", cpu_time);

   // create new linked list node
   struct process_list* tmp = (struct process_list*) kmalloc(sizeof(struct process_list), GFP_KERNEL);
   tmp->pid = pid;
   tmp->cpu_time = cpu_time;
   list_add_tail(&(tmp->list), &(registered_processes.list));

   return size;
}

// ---------------------------------------------------------------------------------------------------------------

const struct proc_ops proc_fops = {
   .proc_read = proc_read_callback,
   .proc_write = proc_write_callback,
};

struct proc_dir_entry* proc_dir;
struct proc_dir_entry* proc_file;

// ---------------------------------------------------------------------------------------------------------------

// mp1_init - Called when module is loaded
int __init mp1_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE LOADING\n");
   #endif
      
   proc_dir = proc_mkdir("mp1", NULL);
   proc_file = proc_create("status", 0666, proc_dir, &proc_fops); 
   
   INIT_LIST_HEAD(&registered_processes.list);

   printk(KERN_ALERT "MP1 MODULE LOADED\n");
   return 0;   
}

// ---------------------------------------------------------------------------------------------------------------

// mp1_exit - Called when module is unloaded
void __exit mp1_exit(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
   #endif

   struct process_list *tmp;
   struct list_head *pos, *q;
   list_for_each_safe(pos, q, &registered_processes.list) {
      printk(KERN_ALERT "mp1_exit(): deleting elements of list\n");
      tmp = list_entry(pos, struct process_list, list);
      list_del(pos);
      kfree(tmp);
   }

   remove_proc_entry("status", proc_dir);
   remove_proc_entry("mp1", NULL);
   printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
