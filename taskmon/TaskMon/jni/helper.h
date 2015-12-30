#ifndef HELPER_H
#define HELPER_H
#include <linux/unistd.h>
#include <linux/mystruct.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/time.h>
#include <dirent.h>
#include <android/log.h>
#include <stdlib.h>

extern int getThreadList(char *string, int num_rt_threads);
extern int getReserveThreadList(char *string);
extern void millisecs_to_timespec(struct timespec *time, unsigned long ms);

#endif
