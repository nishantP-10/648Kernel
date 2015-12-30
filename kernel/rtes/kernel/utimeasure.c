/* 
 * Create the directory for the virtual files of each thread
 * and create the enabled file
 */
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/reserve.h>
#include <linux/threadattr.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/utimeasure.h>
#include <linux/utility.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>

struct kobject *rtes_kobj;
struct kobject *taskmon_kobj;
struct kobject *util_kobj;
struct kobject *config_kobj;
struct kobject *tasks_kobj;

spinlock_t enabled_lock;
spinlock_t policy_lock;
spinlock_t energy_enable_lock;

int enabled;
int energy_enable;
long energy_total;

long lookup_table[9] = {16887, 29517, 48564, 64708, 79544, 102329, 119984, 138749, 158593};

//char policy[] = "FF";
int policy = 0;
char POLICY_TYPE[POLICY_NUMS][4] = {"FF\0", "NF\0", "BF\0", "WF\0", "LST\0"}; 
static ssize_t enabled_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	int retval;
	spin_lock(&enabled_lock);
	retval = sprintf(buf, "%d\n", enabled);
	spin_unlock(&enabled_lock);
	return retval;
}	

static ssize_t enabled_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
	spin_lock(&enabled_lock);
	sscanf(buf, "%d", &enabled);
	spin_unlock(&enabled_lock);
	return count;
}

static struct kobj_attribute enabled_attribute = __ATTR(enabled, 0666, enabled_show, enabled_store);
//LAB 4  /sys/rtes/config/energy
static ssize_t energy_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	int retval;
	spin_lock(&energy_enable_lock);
	retval = sprintf(buf, "%d\n", energy_enable);
	spin_unlock(&energy_enable_lock);
	return retval;
}

static ssize_t energy_enable_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
	spin_lock(&energy_enable_lock);
	sscanf(buf, "%d", &energy_enable);
	spin_unlock(&energy_enable_lock);
	return count;
}

static struct kobj_attribute energy_enable_attribute = __ATTR(energy, 0666, energy_enable_show, energy_enable_store);
//LAB 4 
int freq_get(void) {
	int freq = 0;
	//get_online_cpus();	
	if (cpu_online(0)) {
		freq = (int)cpufreq_quick_get(0) / 1000;
	} else if (cpu_online(1)) {
		freq = (int)cpufreq_quick_get(1) / 1000;
	} else if (cpu_online(2)) {
		freq = (int)cpufreq_quick_get(2) / 1000;
	} else if (cpu_online(3)) {
		freq = (int)cpufreq_quick_get(3) / 1000;
	}
	//put_online_cpus();
//	freq = (int)cpufreq_get(0) / 1000;
	return freq;
}

long power_get(void) {
	int freq = freq_get();
	long kfa = 0;
	int cpuNum = 0;
	if (freq > 204 && freq < 400) {
		kfa = lookup_table[0];
	} else if (freq >= 400 && freq < 550) {
		kfa = lookup_table[1];
	} else if (freq >= 550 && freq < 700) {
		kfa = lookup_table[2];
	} else if (freq >= 700 && freq < 800) {
		kfa = lookup_table[3];
	} else if (freq >= 800 && freq < 950) {
		kfa = lookup_table[4];
	} else if (freq >= 950 && freq < 1050) {
		kfa = lookup_table[5];
	} else if (freq >= 1050 && freq < 1150) {
		kfa = lookup_table[6];
	} else if (freq >= 1150 && freq < 1250) {
		kfa = lookup_table[7];
	} else if (freq >= 1250 && freq < 1350) {
		kfa = lookup_table[8];
	}
	//get_online_cpus();
	if (cpu_online(0)) {
		cpuNum++;
	}
	if (cpu_online(1)) {
		cpuNum++;
	}
	if (cpu_online(2)) {
		cpuNum++;
	}
	if (cpu_online(3)) {
		cpuNum++;
	}
	//put_online_cpus();
	return (kfa * 442 / 100000 + 25) * cpuNum;
}
//LAB 4  /sys/rtes/freq
static ssize_t freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	int retval;
	unsigned long flags;
	spin_lock_irqsave(&energy_enable_lock, flags);
	if (energy_enable) {
		retval = sprintf(buf, "%d\n", freq_get());
	} else {
		retval = sprintf(buf, "%d\n", 0);
	}
	spin_unlock_irqrestore(&energy_enable_lock, flags);
	return retval;
}
static ssize_t freq_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
	return count;
}
static struct kobj_attribute freq_attribute = __ATTR(freq, 0666, freq_show, freq_store);

//LAB 4  /sys/rtes/power
static ssize_t power_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	int retval;
	unsigned long flags;
	spin_lock_irqsave(&energy_enable_lock, flags);
	if (energy_enable) {
		retval = sprintf(buf, "%ld\n", power_get());
	} else {
		retval = sprintf(buf, "%ld\n", 0);
	}
	spin_unlock_irqrestore(&energy_enable_lock, flags);
	return retval;
}
static ssize_t power_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
	return count;
}
static struct kobj_attribute power_attribute = __ATTR(power, 0666, power_show, power_store);
//LAB 4 /sys/rtes/energy
static ssize_t energy_total_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	int retval;
	unsigned long flags;
	spin_lock_irqsave(&energy_enable_lock, flags);
	if (energy_enable) {
		retval = sprintf(buf, "%ld\n", energy_total);
	} else {
		retval = sprintf(buf, "%ld\n", 0);
	}
	spin_unlock_irqrestore(&energy_enable_lock, flags);
	return retval;
}
static ssize_t energy_total_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
	unsigned long flags;
	spin_lock_irqsave(&energy_enable_lock, flags);
	energy_total = 0;
	spin_unlock_irqrestore(&energy_enable_lock, flags);
	return count;
}
static struct kobj_attribute energy_total_attribute = __ATTR(energy, 0666, energy_total_show, energy_total_store);

//LAB4 /sys/rtes/tasks/pid/energy
static ssize_t energy_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	int retval;
	unsigned long flags;
	struct reserve_struct *reserve;
	reserve = container_of(attr, struct reserve_struct, energy_kobj_attr);
	spin_lock_irqsave(&reserve->energy_lock, flags);
	retval = sprintf(buf, "%ld\n", reserve->thattr->energy);
	spin_unlock_irqrestore(&reserve->energy_lock, flags);
	return retval;
}
static ssize_t energy_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
	return count;
}

int energy_init(unsigned int pid) {
	int retval = 0;
	char *pathname;
	struct kobject *pid_kobj;
	struct kobj_attribute *energy_attribute;
	struct task_struct *g, *t;
	pathname = (char*) kmalloc(100 * sizeof(char), GFP_KERNEL);
	sprintf(pathname, "%d", pid);
	pid_kobj = kobject_create_and_add(pathname, tasks_kobj);
	do_each_thread(g, t) {
		if (t->pid == pid) {
			t->reservation.pid_kobject = pid_kobj;
			energy_attribute = &(t->reservation.energy_kobj_attr);
			energy_attribute->attr.name = "energy";
			energy_attribute->attr.mode = 0666;
			energy_attribute->show = energy_show;
			energy_attribute->store = energy_store;
			t->reservation.thattr->energy = 0;
			spin_lock_init(&(t->reservation.energy_lock));
			retval = sysfs_create_file(pid_kobj, &(energy_attribute->attr));
			goto endofenergyinit;
		}
	}while_each_thread(g, t);
endofenergyinit: 
	return retval;
}		
void energy_delete(unsigned int pid) {
	char *pathname;
	struct kobject *pid_kobj;
	struct task_struct *g, *t;
	pathname = (char*) kmalloc(100 * sizeof(char), GFP_KERNEL);
	sprintf(pathname, "%d", pid);
	do_each_thread(g, t) {
		if (t->pid == pid) {
			pid_kobj = t->reservation.pid_kobject;
			kobject_put(pid_kobj);
			goto endofenergydel;
		}
	}while_each_thread(g, t);
endofenergydel:
	return;
}

static ssize_t util_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	int retval;
	unsigned long flags;
	struct reserve_struct *reserve;
	//printk(KERN_ALERT "Before container_of\n");
	reserve = container_of(attr, struct reserve_struct, thattribute);
	spin_lock_irqsave(&reserve->reserve_lock, flags);
	retval = sprintf(buf, "%s", reserve->thattr->val);
	//printk(KERN_ALERT "inside Lock tid: %d\n",reserve->tid);
	memset(reserve->thattr->val,0,2048);
	//printk(KERN_ALERT "inside Lock2\n");
	spin_unlock_irqrestore(&reserve->reserve_lock, flags);
	return retval;
}

static ssize_t util_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
	return count;
}

int util_init(unsigned int pid) {
	int retval = 0;
	char *filename;
	struct kobj_attribute *util_attribute;
	struct task_struct *g, *t;
	filename = (char*) kmalloc(100 * sizeof(char), GFP_KERNEL);
	sprintf(filename, "%d", pid);
	do_each_thread(g, t) {
		if (t->pid == pid) {

			util_attribute = &(t->reservation.thattribute);
			util_attribute->attr.name = filename;
			util_attribute->attr.mode = 0666;
			util_attribute->show = util_show;
			util_attribute->store = util_store;
			retval = sysfs_create_file(util_kobj, &(util_attribute->attr));
			goto endofinit;
		}
	}while_each_thread(g, t);
endofinit:
	return retval;
}

void util_delete(unsigned int pid) {
	char *filename;
	struct attribute attr;
	filename = (char*) kmalloc(100 * sizeof(char), GFP_KERNEL);
	sprintf(filename, "%d", pid);
	attr.name = filename;
	sysfs_remove_file(util_kobj, &attr);
}
// LAB 3
static ssize_t reserves_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	struct task_struct *task_list;
        size_t real_size = 0;
        real_size = sprintf(buf,"TID\tPID\tPRIO\tCPU\tNAME\n");
	// Protect shared resourse among multi CPUs for structrue
	rcu_read_lock();
        for_each_process(task_list) {
		char tBuf[200];
		int len = 0;
		unsigned long flags;
		struct reserve_struct res;
		// Protect share resource for interruptions, and save contexts
		spin_lock_irqsave(&(task_list->reservation.reserve_lock), flags);		
          	res = task_list->reservation; 
                //if (task_list->rt_priority == 0 || task_list->rt_priority >= 100 || 
		//	res.isActive == false) {
		if (res.isActive == false) {
			spin_unlock_irqrestore(&(task_list->reservation.reserve_lock), flags);
                        continue;
                }
                len = sprintf(tBuf, "%d\t%d\t%d\t%d\t%s\n", task_list->pid, task_list->tgid, task_list->rt_priority, res.core_num, task_list->comm);
		spin_unlock_irqrestore(&(task_list->reservation.reserve_lock), flags);
		// PAGE_SIZE is defined by linux, the size of *buf
		if ((len + real_size) > PAGE_SIZE) {
			break;
		} else {
			strcpy(&(buf[real_size]), tBuf);
			real_size += len;		
		}
        }
	rcu_read_unlock();
        return real_size;
}

static ssize_t reserves_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
        return count;
}

static struct kobj_attribute reserves_attribute = __ATTR(reserves, 0666, reserves_show, reserves_store);

static ssize_t policy_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
        int retval;
        spin_lock(&policy_lock);
        retval = sprintf(buf, "%s\n", POLICY_TYPE[policy]);
        spin_unlock(&policy_lock);
        return retval;
} 

static ssize_t policy_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
	int i = 0;
	spin_lock(&policy_lock);
	if (has_res() == true) {
		printk(KERN_ALERT "CANNOT change policy\n");
		spin_unlock(&policy_lock);
		return -EBUSY;
	}
	printk(KERN_ALERT "CAN change policy"); 
	for (i = 0; i < POLICY_NUMS; i++) {
		if ((i < POLICY_NUMS - 1 && strncmp(POLICY_TYPE[i], buf, 2) == 0) || 
		    (i == POLICY_NUMS - 1 && strncmp(POLICY_TYPE[i], buf, 3) == 0)){
			policy = i;
			printk(KERN_ALERT "Change Policy to %d\n", policy);
			init_bins();
			break;
		}
	}
        spin_unlock(&policy_lock);
        return count;
}
static struct kobj_attribute partition_policy_attribute = __ATTR(partition_policy, 0666, policy_show, policy_store);


static int __init utimeasure_init(void) {
	int retval;
	rtes_kobj = kobject_create_and_add("rtes", NULL);
	if (!rtes_kobj) {
		return -ENOMEM;
	}
	taskmon_kobj = kobject_create_and_add("taskmon", rtes_kobj);
	if (!taskmon_kobj) {
		return -ENOMEM;
	}
	util_kobj = kobject_create_and_add("util", taskmon_kobj);
	if (!util_kobj) {
		return -ENOMEM;
	}
	retval = sysfs_create_file(taskmon_kobj, &(enabled_attribute.attr));
	if (retval) {
		kobject_put(rtes_kobj);
		kobject_put(taskmon_kobj);
		kobject_put(util_kobj);
	}
		
	// LAB 3
	// create /sys/rtes/reserves virtual sysfs
	retval = sysfs_create_file(rtes_kobj, &(reserves_attribute.attr));
	if (retval) {
                kobject_put(rtes_kobj);
                kobject_put(taskmon_kobj);
                kobject_put(util_kobj);
        }
	// create /sys/rtes/partition_policy virtual sysfs
	retval = sysfs_create_file(rtes_kobj, &(partition_policy_attribute.attr));
        if (retval) {
                kobject_put(rtes_kobj);
                kobject_put(taskmon_kobj);
                kobject_put(util_kobj);
        }
	// LAB 4
	tasks_kobj = kobject_create_and_add("tasks", rtes_kobj);
	// create /sys/rtes/config/energy
	config_kobj = kobject_create_and_add("config", rtes_kobj);
	retval = sysfs_create_file(config_kobj, &(energy_enable_attribute.attr));
	if (retval) {
		kobject_put(rtes_kobj);
		kobject_put(config_kobj);
	}
	// create /sys/rtes/freq
	retval = sysfs_create_file(rtes_kobj, &(freq_attribute.attr));
	if (retval) {
		kobject_put(rtes_kobj);
	}
	// create /sys/rtes/power
	retval = sysfs_create_file(rtes_kobj, &(power_attribute.attr));
	if (retval) {
		kobject_put(rtes_kobj);
	}
	// create /sys/rtes/energy
	retval = sysfs_create_file(rtes_kobj, &(energy_total_attribute.attr));
	energy_total = 0;
	if (retval) {
		kobject_put(rtes_kobj);
	}
	return retval;
}

core_initcall(utimeasure_init);
