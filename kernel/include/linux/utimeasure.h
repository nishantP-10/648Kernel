#ifndef _LINUX_UTIMEASURE_H_
#define _LINUX_UTIMEASURE_H_

#define POLICY_NUMS 5
extern int enabled;
extern int energy_enable;
//extern char policy[];
extern int policy;
extern long energy_total;
//extern char * POLICY_TYPE[4];
extern spinlock_t enabled_lock;
extern spinlock_t energy_enable_lock;
//long lookup_table[9] = {16887, 29517, 48564, 64708, 79544, 102329, 119984, 138749, 158593};
int util_init(unsigned int pid);
void util_delete(unsigned int pid);
int energy_init(unsigned int pid);
void energy_delete(unsigned int pid);
long power_get(void);
int freq_get(void);
#endif
