#include <linux/unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <sys/types.h>
#include "stdio.h"
#include <errno.h>
#include <string.h>

/* Please use the same syscall number as indicated in the homework 3 instruction */

#define __NR_set_acceleration 249
#define __NR_accevt_create 250
#define __NR_accevt_wait 251
#define __NR_accevt_signal 252
#define __NR_accevt_destroy 253

#define TIME_INTERVAL  200


struct dev_acceleration {
	int x; /* acceleration along X-axis */
	int y; /* acceleration along Y-axis */
	int z; /* acceleration along Z-axis */
}; 


struct acc_motion {

     unsigned int dlt_x; /* +/- around X-axis */
     unsigned int dlt_y; /* +/- around Y-axis */
     unsigned int dlt_z; /* +/- around Z-axis */
     
     unsigned int frq;   /* Number of samples that satisfies:
                          sum_each_sample(dlt_x + dlt_y + dlt_z) > NOISE */
};


int main(int argc, char const *argv[])
{
	// int id = 0;
	struct dev_acceleration sig;
	sig.x = 0;
	sig.y = 1;
	sig.z = 2;

	struct acc_motion data;
	data.dlt_x = 0;
	data.dlt_y = 0;
	data.dlt_z = 0;
	data.frq = 0;

	// printf("Hello world from user\n");

	// syscall(__NR_accevt_create,&data);
	// syscall(__NR_accevt_create,&data);
	// syscall(__NR_accevt_create,&data);
	// syscall(__NR_accevt_create,&data);

	// while(scanf("%d",&id) != 0)
	// {
	// 	int x = syscall(__NR_accevt_wait,id);
	// 	printf("%d\n",x );
	// }

	if (fork() != 0){
		syscall(__NR_accevt_create,&data);
		sleep(5);
		printf("Hello from parent to wake up\n");
		syscall(__NR_accevt_signal,&sig);
	}
	else{
		sleep(1);
		syscall(__NR_accevt_wait,0);
		printf("Return success\n");
	}

	return 0;
}