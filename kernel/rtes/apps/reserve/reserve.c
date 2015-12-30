/*
 * 18-648 FALL 2015 LAB 2
 * ===========================================================================
 * GROUP 10
 * -> Mengye Gong    (mengyeg)
 * -> Nishant Parekh (nmparekh)
 * -> Ziyuan Song    (ziyuans)
 * FILE:             reserve.c 
 * DESCRIPTION:      Reserve syscall application (Lab2 Section 2.2.5)
 * ============================================================================
 * AUTHOR : NISHANT PAREKH(nmparekh)
 * ============================================================================
*/
#include <stdio.h>
#include <linux/unistd.h>
#include <linux/linkage.h>
#include <linux/signal.h>     /* for signal */
#include <linux/time.h>

/* Function to convert C and T from ms to timespec */
static void millisecs_to_timespec(struct timespec *time, unsigned long ms);

int main(int argc,char *argv[]){

    unsigned long c_ms=0, t_ms=0; //arguments entered by user
    int cpuid=0;
	int tid=0;
	char set_cmd[]= "set";
	char cancel_cmd[] ="cancel";
	char command[10];
	struct timespec C, T;
	int retVal=0;

	if(argc< 2){
		printf("Insufficient arguments...\n");	
		return -1;
			
	}	
	if(sscanf(argv[1],"%s",command) < 0 ){
		printf("Please check the command entered...\n");
		return -1;
	}

	//case of set syscall	
        if (strcmp(command,set_cmd)==0){
	
		if(argc != 6 ) //check if all arguments are entered
       		{
                	printf( "******* Usage: %s set TID C(in ms) T(in ms) cpuid \n", argv[0] );
	 	}
		else
		{  
			if(sscanf(argv[2], "%d", &tid)< 0){
				printf("Pls check the TID entered...\n");
				return -1;
			}
			if(sscanf(argv[3], "%lu", &c_ms)< 0){
                                printf("Pls check the C value entered...\n");
                                return -1;
                        }
			if(sscanf(argv[4], "%lu", &t_ms)< 0){
                                printf("Pls check the T value entered...\n");
                                return -1;
                        }
			if(sscanf(argv[5], "%lu", &cpuid)< 0){
                                printf("Pls check the CPU value entered...\n");
                                return -1;
                        }
			//other checks on range values for C, T and cpuid
			if(c_ms > t_ms ){
                                printf("C cannot be greater than T!\n");
                                return -1;
                        }

                        if((cpuid < -1 ||  cpuid >3 )){
                        	printf(" CPU = %d", cpuid);
                                printf("***** CPUID can only be in the range -1 to 3!\n");
                                return -1;
			}

			//printf("Calling set_reserve...\n");
			//printf("Values entered for \"%s\" are: TID =%d  C= %lu T= %lu CPUID= %d\n",command,tid, c_ms, t_ms, cpuid );
			//convert c and t value from ms to timespec format
			millisecs_to_timespec(&C,c_ms);
			millisecs_to_timespec(&T,t_ms);
			//printf("C = %lu : %lu T= %lu : %lu\n",C.tv_sec,C.tv_nsec,T.tv_sec,T.tv_nsec);
			//call set_reserve
			retVal=syscall(__NR_set_reserve,tid,&C,&T,cpuid);
			//printf("Returned from set with value =%d\n",retVal);
                    	
		}
	}
	//case of cancel syscall
	else if(strcmp(command,cancel_cmd)==0){
	//check if all arguments are entered
		if(argc!= 3)
		{
                	printf( "******* Usage: %s cancel TID\n", argv[0] );
        	}
        	else
        	{
                	if(sscanf(argv[2], "%d", &tid)< 0){
				printf("Pls check the TID entered...\n");
				return -1;
			}
			
			//printf("Calling cancel_reserve...\n");		
                        //printf("Values entered for \"%s\" are: TID =%d\n",command,tid);
			retVal=syscall(__NR_cancel_reserve,tid);
			//printf("Returned from cancel with value = %d\n",retVal);

		}
	}
	else{
		printf("Incorrect command! Try again using 'set' or 'reserve'\n");
		return -1;
	}
	
	return retVal;
		
}//end of main

void millisecs_to_timespec(struct timespec *time, unsigned long ms)
{
    time->tv_sec = ms / 1000;
    time->tv_nsec = (ms % 1000) * 1000000;
}

