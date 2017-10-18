#include <linux/unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <sys/types.h>
#include "stdio.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "test.h"

int main(int argc, char const *argv[])
{
	struct acc_motion xmotion;
	xmotion.dlt_x = 1000;
	xmotion.dlt_y = 0;
	xmotion.dlt_z = 0;
	xmotion.frq = 10;

	printf("Hello world from user\n");
	int eid;
	eid = syscall(__NR_accevt_create,&xmotion);
	syscall(__NR_accevt_wait, eid);
	printf("X motion!\n");
	return 0;
}
