#include <linux/unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <sys/types.h>
#include "stdio.h"
#include <errno.h>
#include <string.h>

#define __NR_accevt_signal 252

struct dev_acceleration {
	int x; /* acceleration along X-axis */
	int y; /* acceleration along Y-axis */
	int z; /* acceleration along Z-axis */
}; 

int main(int argc, char const *argv[])
{
	struct dev_acceleration data;
	data.x = 0;
	data.y = 1;
	data.z = 2;
	printf("Hello world from user\n");
	int i = 0;
	for (i = 0; i < 30; ++i)
	{
		data.x = i*3+0;
		data.y = i*3+1;
		data.z = i*3+2;
		long x = syscall(__NR_accevt_signal,&data);
	}
	return 0;
}
