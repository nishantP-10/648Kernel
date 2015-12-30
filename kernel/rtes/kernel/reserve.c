/* =====================================================================
 * 18-648 FALL 2015 LAB 2
 * =====================================================================
 * GROUP 10 -> Mengye Gong     (mengyeg)
 *          -> Nishant Parekh  (nmparekh)
 *          -> Ziyuan Song     (ziyuans)
 * FILE:              reserve.c 
 * DESCRIPTION:       Reservation Framework Syscalls (Lab 2 Section 2.2.2)
 * ========================================================================
 * AUTHOR : NISHANT PAREKH (nmparekh)
 * ========================================================================
 */

#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/utimeasure.h>
#include <asm/div64.h>
#include <linux/utility.h>
#include <linux/reserve.h>


/* period hrtimer callback function */
//long lookup_table[9] = {16887, 29517, 48564, 64708, 79544, 102329, 119984, 138749, 158593};
enum hrtimer_restart period_timer_callback( struct hrtimer *timer);
void start_budget_timer(struct hrtimer* timer, struct timespec length);
void start_period_timer(struct hrtimer* timer, struct timespec length);

long long unsigned timespec_to_ms(struct timespec * val){
 				  long long operand1long = ((long long)(timespec_to_ns(val)));
                                  long long operand2long = (long long)(1000000);
                                  //printk("ope1: %lld\n", operand1long);
                                  //printk("ope2: %lld\n", operand2long);
                                  do_div(operand1long, operand2long);
                                  //printk("Finish calc divide: %lld\n", operand1long);
				  return operand1long;	
}

/* syscall definition for set_reserve */
asmlinkage long sys_set_reserve(pid_t tid, struct timespec *C, struct timespec *T, int cpuid){

	int retval=0; //return values for various functions
	struct task_struct *x,*y,*reserve_task=NULL; //pointer to the task for which we set reserve
	cpumask_t mask; //cpu mask
	unsigned long flags; //irqsave flags for spinlock
	struct reserve_struct *res;
	int cpu_x = -1;
	unsigned long long freq=0;
	struct cpufreq_policy *zero_policy = NULL, *sys_core_policy = NULL;
	int sys_core=0;
	static char *cpufreq_sysfs_governor_path="/sys/devices/system/cpu/cpu%i/cpufreq/scaling_governor";
	char buf[128];
	mm_segment_t old_fs;
	struct file *scaling_gov = NULL;
	loff_t offset = 0;

	

	printk(KERN_INFO "SET_RESERVE: Calling Syscall\n");
	//1a. check sanity of arguments C, T and cpuid here
	if(timespec_compare(T,C)<0){
		printk(KERN_INFO "SET_RESERVE: C cannot be greater than T!\n");
		return -EINVAL;
	}
	if((cpuid <0 || cpuid >3 ) && cpuid!=-1){
		printk(KERN_INFO "SET_RESERVE: CPUID can only be in the range 0 to 3 and -1 for Bin Packing\n");
		return -EINVAL;
	}

	//1b. check if it is a callling thread
	
	if(tid==0){
		//implies calling thread, so initialize the task_struct to current
		reserve_task= current;
		tid=reserve_task->pid;
	}else{
		// we need to find the task_struct with the given tid
		//check if that thread exists in the system or not
		rcu_read_lock();
		do_each_thread(x,y){
			if(y->pid == tid){
				reserve_task=y;
				printk(KERN_INFO "SET_RESERVE: Found my task_struct...\n");
				goto loopexit;
			}
		}while_each_thread(x,y);

loopexit:
	 rcu_read_unlock();
	}//end of else == not calling thread

	if(!reserve_task){
		printk(KERN_INFO "SET_RESERVE: task_struct NOT found!!\n");
		return -ESRCH;
	} else {
		res = &(reserve_task->reservation);
	}

	cpuid = which_core_sched(C, T, cpuid, reserve_task->pid);
	//if(is_sched(C,T,cpuid,reserve_task->pid))
	if (cpuid >= 0)
		printk(KERN_ALERT "Is Schedulable\n");
	else{
		printk(KERN_ALERT "Not Schedulable\n");
		return -EBUSY;
	}
	// LAB 4
	// check if the available cpu is online. If not make it online.
	for_each_online_cpu(cpu_x) {
		if (cpu_x == cpuid) {
			break;
		}
	}
	if (cpu_x != cpuid && cpuid != 0) {
		cpu_up(cpuid);
	}
	
	
	//2. pin the thread to the given core BUT BEFORE THAT CHECK FOR SCHEDULABILITY 
	
	cpumask_clear(&mask);
	cpumask_set_cpu(cpuid, &mask);
	if((retval=sched_setaffinity(tid,&mask)) < 0){
		printk(KERN_ALERT "SET_RESERVE: sched_setaffinity failed!!\n");
		return -EINVAL;
	}
	
	// 3. NOW associate the C and the T value with the thread
	res->tid = tid;
	printk(KERN_ALERT "RESERVE THREAD ID %d\n", (int)res->tid);	
	if(!res->isActive){
		res->thattr= kmalloc(sizeof(struct thread_attr),GFP_KERNEL);
		retval=util_init(tid);
		retval = energy_init(tid); //LAB 4
		spin_lock_init(&(res->reserve_lock));
		spin_lock_irqsave(&(res->reserve_lock), flags);
		print_lock();
		//printk(KERN_ALERT "First Set reservation\n");
	} else {
		spin_lock_irqsave(&(res->reserve_lock), flags);
		print_lock();
		hrtimer_cancel(&(res->budget_timer));
		hrtimer_cancel(&(res->period_timer));
		//printk(KERN_ALERT "More than once to set reservation\n");	
	}
	res->isActive = true;
	res->isSuspend = false;
	res->isRunning = false;
	res->core_num = cpuid;
	res->C_reserve = *C;
	res->T_reserve = *T;
	res->budget_remain = res->C_reserve;
	res->period_runcount = 0;
	reserve_task->rt_priority=50;
	
	start_period_timer(&(res->period_timer), res->T_reserve);	
	if (reserve_task->state == TASK_RUNNING) {
		start_budget_timer(&(res->budget_timer), res->C_reserve);
		res->isRunning = true;
		printk(KERN_ALERT "Task state in 0 state\n");
	} else if (reserve_task->state == TASK_UNINTERRUPTIBLE){
                wake_up_process(reserve_task);
		printk(KERN_ALERT "Task state in 2 state\n");
        } else {
		printk(KERN_ALERT "Set Reserve in other states\n");
	}
	spin_unlock_irqrestore(&(res->reserve_lock), flags);
	print_unlock();
	freq = sysclock_calculation();
	printk(KERN_ALERT "sysclock Frequency factor is %llu", freq);
	


	zero_policy = cpufreq_cpu_get(0);
	if(zero_policy){
		if(strcmp(zero_policy->governor->name,"sysclock") == 0){		
			
			for_each_online_cpu(sys_core) {
	 			
				memset(buf,0,sizeof(buf));
         			sprintf(buf, cpufreq_sysfs_governor_path, sys_core);
				old_fs = get_fs();         
	 			set_fs(KERNEL_DS);	
				scaling_gov = filp_open(buf, O_RDWR, 0);
				if ( IS_ERR_OR_NULL(scaling_gov) ){
                                	printk(KERN_ALERT "cpufreq_set_userspace_governor: open %s fail\n",buf);
                                	return -1;
                        	}
				
				if(scaling_gov->f_op != NULL && scaling_gov->f_op->write != NULL) {
					scaling_gov->f_op->write(scaling_gov,"sysclock",8,&offset);
				}	
				set_fs(old_fs);
			}

			sys_core=0;
			for_each_online_cpu(sys_core) {
				sys_core_policy = cpufreq_cpu_get(sys_core);
				if(sys_core_policy){
					printk(KERN_ALERT "calling governor\n");
					(*sys_core_policy->governor->governor)(sys_core_policy,3);	
				}
				cpufreq_cpu_put(sys_core_policy);
			}
		}
	}
	cpufreq_cpu_put(zero_policy);

	printk(KERN_INFO "SET_RESERVE: Exiting..................\n");
	return retval;
}



asmlinkage long sys_cancel_reserve(pid_t tid){

	int ret_cancel = 0;
	struct task_struct *x,*y,*reserve_task=NULL;
	unsigned long flags;
	int core_num = -1;
	unsigned long long freq = 0;
	struct cpufreq_policy *zero_policy = NULL , *sys_core_policy = NULL;
	int sys_core=0;
	//1. check sanity of arguments here	
	if(tid==0){
		//implies calling thread, so initialize the task_struct to current
		reserve_task= current;
	}
	else{
		// we need to find the task_struct with the given tid
		/* locking this block */
		rcu_read_lock();
		//check if that thread exixts in the system or not
		do_each_thread(x,y){
			if(y->pid == tid){
				reserve_task=y;
				printk(KERN_INFO "CANCEL_RESERVE: Found my task_struct...\n");
				goto exitloop;
			}
		}while_each_thread(x,y);
exitloop:		
	        rcu_read_unlock();
	}
	if(!reserve_task) {
		printk(KERN_INFO "CANCEL_RESERVE: task_struct NOT found!!\n");
		return -ESRCH;
	}

	spin_lock_irqsave(&reserve_task->reservation.reserve_lock, flags);        
	reserve_task->reservation.isSuspend = false;
        reserve_task->reservation.isRunning = false;
	core_num = reserve_task->reservation.core_num;
        reserve_task->reservation.core_num = -1;
        reserve_task->reservation.budget_remain.tv_sec = 0;
        reserve_task->reservation.budget_remain.tv_nsec = 0;
	reserve_task->reservation.C_reserve.tv_sec = 0;
        reserve_task->reservation.C_reserve.tv_nsec = 0;
        reserve_task->reservation.T_reserve.tv_sec = 0;
	reserve_task->reservation.T_reserve.tv_nsec = 0;
        reserve_task->reservation.period_runcount = 0;
	if(reserve_task->reservation.isActive == true){
		reserve_task->reservation.isActive = false;
		util_delete(tid);
		energy_delete(tid);     //LAB 4
		kfree(reserve_task->reservation.thattr);
	} else{
		printk(KERN_INFO "CANCEL_RESERVE: Unable to cancel! Reservation not set!!\n");
		spin_unlock_irqrestore(&reserve_task->reservation.reserve_lock, flags);
		return -1;
	}



	if (hrtimer_active(&(reserve_task->reservation.budget_timer))) { 
		hrtimer_cancel(&(reserve_task->reservation.budget_timer));
	}
	if (hrtimer_active(&(reserve_task->reservation.period_timer))) {
		hrtimer_cancel(&(reserve_task->reservation.period_timer));
	}
	// LAB4
	// If there is no reservation on this core. turn off this cpu.
	spin_unlock_irqrestore(&reserve_task->reservation.reserve_lock, flags);
	if (core_num != -1 && core_num != 0 && !has_res_on_core(core_num)) {
		printk(KERN_ALERT "core_num %d turned off\n", core_num);
                cpu_down(core_num);
        }
	printk(KERN_ALERT "Outside if\n");
	freq= sysclock_calculation();
	printk(KERN_ALERT "sysclock frequency factor is %llu",freq);


	zero_policy = cpufreq_cpu_get(0);
	if(strcmp(zero_policy->governor->name,"sysclock") == 0){
		sys_core=0;
		for_each_online_cpu(sys_core) {
			sys_core_policy = cpufreq_cpu_get(sys_core);
			if(sys_core_policy)
				(*sys_core_policy->governor->governor)(sys_core_policy,3);	
			cpufreq_cpu_put(sys_core_policy);
		}
	}
	cpufreq_cpu_put(zero_policy);
	return ret_cancel;
}

void start_budget_timer(struct hrtimer* timer, struct timespec length) {
	if (hrtimer_active(timer)) {
//		printk(KERN_ALERT "budget_timer is active\n");
		return;
	} else {
//		printk(KERN_ALERT "start with length %llu\n", timespec_to_ms(&length));
		if (timespec_to_ms(&length) <= 0) {
			length.tv_nsec = 0;
			length.tv_sec = 0;
			return;
		}
	}
	hrtimer_init(timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED);
	timer->function = &budget_timer_callback;
        if(hrtimer_start(timer, timespec_to_ktime(length), HRTIMER_MODE_REL_PINNED)) {
		printk(KERN_ALERT "start budget_timer fail\n");	
	} else {
		//printk(KERN_ALERT "start budget_timer success\n");
	}
}
void start_period_timer(struct hrtimer* timer, struct timespec length) {
	if (hrtimer_active(timer)) {
//                printk(KERN_ALERT "period_timer is active \n");
		return;
        }
        hrtimer_init(timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED);
        timer->function = &period_timer_callback;
        if (hrtimer_start(timer, timespec_to_ktime(length), HRTIMER_MODE_REL_PINNED)) {
		printk(KERN_ALERT "start period_timer fail\n");
	} else {
		//printk(KERN_ALERT "start peirod_timer sucess\n");
	}
}
/* Callback function for hrtimer to set hrtimer restart or no restart */
enum hrtimer_restart period_timer_callback( struct hrtimer *timer ) {
	char arr[50];
	struct reserve_struct *rs;
	struct task_struct *t;
	unsigned long flags;
	unsigned long energy_flags;
	long long  remain = 0, budget_length = 0, period_length = 0, ts=0;//, energyTask = 0;//factor=0;
	unsigned int answer=0,timestamp=0;//enbld=0;	
//	printk(KERN_ALERT "IN PERIOD_TIMER_CALLBACK\n");	
	rs= container_of(timer,struct reserve_struct, period_timer);
	t= container_of(rs, struct task_struct, reservation);

	spin_lock_irqsave(&(rs->reserve_lock), flags);
	print_lock();
		
	if (!rs->isActive) {
//		printk(KERN_ALERT "[P_CB], reservation is cancelled\n");
		spin_unlock_irqrestore(&(rs->reserve_lock), flags);
		print_unlock();
		return HRTIMER_NORESTART;
	}
	if (hrtimer_active(&rs->budget_timer)) {
		if(ktime_to_ns(hrtimer_get_remaining(&rs->budget_timer)) >= 0) {
                	rs->budget_remain = ktime_to_timespec(hrtimer_get_remaining(&(rs->budget_timer)));
			hrtimer_cancel(&(rs->budget_timer));
		}
//		printk(KERN_ALERT "[P_CB] budget timer isactive - 0\n");
	} else {
//		printk(KERN_ALERT "[P_CB] budget timer is not active\n");
	}
	rs->period_runcount++;
	// write to util->tid file
	if(enabled==1){
		if(strlen(rs->thattr->val)<2000){
			remain= (long long) timespec_to_ms(&(rs->budget_remain));
			budget_length = (long long) timespec_to_ms(&(rs->C_reserve));
			period_length= (long long) timespec_to_ms(&(rs->T_reserve));
			if (remain < 0 || budget_length < remain) {
				remain = 0;
			}	
			remain = (budget_length - remain) * 1000;
			do_div(remain, period_length);
			answer = (int)remain;
			ts = timespec_to_ms(&(rs->T_reserve)) * rs->period_runcount;
			timestamp = (int)ts; 
			if(answer<10){
				sprintf(arr,"%d\t0.00%d\n",timestamp,answer);
			} else if(answer>=10 && answer<100){ 
				sprintf(arr,"%d\t0.0%d\n",timestamp,answer);  
			} else if(answer>=100){
				sprintf(arr,"%d\t0.%d\n",timestamp,answer);
			}
			strncat(rs->thattr->val,arr,50);
	 	}
       	} else {
//		printk(KERN_ALERT "[P_CB] ENALBE == 0\n");
	}
	//lab4 calculate the energy each task usei
	if (energy_enable == 1) {
	/*	remain = (long)timespec_to_ms(&(rs->budget_remain));
		budget_length = (long)timespec_to_ms(&(rs->C_reserve));
		if (remain < 0 || budget_length < remain) {
			remain = 0;
		}
		remain = (budget_length - remain);           //ms
		energyTask = power_get() * remain;
		do_div(energyTask, 1000);
		rs->thattr->energy = (long)energyTask;
		energy_total += (long)energyTask;*/
		spin_lock_irqsave(&(rs->energy_lock), energy_flags);
		rs->thattr->lastEnergy = 0;
		spin_unlock_irqrestore(&(rs->energy_lock), energy_flags);
	} else {
//		rs->thattr->energy = 0;
	}	
		
        rs->budget_remain = rs->C_reserve;
//	printk(KERN_ALERT "[P_CB]begin new round\n");
        if (rs->isRunning) {
        	start_budget_timer(&(rs->budget_timer), rs->C_reserve);
//		printk(KERN_ALERT "[P_CB] with Running \n");
	} else if (rs->isSuspend || t->state == TASK_UNINTERRUPTIBLE){
		if (wake_up_process(t)) {
//			printk(KERN_ALERT "Is waken up\n");
		} else {
//			printk(KERN_ALERT "Already waken up\n");
		}
		rs->isSuspend = false;
//		printk(KERN_ALERT "[P_CB] with Suspend %ld\n", t->state);
	} else {
//                printk(KERN_ALERT "[P_CB] with other states\n");
        }
	hrtimer_forward_now(&(rs->period_timer), timespec_to_ktime(rs->T_reserve));
	spin_unlock_irqrestore(&(rs->reserve_lock), flags);
	print_unlock();
	return HRTIMER_RESTART;
}
enum hrtimer_restart budget_timer_callback( struct hrtimer *timer ) {
        struct reserve_struct *rs;
        struct task_struct *t;
	unsigned long flags;
//	printk(KERN_ALERT "IN BUDGET_TIMER_CALLBACK\n");
	rs= container_of(timer,struct reserve_struct, budget_timer);
        t= container_of(rs, struct task_struct, reservation);
	spin_lock_irqsave(&(rs->reserve_lock), flags);
	print_lock();

        if (rs->isActive) {
//		printk(KERN_ALERT "budget callback cause context switch\n");
		rs->isSuspend = true;
		rs->budget_remain.tv_sec = 0;
		rs->budget_remain.tv_nsec = 0;
		set_task_state(t, TASK_UNINTERRUPTIBLE);
		set_tsk_need_resched(t);
	}
	spin_unlock_irqrestore(&(rs->reserve_lock), flags);
	print_unlock();
        return HRTIMER_NORESTART;
}
void print_lock() {
//	printk(KERN_ALERT "GET LOCK\n");
}
void print_unlock() {
//        printk(KERN_ALERT "LOSE LOCK\n");
}
