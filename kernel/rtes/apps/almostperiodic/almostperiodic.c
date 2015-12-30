/*
 * 18-648 FALL 2015 LAB 3
 * =====================================================================
 * GROUP 10
 * -> Mengye Gong    (mengyeg)
 * -> Nishant Parekh (nmparekh)
 * -> Ziyuan Song    (ziyuans)
 * FILE:             almostperiodic.c 
 * DESCRIPTION:      almostperiodic test application (Lab3 Section 2.2.3)
 * ======================================================================
 * AUTHOR : Nishant Parekh (nmparekh), Mengye Gong (mengyeg)
 * ======================================================================
*/
#define _GNU_SOURCE

#include <linux/sched.h>      /* for sched_setaffinity */
#include <linux/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>       
#include <signal.h>     
#include <assert.h>
#include <linux/sched.h>
//#include <sys/syscall.h>
//#include <linux/reserve.h>

//  Global Variables to hold c and t values
int c, t;
int bugCount, bugPeriod;
void stop_working(int param);   //signal handler for SIGALRM
void exit_periodic_task(int param);   //signal handler for Ctrl-C i.e. SIGINT


int main(int argc,char *argv[]){

	int cpuid;
    	unsigned long sleep_time;
	unsigned int retVal;
	struct itimerval timeout_val;
 	unsigned long * mask;
	struct timespec cstr, tstr;
	struct sched_param param;
	
	param.sched_priority = 50;
	sched_setscheduler(0, SCHED_FIFO, &param);

	if ( argc != 4 ){
        	printf( "******* Usage: %s C(in ms) T(in ms) cpuid \n", argv[0] );
    		return -1;
	}else{
    	        //Parse and read arguments and check the sanity of C, T and 
        	// CPUID values 
    		int ret = sscanf(argv[1], "%d", &c);
        	ret = ret + sscanf(argv[2],"%d", &t);
        	ret = ret+ sscanf(argv[3], "%d", &cpuid);
       		if(ret!=3){
            		printf("Please check the arguments entered...\n");
        	}else{
        		if(c > t ){
        			printf("C can't be greater than T!\n");
        			return -1;
        		}
        		if(cpuid < 0 || cpuid >3){
        			printf("Only 4 cores ranging from 0 to 3\n");
        			return -1;
        		}
        		if(c > 60000 || t > 60000){
        			printf("Maximum allowed values for C and T are 60 Seconds\n");
        			return -1;
        		}
            		//printf("Values entered are: C= %d T= %d CPUID= %d\n",c, t, cpuid );
        	}
		//set affinity of the thread as per entered cpu ID
		mask=malloc(sizeof(unsigned long));

		*mask=(unsigned long)(1<<cpuid);
		ret=syscall(__NR_sched_setaffinity, getpid(), sizeof(unsigned long), mask);
		if(ret<0){
			printf("Unable to set affinity for thread to %d\n",cpuid);	
			return -1;
		}
    	
 	//	printf("C_TIME  = %d sec: %d usec \n" , c/1000, (c%1000 * 1000));
	//	printf("sleep time = %d  usec\n" , ((t-c) * 1000));
  
  		timeout_val.it_interval.tv_sec = 0;
  		timeout_val.it_interval.tv_usec = 0;
 		timeout_val.it_value.tv_sec = c/1000;/* set the time off at every c units */
  		timeout_val.it_value.tv_usec = ((c % 1000)*1000);      
	  	setitimer(ITIMER_REAL, &timeout_val,0);
		
  		signal(SIGALRM,stop_working); /* set signal handler for SIGALARM when timer goes off*/
  		signal(SIGINT,exit_periodic_task); /* set signal handler for Ctrl-c */
		
		//asume c less than 5000ms
		bugCount = generate_random(10000, 5000) / c;
		bugPeriod = generate_random(20000, 10000) / t;
		while(1); //busy loop
	
	return 0;
    }
}//end of main

int generate_random(int up, int low) {
	return rand() % (up - low) + low;
}

void stop_working(int param)
{
   	struct itimerval tout_val;

   	signal(SIGALRM,stop_working);
   
   	//printf("Busy Looped for C now Sleeping now for %d!\n", ((t-c)*1000));
	//usleep((t-c) * 1000);   //sleep
	if (bugPeriod <= 0 && bugCount > 0) {
		bugCount--;
	} else if (bugPeriod <= 0 && bugCount <= 0) {
		bugCount = generate_random(10000, 5000) / c;
		bugPeriod = generate_random(20000, 10000) / t;
	} else if (bugPeriod > 0) {
		if (syscall(__NR_end_job) == -1) {
			printf("End job Error!\n");
		}
		//printf("Suspend\n");
		usleep((t - c) * 1000);    //sleep
		if (syscall(__NR_wake_up_job) == -1) {
			printf("WakeUp Error!\n");
		}
		//printf("WakeUp\n");
		bugPeriod--;
	}

   	tout_val.it_interval.tv_sec = 0;
   	tout_val.it_interval.tv_usec = 0;
   	tout_val.it_value.tv_sec = c/1000; /* Restart Timer for C units */
   	tout_val.it_value.tv_usec = (c%1000)*1000;
   	setitimer(ITIMER_REAL, &tout_val,0);  	
	//printf("Wake up\n");
}

void exit_periodic_task(int param)
{
    signal(SIGINT,exit_periodic_task);
    printf("\nUser pressed Ctrl-C....Exiting now!\n");
    exit(0);
}

