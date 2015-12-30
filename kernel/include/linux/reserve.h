/*
 * 18-648 FALL 2015 LAB 2
 * ===========================================================================
 * GROUP 10  -> Mengye Gong    (mengyeg)
 *	     -> Nishant Parekh (nmparekh)
 *           -> Ziyuan Song    (ziyuans)
 * FILE:  		reserve.h 
 * DESCRIPTION: 	Reservation paramters for computation time tracking
 *                      (Lab 2 Section 2.2.3)
 * ============================================================================
 * AUTHOR: NISHANT PAREKH (nmparekh)
 * =============================================================================
*/
#ifndef _LINUX_RESERVE_H_
#define _LINUX_RESERVE_H_

/*
 *  Reservation structure defines parameters required to compute the actual time for
 *  which a task executes across context switches
 */
/* include threadattr.h for sysfs file */

#include <linux/threadattr.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#define UTILFACTOR 1000

typedef struct reserve_struct{

	pid_t  tid;
	
	struct timespec C_reserve;  /* C budget alloted using set_reserve */
	struct timespec T_reserve;  /* T budget alloted using set_reserve */
	
	struct timespec budget_remain;		      
	struct thread_attr *thattr;    //stores thread attribute to read its sys file
	struct kobj_attribute thattribute; // sysfs attribute
	struct kobject *pid_kobject;
	struct kobj_attribute energy_kobj_attr;  // sysfs attribute for energy monitor LAB 4	
	struct hrtimer period_timer; //timer that expires every T_reserve for a task
	struct hrtimer budget_timer; //timer that expires every C_reserve for a task
	u64 period_hrtimer_interval;	//time interval for period  hrtimer 
	u64 budget_hrtimer_interval;	//time interval for budget  hrtimer 
	
	
	bool isActive; //stores the active state of task
	bool isSuspend;
	bool isRunning;
	/* spin lock for timer handlers */
	spinlock_t T_lock;
	spinlock_t C_lock;
	spinlock_t reserve_lock;
	spinlock_t energy_lock;

	unsigned long utilization; //util as a multiple of 10 ^k where k = 2, 3 , 4...
	unsigned long period_runcount;
	int core_num;	//assigned core
} reserve_struct;

/* functions needed by reservation framework */
long long unsigned timespec_to_ms(struct timespec*);
void start_budget_timer(struct hrtimer*, struct timespec);
enum hrtimer_restart budget_timer_callback( struct hrtimer * );
void print_lock(void);
void print_unlock(void);
#endif /* _LINUX_RESERVE_H_ */
