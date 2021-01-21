#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");  /* Kernel needs this license. */

#define ENTRY_NAME "remember"
#define PERMS 0644
#define PARENT NULL

/* Function declarations */
ssize_t procfile_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos);

ssize_t procfile_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos);

/* Global variables go here */
char GLOBAL[80];
/* This global structure is necessary to bind the regular file read and write 
 * operations to our special "read" and "write" functions instead. Don't
 * modify. (structs in C are like objects in other languages.)
*/ 
static struct file_operations hello_proc_ops = {
   .owner = THIS_MODULE,
   .read = procfile_read,
   .write = procfile_write,
};


/* This function is called to create the special proc file entry on 
 * module load.  This file is created as /proc/helloworld. */
int hello_proc_init(void) {

   proc_create_data(ENTRY_NAME, 0, NULL, &hello_proc_ops, NULL);
   
   /* This message will print in /var/log/syslog or on the first tty. */
   printk("/proc/%s created\n", ENTRY_NAME);
   return 0;
}

/* This function is called when someone tries to READ from the file
 * /proc/helloworld. */
ssize_t procfile_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{

   /* Static variable?  What does this do inside a C function? Take a look 
    * at the accepted answer in the link below:
    * http://stackoverflow.com/questions/572547/what-does-static-mean-in-a-c-program
    */
   static int finished = 0;
   int ret;
   char ret_buf[80];

   /* Are we done reading? If so, we return 0 to indicate end-of-file */
   if (finished) {
	finished=0;
	return 0;
   }

   finished = 1;

   /* This message will print in /var/log/syslog or on the first tty. */
   printk("/proc/%s read called.\n", ENTRY_NAME);


   /* Take the string "Hello World!\n" and put it in ret_buf.  Copy ret_buf
      into the user-space buffer called buf.  buf is what gets
    * displayed to the user when they read the file. */
   
   ret = sprintf(ret_buf, GLOBAL);
   if(copy_to_user(buf, ret_buf, ret)) {
      ret = -EFAULT;  //failed, let's get out of here
   }

   /* Returning the number of characters returned to the reader. */
   return ret;
}

/* This function is called when someone tries to WRITE to the file
 * /proc/helloworld. */
ssize_t procfile_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{

    char *page; /* don't touch */
    
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
    printk("User has sent the value of %s\n", page);

    strcpy(GLOBAL, page);
    
    /* Free the allocated memory, don't touch. */
    vfree(page); 

    /* Return the number of bytes written to the file. */
    return count;
}

/* Called when module is unloaded.  Function removes the proc file and
 * prints a message to /var/log/syslog. */
void hello_proc_exit(void)
{
   remove_proc_entry(ENTRY_NAME, NULL);
   printk("Removing /proc/%s.\n", ENTRY_NAME);
}

/* Necessary module stuff.  Says init function is hello_proc_init and
 * exit function is hello_proc_exit. */
module_init(hello_proc_init);
module_exit(hello_proc_exit);

