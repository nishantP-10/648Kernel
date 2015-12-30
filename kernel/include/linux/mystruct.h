/*18648 LAB 1
 * =====================================================================
 * GROUP 10  -> Mengye Gong (mengyeg) 
 *	     -> Nishant Parekh (nmparekh)
 *           -> Ziyuan Song (ziyuans)
 * PROGRAM:  		mystuct.h 
 * DESCRIPTION: 	My own header file to contain process related info for Lab 1 Section 4.5.4
*/
#ifndef _LINUX_MYSTRUCT_H_
#define _LINUX_MYSTRUCT_H_

/*
 *  Defining our own structure to store task_struct related info - tid, pid,priority, comm
 */

#define LEN 16
#include<stdbool.h>

typedef struct mytask_struct{

	pid_t task_pid;
	pid_t task_tid;
	int task_prio;
	char task_comm[LEN];
	struct mytask_struct *next;
	bool isActive;
	
}mytask_struct;

#endif /* _LINUX_MYSTRUCT_H_ */
