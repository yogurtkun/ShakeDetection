#include <linux/unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <sys/types.h>
#include "stdio.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "test.h"

int detect(int eid, int id) {
	pid_t pid;
	pid = fork();
	if (pid < 0){ 
		printf("Fork failed\n");
		exit(EXIT_FAILURE);
	}
	if (pid > 0)
		return 0;
	/* children */
	int ret;
	if (ret = syscall(__NR_accevt_wait, &eid)) {
		/* handling the failure */
	}
	if (id == 0)
		printf("%d detected a horizontal shake", id);
	else if (id == 1)
		printf("%d detected a vertical shake", id);
	else if (id == 2)
		printf("%d detected a shake", id);
	return ret;
}

int main(int argc, char const *argv[])
{
	/* TODO: decide the value */
	struct acc_motion xshake;
	xshake.dlt_x = 1000;
	xshake.dlt_y = 0;
	xshake.dlt_z = 0;
	xshake.frq = 10;

	struct acc_motion yshake;
	yshake.dlt_x = 0;
	yshake.dlt_y = 1000;
	yshake.dlt_z = 0;
	yshake.frq = 10;

	struct acc_motion shake;
	shake.dlt_x = 1000;
	shake.dlt_y = 1000;
	shake.dlt_z = 0;
	shake.frq = 10;

	int xeid, yeid, eid;
	xeid = syscall(__NR_accevt_create, &xshake);
	yeid = syscall(__NR_accevt_create, &yshake);
	eid  = syscall(__NR_accevt_create, &shake);
	/* TODO: discuss the meaning of process identifier */
	detect(xeid, 0);
	detect(yeid, 1);
	detect(eid, 2);
	/* close all opened events after a peiod of time */
	sleep(61);
	syscall(__NR_accevt_destroy, &xeid);
	syscall(__NR_accevt_destroy, &yeid);
	syscall(__NR_accevt_destroy, &eid);
	return 0;
}
