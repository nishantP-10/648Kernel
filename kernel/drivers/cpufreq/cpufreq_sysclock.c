/*
 *  linux/drivers/cpufreq/cpufreq_sysclock.c
 *
 *  Copyright (C) 2002 - 2003 Dominik Brodowski <linux@brodo.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/init.h>

#define MAX_FACTOR 100

DEFINE_MUTEX(sysclock_mutex);
static DEFINE_PER_CPU(unsigned int, cpu_max_freq);
static DEFINE_PER_CPU(unsigned int, cpu_min_freq);
static DEFINE_PER_CPU(unsigned int, cpu_cur_freq); /* current CPU freq */
static DEFINE_PER_CPU(unsigned int, cpu_set_freq); /* CPU freq desired by
							userspace */
static DEFINE_PER_CPU(unsigned int, cpu_is_managed);
static DEFINE_PER_CPU(struct cpufreq_policy *, temp_policy);
extern int sysclock_factor;
int curr_frequency =0;
unsigned int freq_supported [] = {340000, 475000,640000,760000,860000,1000000,1100000, 1200000,1300000};
int cpus_using_userspace_governor = 0;
static int sysclock_switch(struct cpufreq_policy *, unsigned int);

static int
userspace_cpufreq_notifier(struct notifier_block *nb, unsigned long val,
	void *data)
{
	struct cpufreq_freqs *freq = data;

	if (!per_cpu(cpu_is_managed, freq->cpu))
		return 0;

	pr_debug("saving cpu_cur_freq of cpu %u to be %u kHz\n",
			freq->cpu, freq->new);
	per_cpu(cpu_cur_freq, freq->cpu) = freq->new;

	return 0;
}

static struct notifier_block userspace_cpufreq_notifier_block = {
	.notifier_call  = userspace_cpufreq_notifier
};

static int cpufreq_governor_sysclock(struct cpufreq_policy *policy,
					unsigned int event)
{
	int p = 0;
	uint64_t x = 0;
	int curr_freq=0;
	policy->min = 340000;
	policy->max = 1300000;
	mutex_lock(&sysclock_mutex);
	x = policy->max * sysclock_factor;
	mutex_unlock(&sysclock_mutex);
	do_div(x,MAX_FACTOR);
	curr_freq = x;
	
	for(p=0 ; p < (sizeof(freq_supported)/sizeof(freq_supported[0])); p++){
		if(freq_supported[p] > curr_freq){
			curr_freq = freq_supported[p];
			break;
		}
	}

	mutex_lock(&sysclock_mutex);
	curr_frequency = curr_freq;
	mutex_unlock(&sysclock_mutex);
	policy->cur = curr_freq;
	return sysclock_switch(policy, event);
	/*
	switch (event) {
	case CPUFREQ_GOV_START:
		if(!cpu_online(policy->cpu))		
			return -EINVAL;
		in_use=1;	
		printk(KERN_ALERT "freq set to start %u", curr_freq );	
		__cpufreq_driver_target(policy, curr_freq, CPUFREQ_RELATION_L);
		break;
	case CPUFREQ_GOV_STOP:
		if(!cpu_online(policy->cpu))
			return -EINVAL;
		in_use=0;
		printk(KERN_ALERT "freq set to stop %u", curr_freq);
		policy->max =0;
		policy->min =0;
		policy->cur = 0;
		policy = NULL;	
		break;	
	case CPUFREQ_GOV_LIMITS:
		pr_debug("setting to %u kHz because of event %u\n",
							curr_freq, event);
		printk(KERN_ALERT "freq set to limits %u", curr_freq);	
		__cpufreq_driver_target(policy, curr_freq, CPUFREQ_RELATION_L);
		break;
	default:
		break;
	}
	return 0;
*/
}
static int sysclock_switch(struct cpufreq_policy *policy, unsigned int event)
{
	unsigned int cpu = policy->cpu;
	int rc = 0;
	
	switch(event){
		case CPUFREQ_GOV_START:
                if (!cpu_online(cpu))
                        return -EINVAL;
                per_cpu(temp_policy, cpu) =policy;
                BUG_ON(!policy->cur);
                mutex_lock(&sysclock_mutex);

                if (cpus_using_userspace_governor == 0) {
                        cpufreq_register_notifier(
                                        &userspace_cpufreq_notifier_block,
                                        CPUFREQ_TRANSITION_NOTIFIER);
                }
                cpus_using_userspace_governor++;

                per_cpu(cpu_is_managed, cpu) = 1;
                per_cpu(cpu_min_freq, cpu) = policy->min;
                per_cpu(cpu_max_freq, cpu) = policy->max;
                per_cpu(cpu_cur_freq, cpu) = policy->cur;
                per_cpu(cpu_set_freq, cpu) = policy->cur;
                pr_debug("managing cpu %u started "
                        "(%u - %u kHz, currently %u kHz)\n",
                                cpu,
                                per_cpu(cpu_min_freq, cpu),
                                per_cpu(cpu_max_freq, cpu),
                                per_cpu(cpu_cur_freq, cpu));

                mutex_unlock(&sysclock_mutex);
                break;
		
		case CPUFREQ_GOV_STOP:
                mutex_lock(&sysclock_mutex);
                cpus_using_userspace_governor--;
                if (cpus_using_userspace_governor == 0) {
                        cpufreq_unregister_notifier(
                                        &userspace_cpufreq_notifier_block,
                                        CPUFREQ_TRANSITION_NOTIFIER);
                }

                per_cpu(cpu_is_managed, cpu) = 0;
                per_cpu(cpu_min_freq, cpu) = 0;
                per_cpu(cpu_max_freq, cpu) = 0;
                per_cpu(cpu_set_freq, cpu) = 0;
                per_cpu(temp_policy, cpu) =NULL;
                pr_debug("managing cpu %u stopped\n", cpu);
                mutex_unlock(&sysclock_mutex);
                break;
		
		case CPUFREQ_GOV_LIMITS:
                mutex_lock(&sysclock_mutex);
                pr_debug("limit event for cpu %u: %u - %u kHz, "
                        "currently %u kHz, last set to %u kHz\n",
                        cpu, policy->min, policy->max,
                        per_cpu(cpu_cur_freq, cpu),
                        per_cpu(cpu_set_freq, cpu));
		per_cpu(cpu_min_freq, cpu) = policy->min;
                per_cpu(cpu_max_freq, cpu) = policy->max;
                per_cpu(cpu_cur_freq, cpu) = policy->cur;
                per_cpu(cpu_set_freq, cpu) = policy->cur;
                if (policy->max < per_cpu(cpu_set_freq, cpu)) {
                        __cpufreq_driver_target(policy, policy->max,
                                                CPUFREQ_RELATION_H);
                } else if (policy->min > per_cpu(cpu_set_freq, cpu)) {
                        __cpufreq_driver_target(policy, policy->min,
                                                CPUFREQ_RELATION_L);
                } else {
                        __cpufreq_driver_target(policy,
                                                per_cpu(cpu_set_freq, cpu),
                                                CPUFREQ_RELATION_L);
                }
                per_cpu(cpu_min_freq, cpu) = policy->min;
                per_cpu(cpu_max_freq, cpu) = policy->max;
                per_cpu(cpu_cur_freq, cpu) = policy->cur;
                mutex_unlock(&sysclock_mutex);
                break;
        }
        return rc;
}
	



#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_SYSCLOCK
static
#endif
struct cpufreq_governor cpufreq_gov_sysclock = {
	.name		= "sysclock",
	.governor	= cpufreq_governor_sysclock,
	.owner		= THIS_MODULE,
};

static int __init cpufreq_gov_sysclock_init(void)
{
	return cpufreq_register_governor(&cpufreq_gov_sysclock);
}


static void __exit cpufreq_gov_sysclock_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_sysclock);
}


MODULE_AUTHOR("Nishant Parekh");
MODULE_DESCRIPTION("CPUfreq policy governor 'sysclock'");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_SYSCLOCK
fs_initcall(cpufreq_gov_sysclock_init);
#else
module_init(cpufreq_gov_sysclock_init);
#endif
module_exit(cpufreq_gov_sysclock_exit);
