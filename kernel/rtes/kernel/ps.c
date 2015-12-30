/**************************************************************************
 * 18-648 FAll 2015 LAB 1
 * =====================================================================
 * GROUP 10  	-> Mengye Gong    (mengyeg) 
 *	     	-> Nishant Parekh (nmparekh)
 *          	-> Ziyuan Song    (ziyuans)
 * PROGRAM:  	ps.c 
 * AUTHOR:	Nishant Parekh
 * DESCRIPTION: Process Information Syscalls (Lab 1 Section 4.5.4)
*****************************************************************************/
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/mystruct.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

/* syscall definition for count_rt_threads */
asmlinkage long sys_count_rt_threads(void){

	struct task_struct *t, *tasks; 	//task struct of kernel from sched.h
	int count=0;			   //hold the count of rt threads
	
	printk(KERN_ALERT "COUNT_RT_THREADS[INFO]: Calling Syscall COUNT_RT_THREADS\n");
	do_each_thread(t,tasks)
   	{ 	
		//RealTime Priority between 1 and 100
		if(tasks->rt_priority > 0 && tasks->rt_priority < 100){
			++count;
    			printk(KERN_ALERT"COUNT_RT_THREADS[INFO]: TGID:[%d] PID:[%d] PRIO:%d COMM:%s\n",tasks->tgid,tasks->pid,tasks->rt_priority,tasks->comm);
		}
    	}while_each_thread(t,tasks);

	// return the number of rt threads
    	printk("KERNEL ALERT: count= %d\n",count);
	return count;
}

/* syscall definition for list_rt_threads */
asmlinkage long sys_list_rt_threads(struct mytask_struct* t, int count){
	struct task_struct *t1, *task;    		//task struct of kernel from sched.h
	struct mytask_struct *temp=NULL;   	//pointers to my own task struct data structure
	int cnt=0;        //a simple counter to track number of rt threads printed
	temp=t;  	//assign to a tmp pointer 
	
	printk(KERN_ALERT "LIST_RT_THREADS[INFO]: Calling Syscall LIST_RT_THREADS\n");
	do_each_thread(t1,task){ 

		//real time tasks are always in teh range of 1 to 99
		if(task->rt_priority >0 && task->rt_priority < 100){
    			printk(KERN_ALERT "LIST_RT_THREADS[INFO]: TGID:[%d] PID:[%d] PRIO:%d COMM:%s\n",task->tgid, task->pid, task->prio,task->comm);
	    		//if list is not empty copy the attributes of the task struct to my own task struct
			if(temp!=NULL){

				if(copy_to_user(&(temp->task_pid),&(task->pid),sizeof(pid_t))){
					printk("LIST_RT_THREADS[INFO]: [%d] pid= %d", cnt+1, temp->task_pid);
					return -EFAULT;
				}
				if(copy_to_user(&(temp->task_tid),&(task->tgid),sizeof(pid_t))){
					printk("LIST_RT_THREADS[INFO]: [%d] tid= %d", cnt+1, temp->task_tid);
					return -EFAULT;
				}
				if(copy_to_user(&(temp->task_prio),&(task->rt_priority),sizeof(int))){
					printk("LIST_RT_THREADS[INFO]: [%d] prio= %d", cnt+1, temp->task_prio);
					return -EFAULT;	
				}
				if(copy_to_user(&(temp->task_comm),&(task->comm),sizeof(char[16]))){
					printk("LIST_RT_THREADS[INFO]: [%d] comm= %s", cnt+1, temp->task_comm);
					return -EFAULT;
				}
				if(copy_to_user(&(temp->isActive),&(task->reservation.isActive),sizeof(bool))){
                                         printk("LIST_RT_THREADS[INFO]: [%d] comm= %s", cnt+1, temp->isActive?"true":"false");
                                          return -EFAULT;
                                  }

				temp=temp->next;
				++cnt;
			}
		}
	}while_each_thread(t1,task);
	return 0;
}
