/*****************************************************************************
 * 18-648 FAll 2015 LAB 1
 * =====================================================================
 * GROUP 10  		-> Mengye Gong (mengyeg) 
 *	     		-> Nishant Parekh (nmparekh)
 *           		-> Ziyuan Song (ziyuans)
 * PROGRAM:  		rtps.c 
 * AUTHOR:              Nishant Parekh
 * DESCRIPTION: 	User-app to display lis of RT threads in descending 
 * 			order with NCURSES (Lab1 Section 4.6 and 4.6.1)
*******************************************************************************/

#include <stdio.h>
#include <linux/unistd.h>
#include <linux/linkage.h>
#include <linux/mystruct.h>
#include <stdlib.h>
#include <signal.h>

#define DELAY 2000000   //screen update time = 2 seconds


void print_rt_tasks_list(struct mytask_struct *,int);
void bubbleSortTasks(struct mytask_struct *);
void swapTasks(struct mytask_struct *, struct mytask_struct *);
void signalhandler(int param);


int main(){
	
	int iterator=0;
	struct mytask_struct *head,*tail=NULL,*temp=NULL,*freenode=NULL;
	//signal handler for Ctrl+C
	signal(SIGINT, signalhandler);
	//initialize screen for Ncurses
	while(1){
		//get number of rt threads using count_rt_threads syscall
		int num_rt_threads=syscall(__NR_count_rt_threads);
		//printf("RTPS: Count rt threads returned -> %d\n", num_rt_threads);
		head=(struct mytask_struct *) malloc(sizeof(struct mytask_struct ));
		tail=head;
		//allocate memory for the list as per the count of rt threads
		for(iterator=1;iterator<num_rt_threads;iterator++){
			temp =(struct mytask_struct *) malloc(sizeof(struct mytask_struct));	
        		tail->next=temp;
			temp->next=NULL;
			tail=temp;
		}
		//call list_rt_threads syscall
		syscall(__NR_list_rt_threads, head, num_rt_threads);
 		//sort rt threads in descending order	
		bubbleSortTasks(head);
		//print the list of sorted rt threads
		print_rt_tasks_list(head,num_rt_threads);	
		//fflush(stdout);
		temp=head;
		while(temp!=NULL){
		freenode=temp;
		temp=temp->next;
		free(freenode);

}
		usleep(DELAY);		//refresh screen every 2 seconds
	
	}//end of while
	return 0;
}

/* Function to print rt tasks list */
void print_rt_tasks_list(struct mytask_struct *head,int count){
	int cnt=0;
	//pointer for traversal, initially poiniting to head
	struct mytask_struct *nxt_task;
	nxt_task=head;
	printf("\nPID\tTID\tPRIORITY\tCOMMAND\n");
	for(cnt=0;cnt<count;cnt++) {
		if(nxt_task!=NULL){
			printf("%d\t%d\t%d\t\t%s\n",nxt_task->task_pid,nxt_task->task_tid,nxt_task->task_prio,nxt_task->task_comm);
			nxt_task=nxt_task->next;
		}else
			return;
	}	
}

/* Bubble sort the given rt tasks list */
void bubbleSortTasks(struct mytask_struct *head)
{
    int swapped, i;
    struct mytask_struct *ptr1;
    struct mytask_struct *lptr = NULL;

    //check if list is empty
    if (ptr1 == NULL)
        return;

    do
    {
        swapped = 0;
        ptr1 = head;
        /* until we reach the end of the list
	 * compare the task nodes based on priority
         * and swap for descending order
	 */ 
        while (ptr1->next != lptr)
        {
            if (ptr1->task_prio >  ptr1->next->task_prio)
            {
                swapTasks(ptr1, ptr1->next);
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    }
    while (swapped);
}

/*Function to swap data of two mytask_structs a and b*/
void swapTasks(struct mytask_struct *t1, struct mytask_struct *t2)
{
        pid_t tmp_pid, tmp_tid;
        int tmp_prio;
        char tmp_comm[16];

	//swap task attributes
        tmp_pid = t1->task_pid;
        tmp_tid = t1->task_tid;
        tmp_prio = t1->task_prio;
        strcpy(tmp_comm,t1->task_comm);

        t1->task_pid = t2->task_pid;
        t1->task_tid = t2->task_tid;
        t1->task_prio = t2->task_prio;
        strcpy(t1->task_comm,t2->task_comm);

        t2->task_pid = tmp_pid;
        t2->task_tid = tmp_tid;
        t2->task_prio = tmp_prio;
        strcpy(t2->task_comm,tmp_comm);
}
/*Function to catch CTRL+C signal */
void signalhandler(int param)
{
	//cloddse the ncurses window
//	endwin();
  	exit(1);
}
