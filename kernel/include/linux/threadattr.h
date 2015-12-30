/*===========================================================================
 * 18-648 FALL 2015 LAB 2 
 * ===========================================================================
 * GROUP 10  -> Mengye Gong    (mengyeg)
 *	     -> Nishant Parekh (nmparekh)
 *           -> Ziyuan Song (ziyuans)
 * FILE:  		threadattr.h 
 * DESCRIPTION: 	Thread attribute parameters for individual threads
 *                 
 * ============================================================================
 * AUTHOR: NISHANT PAREKH (nmparekh)
 * =============================================================================
*/
#ifndef _LINUX_THREADATTR_H_
#define _LINUX_THREADATTR_H_

/*Definition of read write methods for sysfs thread attribute*/

struct thread_attr{
	char name[20];
	char val[2048];
	long energy;                 //energy consumed by this task LAB4
	long lastEnergy;
	mode_t mode;
};

#endif /* _LINUX_THREADATTR_H_ */
