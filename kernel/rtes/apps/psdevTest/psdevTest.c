#include <stdio.h>
#include <pthread.h>
void read_print_2(int *fp);
void read_print_0(int *fp);
void read_print_1(int *fp);
int main(int argc, char *argv[]) {
	pthread_t  thread1, thread2, thread3;
	int fp = open("/dev/psdev", "r");
	int fp1 = open("/dev/psdev1", "r");
//	read_print(fp);
	pthread_create(&thread1, NULL, (void*) &read_print_0, &fp);
	pthread_create(&thread2, NULL, (void*) &read_print_1, &fp);	
	pthread_create(&thread3, NULL, (void*) &read_print_2, &fp1); 
	pthread_join(thread3, NULL);
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	close(fp);
	close(fp1);
	return 0;
}
void read_print_2(int *fp) {
        char buf[1024];
        size_t size;
        int i;
	for (i = 0; i < 1; i++) {
                printf("[fun3]: %d\n", i);
		int ii = 0;
                while (read(*fp, buf, 1000)) {
			;
		}
		printf("%s",buf);
                
        }
}

void read_print_0(int *fp) {
        char buf[1024];
        size_t size;
        int i;
        for (i = 0; i < 1; i++) {
                printf("[fun 0]: %d\n", i);
                int ii = 0;
                while (read(*fp, &buf[ii++], 1)) {
                        ;
                }
                printf("%s",buf);

        }
}
void read_print_1(int *fp) {
        char buf[1024];
        size_t size;
        int i;
        for (i = 0; i < 1; i++) {
                printf("[fun 1]: %d\n", i);
                int ii = 0;
                while (read(*fp, &buf[ii++], 1)) {
                        ;
                }
                printf("%s",buf);

        }
}

