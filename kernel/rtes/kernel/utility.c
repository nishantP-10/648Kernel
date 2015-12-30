#include <linux/spinlock.h>
#include <linux/reserve.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/utility.h>

#define UTIL_FACTOR 100

int bound1000000[101] = {1000000,828427,779763,756828,743491,734772,728626,724061,720537,717734,715451,713557,711958,710592,709411,708380,707472,706666,705945,705298,704713,704182,703697,703253,702845,702469,702121,701797,701497,701216,700954,700708,700478,700260,700056,699863,699680,699507,699343,699187,699039,698898,698763,698635,698513,698395,698283,698176,698072,697973,697878,697787,697699,697614,697533,697454,697378,697305,697234,697166,697100,697036,696974,696914,696856,696799,696745,696691,696640,696590,696541,696494,696448,696403,696360,696317,696276,696236,696196,696158,696121,696085,696049,696014,695981,695948,695915,695884,695853,695823,695793,695764,695736,695709,695682,695655,695629,695604,695579,695555};

bool is_RM_sched(struct res_info_struct*, struct timespec*, struct timespec*, int, pid_t);
int is_bound_sched(struct res_info_struct*, struct timespec*, struct timespec*, int, pid_t);
bool is_RM_sched_each(struct res_info_struct*, struct res_info_struct*);
long div_ceil_int(long long, long long);
int cal_utilization1000000 (struct timespec*, struct timespec*);
void free_res_info(struct res_info_struct*);
int alloc_res_info(struct res_info_struct*, pid_t);
void init_bins_util(struct res_info_struct*);
int find_bin_FF(struct res_info_struct*, struct timespec*, struct timespec*, pid_t);
int find_bin_NF(struct res_info_struct*, struct timespec*, struct timespec*, pid_t);
int find_bin_BF(struct res_info_struct*, struct timespec*, struct timespec*, pid_t);
int find_bin_WF(struct res_info_struct*, struct timespec*, struct timespec*, pid_t);
int find_bin_LST(struct res_info_struct*, struct timespec*, struct timespec*, pid_t);
int nth_highest_bin(int);
int nth_lowest_bin(int n);
bool is_sched_on_core(struct res_info_struct*, struct timespec*, struct timespec*,int, pid_t);
void print_res(struct res_info_struct*);
void print_bins_util(void);
int get_min_freq_per_task(struct res_info_struct*, struct res_info_struct*);
long long getNextPeriod(long long, struct res_info_struct *, struct res_info_struct*);
int bins_util[4] = {0, 0, 0, 0};
int cur_bin = 0;
volatile int sysclock_factor=0; 

//int core_sysclock_freq[4];
// LAB 3
// Check if there is reservations.
bool has_res() {
	struct task_struct *task,*x,*y;
	bool rs = false;
        rcu_read_lock();
        do_each_thread(x,y){
                unsigned long flags;
		struct reserve_struct res;
                task = y;
                spin_lock_irqsave(&(task->reservation.reserve_lock), flags);
                res = task->reservation;
                if (res.isActive == true) {
                	rs = true;
		}
                spin_unlock_irqrestore(&(task->reservation.reserve_lock), flags);
		if (rs == true) {
			break;
		}
        }while_each_thread(x,y);
        rcu_read_unlock();
	return rs;
}
bool has_res_on_core(int core_num) {
        struct task_struct *task,*x,*y;
        bool rs = false;
        rcu_read_lock();
        do_each_thread(x,y){
                unsigned long flags;
                struct reserve_struct res;
                task = y;
                spin_lock_irqsave(&(task->reservation.reserve_lock), flags);
                res = task->reservation;
		if (res.isActive == true && res.core_num == core_num) {
                        rs = true;
                }
                spin_unlock_irqrestore(&(task->reservation.reserve_lock), flags);
                if (rs == true) {
                        break;
                }
        }while_each_thread(x,y);
        rcu_read_unlock();
        return rs;
}
// Change Policy
void init_bins () {
	memset(bins_util, 0, sizeof(bins_util));
	cur_bin = 0;
}
// Set Reserve Call this function
int which_core_sched(struct timespec *C_reserve,
              struct timespec *T_reserve,
              int core_num,
              pid_t tid) {
        struct res_info_struct *res_info;
        int size = 0;
        int rs = -1;
        res_info = (struct res_info_struct *) kmalloc(sizeof(res_info_struct),GFP_KERNEL);
        res_info->next = NULL;
        size = alloc_res_info(res_info, tid);
        if (core_num == -1) {
                init_bins_util(res_info);
                //if (strcmp(policy, POLICY_TYPE[3]) == 0) {
		if (policy == 4) {
			printk(KERN_ALERT "[u] LST\n");
			rs = find_bin_LST(res_info, C_reserve, T_reserve, tid);
		} else if (policy == 3) {
			printk(KERN_ALERT "[u] WF\n");
                        rs = find_bin_WF(res_info, C_reserve, T_reserve, tid);
                } else if (policy == 2) {
			printk(KERN_ALERT "[U] BF\n");
                        rs = find_bin_BF(res_info, C_reserve, T_reserve, tid);
                } else if (policy == 1) {
			printk(KERN_ALERT "[U] NF\n");
                        rs = find_bin_NF(res_info, C_reserve, T_reserve, tid);
                } else {
			printk(KERN_ALERT "[U] FF\n");
                        rs = find_bin_FF(res_info, C_reserve, T_reserve, tid);
                }
        } else {
                if (is_sched_on_core(res_info, C_reserve, T_reserve, core_num, tid) == true) {
                        rs = core_num;
                }
        }

	//sysclock_calculation(res_info);

	print_res(res_info);
	printk(KERN_ALERT "[U]Core %d is GOOD\n", rs);

        free_res_info(res_info);
        kfree(res_info);
        return rs;
}
/*****************************Below helper funtions*****************************/
void print_res(struct res_info_struct *res_info) {
	
	struct res_info_struct *cur;
	int core_num;
	printk(KERN_ALERT "[U]************************* UTILITY PRINT BELOW ****************************\n");
	for (core_num = 0; core_num < 4; core_num++) {
		cur = res_info;
		printk(KERN_ALERT "[U]FOR CORE %d RESERVATION", core_num);
		while(cur->next != NULL) {
			cur = cur->next;
			if (cur->core_num == core_num) {
				long long c_ms = timespec_to_ms(&(cur->C_reserve));
        			long long t_ms = timespec_to_ms(&(cur->T_reserve));
				printk(KERN_ALERT "[U]tid: %d, C: %llu, T: %llu\n", cur->tid, c_ms, t_ms);
			}
		}
	}
	printk(KERN_ALERT "[U]************************** UTILITY PRINT END ******************************\n");
}
void print_bins_util() {
	int i = 0;
	for (i = 0; i < 4; i++) {
		printk(KERN_ALERT "[U] bins_util[%d] = %d\n", i, bins_util[i]);
	}
}
void init_bins_util(struct res_info_struct *res_info) {
	struct res_info_struct *cur = res_info;
	memset(bins_util, 0, sizeof(bins_util));
        while (cur->next != NULL) {
		cur = cur->next;
                bins_util[cur->core_num] += cal_utilization1000000(&(cur->C_reserve), &(cur->T_reserve));
        }
	print_bins_util();
}
int alloc_res_info(struct res_info_struct *res_info, pid_t tid) {
	struct task_struct *task,*x,*y;
	int size = 0;
	struct res_info_struct *cur = res_info;
	rcu_read_lock();
        do_each_thread(x,y){
		unsigned long flags;
		struct reserve_struct res;
		task = y;
		spin_lock_irqsave(&(task->reservation.reserve_lock), flags);		
          	res = task->reservation; 
                if (res.isActive == true && res.tid != tid) {
                        // do anything for reversed Threads
			
			struct res_info_struct *node = 
				(struct res_info_struct*) kmalloc (sizeof (res_info_struct), GFP_KERNEL);
			size++;
			node->C_reserve = res.C_reserve;
			node->T_reserve = res.T_reserve;
			node->core_num = res.core_num;
			node->tid = res.tid;
			node->next = NULL;
			cur->next = node;
			cur = cur->next;
			//printk(KERN_ALERT "alloc one\n");
                }
		spin_unlock_irqrestore(&(task->reservation.reserve_lock), flags);
        }while_each_thread(x,y);
	rcu_read_unlock();
        return size;
}
void free_res_info(struct res_info_struct *res_info) {
	struct res_info_struct *cur = res_info->next;
	//printk(KERN_ALERT "Free_res_info.\n");
	while (cur != NULL) {
		struct res_info_struct *temp = cur->next;
		//printk(KERN_ALERT "free one\n");
		kfree(cur);
		cur = temp;
	}
}
int find_bin_FF(struct res_info_struct *res_info,
                struct timespec *C_reserve,
                struct timespec *T_reserve,
                pid_t tid) {
	int i = 0;
	for (i = 0; i < 4; i++) {
		if (bins_util[i] != 0) {
			if (is_sched_on_core(res_info, C_reserve, T_reserve, i, tid) == true) {
				return i;
			}
		}
	}
	for (i = 0; i < 4; i++) {
		if (bins_util[i] == 0) {
			if (is_sched_on_core(res_info, C_reserve, T_reserve, i, tid) == true) {
				return i;
			} else {
				return -1;
			}
		}
	}
	return -1;
}
int find_bin_NF(struct res_info_struct *res_info,
                struct timespec *C_reserve,
                struct timespec *T_reserve,
                pid_t tid) {
	int i = 0;
	for (i = 0; i < 4; i++) {
		//printk(KERN_ALERT "Current bin == %d", cur_bin);
		int bin = (cur_bin + i) % 4;
		if (bins_util[bin] != 0) {
			if (is_sched_on_core(res_info, C_reserve, T_reserve, bin, tid) == true) {
				cur_bin = bin;
				return bin;
			}
		}
	}
	for (i = 0; i < 4; i++) {
                int bin = (cur_bin + i) % 4;
                if (bins_util[bin] == 0) {
                        if (is_sched_on_core(res_info, C_reserve, T_reserve, bin, tid) == true) {
                                cur_bin = bin;
                                return bin;
                        } else {
				return -1;
			}
                }
        }
	return -1;
}

int find_bin_BF(struct res_info_struct *res_info,
                struct timespec *C_reserve,
                struct timespec *T_reserve,
                pid_t tid) {
	int i = 0;
	for (i = 0; i < 4; i++) {
		int bin = nth_highest_bin(i);
		if (is_sched_on_core(res_info, C_reserve, T_reserve, bin, tid) == true) {
			return bin;
		}
	}
	return -1;
}
int find_bin_WF(struct res_info_struct *res_info,
                struct timespec *C_reserve,
                struct timespec *T_reserve,
                pid_t tid) {
	int i = 0;
	for (i = 3; i >= 0; i--) {
		int bin = nth_highest_bin(i);
		if (bins_util[bin] != 0) {
			if (is_sched_on_core(res_info, C_reserve, T_reserve, bin, tid) == true) {
				return bin;
			}
		}
	}

	for (i = 0; i < 4; i++) {
		if (bins_util[i] == 0) {
			if (is_sched_on_core(res_info, C_reserve, T_reserve, i, tid) == true) {
				return i;
			} else {
				return -1;
			}
		}
	}
	return -1;
}
int find_bin_LST(struct res_info_struct *res_info,
                struct timespec *C_reserve,
                struct timespec *T_reserve,
                pid_t tid) {
	
      	int bin = nth_lowest_bin(0);       
        if (is_sched_on_core(res_info, C_reserve, T_reserve, bin, tid) == true) {
		return bin;
        }
        return -1;
}

// n: 0 ~ 3
int nth_highest_bin(int n) {
	int bin_num[] = {0, 1, 2, 3};
	int bins_util_temp[4];
	int i = 0;
	int j = 0;
	memcpy(bins_util_temp, bins_util, 4 * sizeof(int));
	for (i = 3; i >= 0; i--) {
		for (j = 1; j <= i; j++) {
			if (bins_util_temp[j] > bins_util_temp[j - 1]) {
				int temp = bins_util_temp[j];
				bins_util_temp[j] = bins_util_temp[j - 1];
				bins_util_temp[j - 1] = temp;
				temp = bin_num[j];
				bin_num[j] = bin_num[j - 1];
				bin_num[j - 1] = temp;
			}
		}
	} 
	return bin_num[n];
}
int nth_lowest_bin(int n) {
        int bin_num[] = {0, 1, 2, 3};
        int bins_util_temp[4];
        int i = 0;
        int j = 0;
        memcpy(bins_util_temp, bins_util, 4 * sizeof(int));
        for (i = 3; i >= 0; i--) {
                for (j = 1; j <= i; j++) {
                        if (bins_util_temp[j] < bins_util_temp[j - 1]) {
                                int temp = bins_util_temp[j];
                                bins_util_temp[j] = bins_util_temp[j - 1];
                                bins_util_temp[j - 1] = temp;
                                temp = bin_num[j];
                                bin_num[j] = bin_num[j - 1];
                                bin_num[j - 1] = temp;
                        }
                }
        }
        return bin_num[n];
}
bool is_sched_on_core(struct res_info_struct *res_info,
		struct timespec *C_reserve,
              	struct timespec *T_reserve,
              	int core_num,
	      	pid_t tid){
	bool rs = false;
	int bound_rs = 0;
//	printk(KERN_ALERT "Number of Tasks reserved %d\n",size);
	bound_rs = is_bound_sched(res_info, C_reserve, T_reserve, core_num, tid);
	if (bound_rs == -1) {
		printk(KERN_ALERT "[U]BOUND > 1\n");
		rs = false;
	} else if (bound_rs == 1) {
		printk(KERN_ALERT "[U]BOUND is Sched\n");
		rs = true;
	} else if (is_RM_sched(res_info, C_reserve, T_reserve, core_num, tid)) {
		rs = true;
	}
	return rs;
}
// Check rate monotonic
bool is_RM_sched(struct res_info_struct *res_info,
                     struct timespec *C_reserve,
                     struct timespec *T_reserve,
                     int core_num,
		     pid_t tid){
	struct res_info_struct *cur = res_info;
        bool rs = true;
	res_info->C_reserve = *C_reserve;
	res_info->T_reserve = *T_reserve;
	res_info->core_num = core_num;
	res_info->tid = tid;
	while(cur != NULL) {
		if (cur->core_num == core_num) {
			if(is_RM_sched_each(res_info, cur) == false) {
				rs = false;
				break;
			}
		}
		cur = cur->next;
	}
	return rs;
}
// helper: check one thread, consider same core, and other higher priority thread
bool is_RM_sched_each(struct res_info_struct *res_info, struct res_info_struct *cur) {
	long long last = 0;
	long long curC = timespec_to_ms(&(cur->C_reserve));
        long long curT = timespec_to_ms(&(cur->T_reserve));
	while (1) {
		struct res_info_struct *ptr = res_info;
		long long this = curC;
		while (ptr != NULL) {
			if (ptr->tid != cur->tid &&
			    ptr->core_num == cur->core_num &&
			    higher_priority(ptr, cur) > 0) {
				long long otherC = timespec_to_ms(&(ptr->C_reserve));
				long long otherT = timespec_to_ms(&(ptr->T_reserve));
				this += div_ceil_int(last, otherT) * otherC;
				printk(KERN_ALERT "[U]tid:%d > tid:%d\n", ptr->tid, cur->tid);
			}
			ptr = ptr->next;
		}
		if (this > curT) {
			printk(KERN_ALERT "[U]tid: %d failed with this == %llu\n", cur->tid, this);
			return false;
		} else if (this == last) {
			return true;
		} else {	
			last = this;
		}
	}
}
// helper: get the least integer that greater or equals to (a / b) 
long div_ceil_int(long long a, long long b) {
	long rs = 1;
	while (a > b) {
		rs += 1;
		a = a - b;
	}
	return rs;
}
// helper: compare priority between two, consider order is T, C, TID, 
// 	   only when r0 > r1 return 1, r0 will be calculate in r1 RMA
int higher_priority(struct res_info_struct *r0, struct res_info_struct *r1) {
	long long r0_c_ms = timespec_to_ms(&(r0->C_reserve));
        long long r0_t_ms = timespec_to_ms(&(r0->T_reserve));
	long long r1_c_ms = timespec_to_ms(&(r1->C_reserve));
        long long r1_t_ms = timespec_to_ms(&(r1->T_reserve));
	if (r0_t_ms < r1_t_ms) {
		return 1;
	} else if (r0_t_ms > r1_t_ms) {
		return -1;
	} else {
		if (r0_c_ms < r1_c_ms) {
			return 1;
		} else if (r0_c_ms > r1_c_ms) {
			return -1;
		} else {
			if (r0->tid < r1->tid) {
				return 1;
			} else {
				return -1;
			}
		}
	}
}

// Check utilization bound (sufficient), use a static array to check(<100)
int is_bound_sched (struct res_info_struct *res_info, 
		     struct timespec *C_reserve, 
		     struct timespec *T_reserve, 
		     int core_num,
		     pid_t tid) {
	struct res_info_struct *cur = res_info;
	int size = 1;
	int sum = cal_utilization1000000(C_reserve,T_reserve);
	while (cur->next != NULL) {
		cur = cur->next;
		if (cur->core_num == core_num && cur->tid != tid) {
			sum += cal_utilization1000000(&(cur->C_reserve), &(cur->T_reserve));
			size++;	
		}
		//cur = cur->next;
	}
	if (sum > bound1000000[0]) {
		return -1;
	} else if (sum > bound1000000[size]) {
		return 0;
	} else {
		return 1;
	}
	
}
// helper: calculate utilization when given C and T;
int cal_utilization1000000 (struct timespec *C_reserve, struct timespec *T_reserve) {
	long long c_ms = timespec_to_ms(C_reserve);
        long long t_ms = timespec_to_ms(T_reserve);
        long long u_ms = c_ms * 1000000;
        do_div(u_ms, t_ms);
	printk(KERN_ALERT "Utilization %llu", u_ms);
	return (int)u_ms;
}
// LAB 4
/*
 *sysclock_calculation: Calculating eta for a task-set
 */

int sysclock_calculation(void) {
	struct res_info_struct * res_info = NULL , *head=NULL;
	int cpu=0;
	int core_sysclock_freq[4];
	int cpus_sysclock_freq = 0;
	res_info = (struct res_info_struct *) kmalloc(sizeof(res_info_struct),GFP_KERNEL);
        res_info->next = NULL;
        alloc_res_info(res_info, -1);
	head = res_info;
	if (head->next == NULL) {
		sysclock_factor=0;
		return 0;
	}
	/* Finding the energy minimum frequency for each task */
	for(cpu=0; cpu < 4; cpu++){
		int max_freq = 0;
		while (res_info->next != NULL) {
			struct res_info_struct * next = res_info->next;
			if(next->core_num == cpu){
				int freq = get_min_freq_per_task(head, next);
				if(freq > max_freq) {
					max_freq = freq;
				}
			}	
			res_info = next;
		}
		core_sysclock_freq[cpu] = max_freq;
		res_info = head;
	}
	printk(KERN_ALERT "cpu[0]=%d, cpu[1]=%d, cpu[2]=%d, cpu[3]=%d", 
				core_sysclock_freq[0],
				core_sysclock_freq[1],
				core_sysclock_freq[2],
				core_sysclock_freq[3]);
	/* Select the max frequency of all the cores as the max sysclock frequency */
	for(cpu = 0; cpu < 4; cpu++){
		if(cpus_sysclock_freq < core_sysclock_freq[cpu]) {
			cpus_sysclock_freq = core_sysclock_freq[cpu];	
		}
	}
	free_res_info(head);
	kfree(head);
	printk(KERN_INFO "Sysclock Calculation %d sysclk freq(util:0 ~ 100)\n", cpus_sysclock_freq);	
	sysclock_factor= cpus_sysclock_freq;
	return cpus_sysclock_freq;
}
// return 0 ~ 100(UTIL_FACTOR)
int get_min_freq_per_task(struct res_info_struct * head, struct res_info_struct *res) {
	//div_ceil_int(long long, long long)
	struct res_info_struct *ptr = head->next;
	int min_freq = UTIL_FACTOR;
	long long thisC = timespec_to_ms(&(res->C_reserve));
	long long thisT = timespec_to_ms(&(res->T_reserve));
	long long C_accum = thisC;
	long long lastPeriod = 0;

	while (lastPeriod < thisT) {
		//printk(KERN_ALERT "lastPeiod == %lld", lastPeriod);
		lastPeriod = getNextPeriod(lastPeriod, head, res);
		while (ptr != NULL) {
			if (ptr->tid != res->tid &&
               		     ptr->core_num == res->core_num &&
        	       	     higher_priority(ptr, res) > 0) {
				long long otherC = timespec_to_ms(&(ptr->C_reserve));
				long long otherT = timespec_to_ms(&(ptr->T_reserve));
				int times = div_ceil_int(lastPeriod, otherT);
				C_accum += times * otherC;
				//printk(KERN_ALERT "C_accum == %lld", C_accum);
			}
			ptr = ptr->next;
		}
		// c = c * factor / t;
		// then c become the utilization
		C_accum = C_accum * UTIL_FACTOR;	
		do_div(C_accum, lastPeriod);
		if (C_accum < min_freq) {
			min_freq = C_accum;
		}
		C_accum = thisC;
		ptr = head->next;
	}
	return min_freq;
}
long long getNextPeriod(long long last, 
			struct res_info_struct *head, 
			struct res_info_struct *res) {
	struct res_info_struct *ptr = head->next;
        long long thisT = timespec_to_ms(&(res->T_reserve));
	long long min = thisT;
	while (ptr != NULL) {
		if (ptr->tid != res->tid &&
                    ptr->core_num == res->core_num &&
                    higher_priority(ptr, res) > 0) {
                        long long otherT = timespec_to_ms(&(ptr->T_reserve));
			long long temp = otherT;
			while (temp <= last) {
				temp += otherT;
			}
			if (temp < min) {
				min = temp;
			}
                }
                ptr = ptr->next;
	}
	return min;
}


