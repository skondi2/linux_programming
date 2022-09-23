# CS 423 MP1: Introduction to Linux Kernel Programming

Creates a kernel module to track the usertime of userspace processes. Processes register with the kernel module by writing their PID to file named /proc/mp1/status. Reading from /proc/mp1/status will display the CPU time of each process that has registered with the module.

## Stored State:
The module stores registered processes and their CPU time with a linked list. When a process is registering its PID, a proc write callback is called. Every time a new process is registered, this write callback function is called and a new node is is added to the linked list. When the /proc/mp1/status file is read from, this linked list is traversed and the PID and CPU usage stored in the node is returned. 

Accessing and modifying this linked list is thread-safe because these operations are protected by a spinlock. When the spinlock is acquired, interrupts are disabled on that processor.

## Updating CPU Time:
The CPU time of each registered process is updated every 5 seconds since the initialization of the module. The module has a timer that expires every 5 seconds. When this timer times out, the CPU time stored in every node in the linked list must be updated. This task is stored in a workqueue, and a worker thread will traverse the linked list and recalculate the CPU time.

## Example User Process: 
A sample userapp that performs a factorial computation is also implemented to test the module
