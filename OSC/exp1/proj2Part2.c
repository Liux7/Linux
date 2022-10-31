#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/types.h> // list_head struct.
#include <linux/list.h> // list defs.
//#include <linux/sched.h> // task_struct

#include <linux/sched/signal.h> //after 4.11 
struct task_struct *task;
struct list_head   *list;

int dfs_task_iter(struct task_struct *parent_task)
{
  struct list_head   *children_list;
  struct task_struct *child_task;

//  printk(KERN_INFO "part 2 dfs\n");

  if(parent_task == NULL) {
    return 1;
  }

  printk(KERN_INFO "dfs Name = %s. State = %ld. PID = %d.\n", parent_task->comm, parent_task->__state, parent_task->pid);

  list_for_each(children_list, &parent_task->children) { // for each child.
	child_task = list_entry(children_list, struct task_struct, sibling);
    dfs_task_iter(child_task);
  }

  return 0;
}

int simple_init(void)
{
  printk(KERN_INFO "Loading Module\n");
  dfs_task_iter(&init_task);
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

