#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	FILE *fd;
	int tid;
	int freq;
	long power;
	long energy;
	char path[50];

	if (argc != 2) {
		printf("commond format error! energymon <tid>\n");
		return -1;
	}
	tid = atoi(argv[1]);
	printf("FREQ(MHZ)	POWER(mW)	ENERGY(mJ)\n");
	while (1) {
		if ((fd = fopen("/sys/rtes/freq", "r")) == NULL) {
			return -1;
		}
		fscanf(fd, "%d", &freq);
		if ((fd = fopen("/sys/rtes/power", "r")) == NULL) {
			return -1;
		}
		fscanf(fd, "%ld", &power);
		if (tid != 0) {
			sprintf(path, "/sys/rtes/tasks/%d/energy", tid);
			if ((fd = fopen(path, "r")) == NULL) {
				return -1;
			}
		} else {
			if ((fd = fopen("/sys/rtes/energy", "r")) == NULL) {
				return -1;
			}
		}
		fscanf(fd, "%ld", &energy);
		printf("%d\t%16ld\t%10ld\t\n", freq, power, energy);
		sleep(1);
	}
	return 0;
}
		
