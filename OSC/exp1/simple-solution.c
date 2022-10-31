#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/slab.h>
struct birthday 
{	
	int month;
	int day;
	int year;

       struct list_head list;
};

/**
 * The following defines and initializes a list_head object named birthday_list
 */
static LIST_HEAD(birthday_list);

int simple_init(void)
{
	/* the pointer for memory allocation */
	struct birthday *person_one;

	/* the pointer for list traversal */
	struct birthday *ptr;

       printk(KERN_INFO "Loading Module\n");

       person_one = kmalloc(sizeof(*person_one), GFP_KERNEL);
       person_one->month = 8;
       person_one->day = 13;
       person_one->year = 1995;
	INIT_LIST_HEAD(&person_one->list);

       /* add the new node */
	list_add_tail(&person_one->list, &birthday_list);

       person_one = kmalloc(sizeof(*person_one), GFP_KERNEL);
       person_one->month = 9;
       person_one->day = 2;
       person_one->year = 1998;
	INIT_LIST_HEAD(&person_one->list);

       /* add the new node */
	list_add_tail(&person_one->list, &birthday_list);

       person_one = kmalloc(sizeof(*person_one), GFP_KERNEL);
       person_one->month = 8;
       person_one->day = 12;
       person_one->year = 1963;
	INIT_LIST_HEAD(&person_one->list);

	/* add the new node */
	list_add_tail(&person_one->list, &birthday_list);

       person_one = kmalloc(sizeof(*person_one), GFP_KERNEL);
       person_one->month = 10;
       person_one->day = 22;
       person_one->year = 1963;
	INIT_LIST_HEAD(&person_one->list);

	/* add the new node */
	list_add(&person_one->list, &birthday_list);

       /* now traverse the list */

       list_for_each_entry(ptr, &birthday_list, list) {
               printk(KERN_INFO "Birthday: Month %d Day %d Year %d\n",ptr->month,ptr->day,ptr->year);
       }

       return 0;
}

void simple_exit(void) {
	struct  birthday *ptr, *next;

	printk(KERN_INFO "Removing Module\n");

	/* remove allocated memory */
	list_for_each_entry_safe(ptr, next, &birthday_list, list) {
		printk(KERN_INFO "Removing %d %d %d\n",ptr->month, ptr->day, ptr->year);
		list_del(&ptr->list);
		kfree(ptr);
	}

}

module_init( simple_init );
module_exit( simple_exit );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel Data Structures");
MODULE_AUTHOR("SGG");

