#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <android/log.h>


#include <helper.h>
jstring
Java_edu_cmu_ziyuans_reservationui_MainActivity_threadList(
		JNIEnv* env, jobject thiz) {
	int num_rt_threads = syscall(__NR_count_rt_threads);
	char string[4 * num_rt_threads + 1];
	getThreadList(string, num_rt_threads);
	__android_log_print(ANDROID_LOG_INFO, "NATIVE", "[threadList]");
	return (*env)->NewStringUTF(env, string);
}
jstring
Java_edu_cmu_ziyuans_reservationui_MainActivity_reserveThreadList(
		JNIEnv* env, jobject thiz) {
	char string[1000000];
	int size = getReserveThreadList(string);
	__android_log_print(ANDROID_LOG_INFO, "NATIVE", "[r-threadList size: %s]", string);
	if (size == 0) {
		return (*env)->NewStringUTF(env, NULL);
	} else {
		return (*env)->NewStringUTF(env, string);
	}
}
jint
Java_edu_cmu_ziyuans_reservationui_MainActivity_setReserve(
		JNIEnv* env, jobject thiz,
		jint threadId, jint budget, jint period, jint cpuid) {
	struct timespec C, T;
	millisecs_to_timespec(&C, budget);
	millisecs_to_timespec(&T, period);
	int retVal = syscall(__NR_set_reserve, threadId, &C, &T, cpuid);
	__android_log_print(ANDROID_LOG_INFO, "NATIVE","[setReserve] Thread id : %d\tBudget : %d\tPeriod : %d\tCPU Id : %d", threadId, budget, period, cpuid);
	__android_log_print(ANDROID_LOG_INFO, "NATIVE","set: %d", retVal);
	return retVal;
}
jint
Java_edu_cmu_ziyuans_reservationui_MainActivity_cancelReserve(
		JNIEnv* env, jobject thiz, jint threadid) {
	int retVal=syscall(__NR_cancel_reserve, threadid);
	__android_log_print(ANDROID_LOG_INFO,"NATIVE","[cancelReserve] Thread id : %d", threadid);
	__android_log_print(ANDROID_LOG_INFO, "NATIVE","cancel: %d", retVal);
	return retVal;
}

