#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>


MODULE_LICENSE("GPL");  /* Kernel needs this license. */

#define ENTRY_NAME "penguin"
#define PERMS 0644
#define PARENT NULL

/* Function declarations */
ssize_t procfile_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos);

ssize_t procfile_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos);

static int t_penguin(void* data);



/* Global variables go here */
struct mutex my_mutex;

int PRINT_SLOT_ARRAY[11];
int QUEUE_POSITION = 0;
int TOTAL_JOBS_COUNTER = 0;


static struct file_operations hello_proc_ops = {
   .owner = THIS_MODULE,
   .read = procfile_read,
   .write = procfile_write,
};

struct task_struct *t; /* Printer thread task struct (use to start/stop)*/


/* The kitchen thread will run this function.  The thread will stop
 * when either kitchen_stop(t) is called or else the function ends. */

static int t_penguin(void* data) {

   while(!kthread_should_stop()) {

      if(mutex_lock_interruptible(&my_mutex))
         return -ERESTARTSYS;

/*Iterates through the printer slot, and each to see if there is anything placed in the slot, if there is, it sleeps for the appropriate amount of seconds, sets the slot back to 0, and increases the counter for jobs processed. */

      for (QUEUE_POSITION = 0; QUEUE_POSITION < 11; QUEUE_POSITION++) {
         
         /* Sleep one second */
         ssleep(1);

         if (PRINT_SLOT_ARRAY[QUEUE_POSITION] == 1) {
            ssleep(1);
            PRINT_SLOT_ARRAY[QUEUE_POSITION] = 0;
            TOTAL_JOBS_COUNTER++;
         }
         if (PRINT_SLOT_ARRAY[QUEUE_POSITION] == 2) {
            ssleep(2);
            PRINT_SLOT_ARRAY[QUEUE_POSITION] = 0;
            TOTAL_JOBS_COUNTER++;
         }
         if (PRINT_SLOT_ARRAY[QUEUE_POSITION] == 3) {
            ssleep(3);
            PRINT_SLOT_ARRAY[QUEUE_POSITION] = 0;
            TOTAL_JOBS_COUNTER++;
         }
         if (PRINT_SLOT_ARRAY[QUEUE_POSITION] == 4) {
            ssleep(4);
            PRINT_SLOT_ARRAY[QUEUE_POSITION] = 0;
            TOTAL_JOBS_COUNTER++;
         }
         if (PRINT_SLOT_ARRAY[QUEUE_POSITION] == 5) {
            ssleep(10);
            PRINT_SLOT_ARRAY[QUEUE_POSITION] = 0;
            TOTAL_JOBS_COUNTER++;
         }
         mutex_unlock(&my_mutex);
      }
   }
   return 0;
}


int hello_proc_init(void) {
   
   proc_create_data(ENTRY_NAME, 0, NULL, &hello_proc_ops, NULL);
   
   /* This message will print in /var/log/syslog or on the first tty. */
   printk("/proc/%s created\n", ENTRY_NAME);
   
   mutex_init(&my_mutex);

   /* Start the printer -- move out of here later. */
   t = kthread_run(t_penguin, NULL, "penguin_thread");

   return 0;
}

ssize_t procfile_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{

   static int finished = 0;
   int ret;
   char ret_buf[80];
   
   const char *currentTask[6];
   currentTask[0] = "nothing";
   currentTask[1] = "1 page document";
   currentTask[2] = "2 page document";
   currentTask[3] = "3 page document";
   currentTask[4] = "4 page document";
   currentTask[5] = "maintanance";

   /* Are we done reading? If so, we return 0 to indicate end-of-file */
   if (finished) {
	finished=0;
	return 0;
   }

   finished = 1;

   /* This message will print in /var/log/syslog or on the first tty. */
   printk("/proc/%s read called.\n", ENTRY_NAME);
   
   if(mutex_lock_interruptible(&my_mutex))
      return -ERESTARTSYS;

   ret=sprintf(ret_buf, "Processing %s at slot %i. Total jobs processed: %i\n", currentTask[PRINT_SLOT_ARRAY[QUEUE_POSITION]], QUEUE_POSITION, TOTAL_JOBS_COUNTER);

   mutex_unlock(&my_mutex);

   if(copy_to_user(buf, ret_buf, ret)) {
      ret = -EFAULT;  
   }
   
   return ret;
}

ssize_t procfile_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    char *page; /* don't touch */
    int my_data = 0;
    int slotInt = 0;
    
    /* Allocating kernel memory, don't touch. */
    page = (char *) vmalloc(count);
    if (!page)
       return -ENOMEM;   

    /* Copy the data from the user space.  Data is placed in page. */ 
    if (copy_from_user(page, buf, count)) {
       vfree(page);
       return -EFAULT;
    }

   /* Now do something with the data, here we just print it */
    sscanf(page,"%d",&my_data);
    printk("User has sent the value of %d\n", my_data);
    
    if(mutex_lock_interruptible(&my_mutex))
      return -ERESTARTSYS;

    for (slotInt = 0; slotInt < 11; slotInt++) {
       if (PRINT_SLOT_ARRAY[slotInt] == 0) {
          PRINT_SLOT_ARRAY[slotInt] = my_data;
          break;
       }
    }
    
    
   
    /* Free the allocated memory, don't touch. */
    vfree(page); 

    mutex_unlock(&my_mutex);
    return count;
    
}

void hello_proc_exit(void)
{

   /* Will block here and wait until kthread stops */
   kthread_stop(t);

   remove_proc_entry(ENTRY_NAME, NULL);
   printk("Removing /proc/%s.\n", ENTRY_NAME);
}

module_init(hello_proc_init);
module_exit(hello_proc_exit);

