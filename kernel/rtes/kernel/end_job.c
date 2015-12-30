/*
 * 18-648 FAll 2015 LAB 3
 * =====================================================================
 * GROUP 10  -> Mengye Gong (mengyeg)
 *	     -> Nishant Parekh (nmparekh)
 *           -> Ziyuan Song (ziyuans)
 * PROGRAM:  		end_job.c 
 * DESCRIPTION: 	Syscall which suspends the calling thread until beginning of 
 * 			its next period T.
 * ======================================================================
 * AUTHOR : Nishant Parekh
 * ======================================================================
*/


#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <asm/current.h>
#include <linux/reserve.h>


/* syscall definition for end_job */
asmlinkage long sys_end_job(void){

	struct task_struct *mytask; 	//task struct to hold pointer to current task
	int retVal=0;			   //return value of syscall
	
	printk(KERN_ALERT "\tEND_JOB: In syscall .....\n");
	
	mytask =current;
	
	printk(KERN_ALERT "\tEND_JOB: BEFORE ENDJOB ----- Current PID = %d\n", current->pid);
	//check if reserve is suspended for a task  with active_reserve
		if(current->state != TASK_UNINTERRUPTIBLE){
		
			set_current_state(TASK_UNINTERRUPTIBLE);
			set_tsk_need_resched(current);
			retVal=1;
		}
		printk(KERN_ALERT "\tEND_JOB: AFTER ENDJOB ----- Current PID = %d\n", current->pid);


	/*
	else if(current->myreserve.suspended){
		retVal=wakeup_task(current);
		printk(KERN_ALERT "\tEND_JOB: AFTER WAKEUP  ----- Current PID = %d, State= %lu *SUSPENDED**= %d\n", current->pid, current->state, current->myreserve.suspended);
	}
		*/
	
	
	printk(KERN_ALERT "\tEND_JOB: EXITING .....\n");
	return retVal;
}


/* syscall definition for wake_up */
asmlinkage long sys_wake_up_job(void){

	int retVal=0;			   //return value of syscall
	
	printk(KERN_ALERT "\tWAKE_UP: In syscall .....\n");
	
	
	printk(KERN_ALERT "\tWAKE_UP: BEFORE WAKE_UP ----- Current PID = %d\n", current->pid);
	//check if reserve is suspended for a task  with active_reserve
		wake_up_process(current);
		
		printk(KERN_ALERT "\tWAKE_UP_JOB: AFTER WAKE_UP ----- Current PID = %d\n", current->pid);


	/*
	else if(current->myreserve.suspended){
		retVal=wakeup_task(current);
		printk(KERN_ALERT "\tEND_JOB: AFTER WAKEUP  ----- Current PID = %d, State= %lu *SUSPENDED**= %d\n", current->pid, current->state, current->myreserve.suspended);
	}
		*/
	
	
	printk(KERN_ALERT "\tEND_JOB: EXITING .....\n");
	return retVal;
}


