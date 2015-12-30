/*
 * 18-648 FALL 2015 LAB 3
 * ===========================================================================
 * GROUP 10  -> Mengye Gong    (mengyeg)
 *	     -> Nishant Parekh (nmparekh)
 *           -> Ziyuan Song    (ziyuans)
 * FILE:  		utility.h 
 * DESCRIPTION: 	
 *                      (Lab 3 Section 2.3.2)
 * ============================================================================
 * AUTHOR: ZIYUAN SONG
 * =============================================================================
*/
#ifndef _LINUX_UTILITY_H_
#define _LINUX_UTILITY_H_

#include <linux/time.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/utimeasure.h>
typedef struct res_info_struct{

        struct timespec C_reserve;
        struct timespec T_reserve;
        int core_num;
        pid_t tid;
	unsigned long long sysclock_freq;
        struct res_info_struct *next;

}res_info_struct;

// return the core num that could be scheduled.If not, return -1
int which_core_sched(struct timespec*, struct timespec*,int, pid_t);
bool has_res(void);
bool has_res_on_core(int); 
void init_bins(void);
int sysclock_calculation(void);
int higher_priority(struct res_info_struct*, struct res_info_struct*);

#endif /* _LINUX_UTILITY_H_ */

