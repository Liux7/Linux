#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h> 
#include <linux/list.h> 

//#include <linux/sched.h> //before 4.11 
#include <linux/sched/signal.h> //after 4.11 
struct task_struct *task;

int simple_init(void)
{
  printk(KERN_INFO "Loading Module\n");

  for_each_process(task) {
    printk(KERN_INFO "process for each Name = %s. State = %ld. PID = %d.!\n", task->comm, task->__state, task->pid);
  }

  return 0;
}

void simple_exit(void) {
  printk(KERN_INFO "Removing Module\n");
}

module_init( simple_init );
module_exit( simple_exit );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("SGG");

