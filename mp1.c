#define LINUX

#include "mp1_given.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include "linux/list.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1

// ---------------------------------------------------------------------------------------------------------------

struct process_list {
   struct list_head list;
   int pid;
   unsigned long cpu_time;
};
struct process_list registered_processes;
struct timer_list* timer;
struct work_struct* work;
spinlock_t lock;

// ---------------------------------------------------------------------------------------------------------------
void update_cpu_times(struct work_struct *work) {
   //printk(KERN_ALERT "work function should be called every 5 seconds %lu\n", jiffies);

   struct process_list* tmp;
   struct list_head* curr_pos;
   
   spin_lock_irq(&lock);
   list_for_each(curr_pos, &registered_processes.list) {
      tmp = list_entry(curr_pos, struct process_list, list);
      int pid = tmp->pid;

      unsigned long new_cpu_time = 0;
      int success = get_cpu_use((int)pid, &new_cpu_time);
      if (success == -1) { // remove node from the list
         list_del(curr_pos);
         kfree(tmp);
      }
      else {
         //printk("update_cpu_time(): new cpu time is %lu\n", new_cpu_time);
         tmp->cpu_time = new_cpu_time;
      }
   }
   spin_unlock_irq(&lock);

   mod_timer(timer, jiffies + 5*HZ);
}

void timer_callback(struct timer_list* data) {
   // puts the job into the kernel-global workqueue
   schedule_work(work);
}

/*
   file: pointer to /mp1/status
   buf: the data we want to fill with
   size: number of bytes to read from the file
   pos: the offset in the file

   return the size of the buffer we filled
*/
ssize_t proc_read_callback(struct file* file, char __user *buf, size_t size, loff_t* pos) {
   if (*pos != 0) { // CHECK: is it safe to just do this????
      return 0;
   }
   
   char* data = kmalloc(size + 1, GFP_KERNEL);
   memset(data, 0, size + 1); // CHECK: + 1 is correct????

   // write to data the linked list node data
   int bytes_read = 0;
   struct process_list* tmp;
   struct list_head* curr_pos;

   spin_lock_irq(&lock);
   list_for_each(curr_pos, &registered_processes.list) {
      tmp = list_entry(curr_pos, struct process_list, list);
      int pid = tmp->pid;
      unsigned long cpu_time = tmp->cpu_time;

      int curr_bytes_read = sprintf(data + bytes_read, "%d: %lu\n", pid, cpu_time);
      bytes_read += curr_bytes_read;
   }
   spin_unlock_irq(&lock);
   data[bytes_read] = '\0';

   int success = copy_to_user(buf, data, bytes_read + 1);
   if (success != 0) {
      //printk(KERN_ALERT "proc_read_callback(): Failed to copy data to user space\n");
      return 0;
   }

   *pos += bytes_read;
   return bytes_read; // return the number of bytes that were read
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
      return -EFAULT;
   }

   // convert pid to int
   int pid = 0;
   success = kstrtoint(buf_cpy, 10, &pid);
   if (success != 0) {
      printk(KERN_ALERT "proc_write_callback(): Failed to convert pid to int\n");
      return -EFAULT;
   }

   // get current cpu usage
   unsigned long cpu_time = 0;

   // create new linked list node
   struct process_list* tmp = (struct process_list*) kmalloc(sizeof(struct process_list), GFP_KERNEL);
   tmp->pid = pid;
   tmp->cpu_time = cpu_time;
   INIT_LIST_HEAD(&tmp->list);

   spin_lock_irq(&lock);
   list_add_tail(&(tmp->list), &(registered_processes.list));
   spin_unlock_irq(&lock);

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

   spin_lock_init(&lock);

   INIT_LIST_HEAD(&registered_processes.list);

   timer = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
   timer_setup(timer, timer_callback, 0);
   mod_timer(timer, jiffies + 5*HZ);

   // update_cpu_times() is the function to be scheduled in the workqueue
   // creates a workqueue with the name work
   work = kmalloc(sizeof(struct work_struct), GFP_KERNEL);
   INIT_WORK(work, update_cpu_times); 

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

   del_timer_sync(timer);
   kfree(timer);
   
   // terminates the work in the queue or blocks until the callback has finished 
   // (if the work is already in progress in the handler)
   cancel_work_sync(work);
   kfree(work);

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
