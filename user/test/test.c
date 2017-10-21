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
		
		exit(EXIT_FAILURE);
	}
	
	fprintf(stdout,"%d detected a shake!\n",id );

	return ret;
}

void init_acc_motion(struct acc_motion * acc,int x,int y, int z,int frq){

	acc->dlt_x = x;
	acc->dlt_y = y;
	acc->dlt_z = z;
	acc->frq = frq;

	return;
}

int main(int argc, char const *argv[])
{
	/* TODO: decide the value */
	struct acc_motion xshake;
	init_acc_motion(&xshake,200,0,0,10);

	struct acc_motion yshake;
	init_acc_motion(&yshake,0,200,0,10);

	struct acc_motion shake;
	init_acc_motion(&shake,200,200,0,10);

	int xeid, yeid, eid;
	xeid = syscall(__NR_accevt_create, &xshake);
	yeid = syscall(__NR_accevt_create, &yshake);
	eid  = syscall(__NR_accevt_create, &shake);

	pid_t pid = fork();

	if(pid != 0){
		sleep(61);
		/* close all opened events after a peiod of time */
		syscall(__NR_accevt_destroy,&xeid);
		syscall(__NR_accevt_destroy,&yeid);
		syscall(__NR_accevt_destroy,&eid);
		wait();
	}else{
		/* TODO: discuss the meaning of process identifier */
		detect(xeid,0);
		detect(yeid,1);
		detect(eid,2);
	}

	return 0;
}
