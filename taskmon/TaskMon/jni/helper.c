
#include "helper.h"

int getThreadList(char *string, int num_rt_threads) {
	int iterator=0;
	struct mytask_struct *head, *tail, *temp, *freenode;
	head=(struct mytask_struct *) malloc(sizeof(struct mytask_struct));
	tail=head;

	for(iterator=1; iterator<num_rt_threads; iterator++){
		temp =(struct mytask_struct *) malloc(sizeof(struct mytask_struct));
		tail->next=temp;
		temp->next=NULL;
		tail=temp;
	}
	syscall(__NR_list_rt_threads, head, num_rt_threads);
	temp = head;
	int size = 0;
	while (temp != NULL) {
		size += sprintf(string + size, "%d\n", temp->task_pid);
		temp = temp->next;
	}
	temp=head;
	while (temp != NULL) {
		freenode = temp;
		temp = temp->next;
		free(freenode);
	}
	return size;
}
int getReserveThreadList(char *string) {
	DIR *d;
	int size = 0;
	struct dirent *dir;
	//struct stat s;

	d = opendir("/sys/rtes/taskmon/util/");
	//d = opendir("/data/lab2/files/");
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			__android_log_print(ANDROID_LOG_INFO, "NATIVE", "dir %s", dir->d_name);
			size += sprintf(string + size, "%s\n", dir->d_name);
		}
	}
	closedir(d);
	return size;
}

void millisecs_to_timespec(struct timespec *time, unsigned long ms) {
    time->tv_sec = ms / 1000;
    time->tv_nsec = (ms % 1000) * 1000000;
}
