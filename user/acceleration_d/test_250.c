#include <linux/unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <sys/types.h>
#include "stdio.h"
#include <errno.h>
#include <string.h>

#define __NR_accevt_create 250

struct acc_motion {
     unsigned int dlt_x; /* +/- around X-axis */
     unsigned int dlt_y; /* +/- around Y-axis */
     unsigned int dlt_z; /* +/- around Z-axis */
     
     unsigned int frq;   /* Number of samples that satisfies:
                          sum_each_sample(dlt_x + dlt_y + dlt_z) > NOISE */
};

struct dev_acceleration {
	int x; /* acceleration along X-axis */
	int y; /* acceleration along Y-axis */
	int z; /* acceleration along Z-axis */
}; 

int main(int argc, char const *argv[])
{
	struct acc_motion baseline; 
	baseline.dlt_x = 0;
	baseline.dlt_x = 1;
	baseline.dlt_x = 2;
	int i;
	for (i = 0; i < 30; ++i)
	{
		baseline.dlt_x = 0;
		baseline.dlt_x = 1;
		baseline.dlt_x = 2;
		int eid = syscall(__NR_accevt_create, &baseline);
		printf("%d\n", eid);
	}

	return 0;

}
