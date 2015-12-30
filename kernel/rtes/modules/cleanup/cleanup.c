/*********************************************************************** 
 * 18-648 FAll 2015 LAB 1
 * =====================================================================
 * GROUP 10  -> Mengye Gong (mengyeg)
 *	     -> Nishant Parekh (nmparekh)
 *           -> Ziyuan Song (ziyuans)
 * PROGRAM:  	cleanup.c
 * AUTHOR: 	Nishant Parekh
 * DESCRIPTION: Loadable Kernel Module to override syscalls when a process 
 *		exits and print the paths of any open files to kernel log. 
 *		(Lab 1 Section 4.5.3)
 ***************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <asm/current.h>
#include <asm/system.h>
#include <asm/page.h>
#include <linux/kallsyms.h>
#include <linux/slab.h>
#include <linux/fdtable.h>
#include <linux/limits.h>


static char *comm="sloppyapp";//Check for this substring

module_param(comm,charp,0000);
MODULE_PARM_DESC(mystring, "String to check for in process description\n");

unsigned int *table_address;
asmlinkage long (*temp1)(int);
asmlinkage long (*temp2)(int);

asmlinkage int printdetails(int det)
{

	int openfd=0;
	int first=0;
	struct fdtable *fd_ptr;
	char *pathl;
	char buffer[256];

	if(strstr(current->comm,comm))
	{
		/* locking the block */
		rcu_read_lock();
		fd_ptr=files_fdtable(current->files);// asm/current.h (points to the File table for the current process.)


		/* This loop iterates through the fs table and gets the path */
		for(openfd=0;(fd_ptr->fd[openfd])!=NULL;openfd++)
		{
			if(first==0){	
				printk(KERN_ALERT"cleanup: process '%s' (PID %d) did not close files:\n",current->comm,current->pid);
			}
			pathl=d_path((&(fd_ptr->fd[openfd]->f_path)),buffer,256);
			printk(KERN_INFO"cleanup: %s\n",pathl);
			first++;
		}
		rcu_read_unlock();
	}
	return temp1(det);
}


/* This function prints the open file details */
asmlinkage int printdetailsg(int det)
{

	int openfd;
	int first=0;
	struct fdtable *fd_ptr;
	char *pathl;
	char *buffer;


	if(strstr(current->comm,comm))
	{
		/* locking the block */
		rcu_read_lock();
		buffer=kmalloc(256,GFP_KERNEL);
		fd_ptr=files_fdtable(current->files);

		/* This loop iterates through the fs table and gets the path */
		for(openfd=0;(fd_ptr->fd[openfd])!=NULL;openfd++)
		{
			if(first==0){
				printk(KERN_ALERT"cleanup: process '%s' (PID %d) did not close files:\n",current->comm,current->pid);
			}
			pathl=d_path((&(fd_ptr->fd[openfd]->f_path)),buffer,PATH_MAX);
			printk(KERN_INFO"cleanup: %s\n",pathl);
			first++;
		}
		kfree(buffer);

		rcu_read_unlock();
	 }
	return temp2(det);
}


/* This function finds the address of the sycall
   table and returns to the calling funtion */

void *tablefind(void)
{

	unsigned long tempadd;
	unsigned int *syscall_table;

	/* kallsyms_lookup_name is defined in kallsyms.c */
	tempadd=kallsyms_lookup_name("sys_call_table");
	syscall_table=(void *) tempadd;
	return syscall_table;
}

/* In the init module we corrupt the syscall table to 
   have our function's address instead of exit's address */

int cleanup_init(void)
{

	table_address=(unsigned int *)tablefind();
	if(table_address== NULL){
		printk(KERN_INFO"table address is null");
	}
	else if(table_address!=NULL){

		temp1=table_address[__NR_exit];
		temp2=table_address[__NR_exit_group];
		table_address[__NR_exit]=(unsigned long *)printdetails;
		table_address[__NR_exit_group]=(unsigned long *)printdetailsg;

	}

	return 0 ;
}

void cleanup_exit(void)
{
	table_address[__NR_exit]=temp1;
	table_address[__NR_exit_group]=temp2;
}

module_init(cleanup_init);
module_exit(cleanup_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NISHANT");
MODULE_DESCRIPTION("CLEANUP FUNCTION WHICH PRINTS THE OPEN FILE DETAILS!");
