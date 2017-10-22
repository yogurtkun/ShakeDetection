/*
 * Columbia University
 * COMS W4118 Fall 2017
 * Homework 3
 *
 * Adapted from Fall 2016 solution
 *
 */
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>	/* <-- This is a good place to look! */

#include "accelerationd.h"


static int effective_linaccel_sensor = -1;
int DAEMON_TYPE = 1;		/* indicating the behavior of daemon */

/* helper functions which you should use */
static int open_sensors(struct sensors_module_t **hw_module,
			struct sensors_poll_device_t **poll_device);
static void enumerate_sensors(const struct sensors_module_t *sensors);
static struct dev_acceleration *poll_sensor_data_emulator(void);
static int poll_sensor_data(struct sensors_poll_device_t *sensors_device);

void daemon_mode(void)
{
	pid_t pid, sid;

	pid = fork();

	if (pid < 0) {
		printf("Fork failed\n");
		exit(EXIT_FAILURE);
	}

	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	umask(0);

	int flog =
	    open("./acceleration_log.txt", O_WRONLY | O_APPEND | O_CREAT);

	sid = setsid();
	if (sid < 0) {
		printf("Set sid failed\n");
		exit(EXIT_FAILURE);
	}

	if (chdir("/") < 0) {
		printf("Change directory failed\n");
		exit(EXIT_FAILURE);
	}

	dup2(flog, 1);
	close(0);
	close(2);
}

int main(int argc, char **argv)
{
	struct sensors_module_t *sensors_module = NULL;
	struct sensors_poll_device_t *sensors_device = NULL;
	int errsv = 0;		/* Error that we pull up from system call */

	if (argv[1] && strcmp(argv[1], "-e") == 0)
		goto emulation;
	if (argv[1] && strcmp(argv[1], "-o") == 0)
		DAEMON_TYPE = 0;

	/*
	 * TODO: Implement your code to make this process a daemon in
	 * daemon_mode function
	 */
	daemon_mode();

	printf("Opening sensors...\n");
	if (open_sensors(&sensors_module, &sensors_device) < 0) {
		printf("open_sensors failed\n");
		return EXIT_FAILURE;
	}
	enumerate_sensors(sensors_module);

	printf("turn me into a daemon!\n");
	while (1) {
	      emulation:
		errsv = poll_sensor_data(sensors_device);
		if (errsv != 0)
			break;
		/* TODO: Define time interval and call usleep */
		usleep(TIME_INTERVAL);
	}

	fprintf(stdout, "Daemon process exit!\n");
	/*return EXIT_SUCCESS;*/
	return errsv;
}


static int poll_sensor_data(struct sensors_poll_device_t *sensors_device)
{
	struct dev_acceleration *cur_acceleration;
	int err = 0;		/* Return value of this function */

	if (effective_linaccel_sensor < 0) {
		/* emulation */
		cur_acceleration = poll_sensor_data_emulator();
		fprintf(stdout, "Acceleration data: %d %d %d\n",
			cur_acceleration->x, cur_acceleration->y,
			cur_acceleration->z);
		/*
		 * TODO: You have the acceleration here -
		 * scale it and send it to your kernel
		 */
		if (DAEMON_TYPE == 1) {
			syscall(__NR_accevt_signal, cur_acceleration);
		} else {	/* original */
			syscall(__NR_set_acceleration, cur_acceleration);
		}
	} else {
		sensors_event_t buffer[128];
		ssize_t buf_size = sizeof(buffer) / sizeof(buffer[0]);
		ssize_t count = sensors_device->poll(sensors_device,
						     buffer,
						     buf_size);
		/*
		 * TODO: You have the acceleration here - scale it and
		 * send it to kernel
		 */
		if (count == 0) {
			printf("sensors device poll failed!\n");
			exit(EXIT_FAILURE);
		}
		cur_acceleration = malloc(sizeof(struct dev_acceleration));
		cur_acceleration->x =
		    (int) (100 * buffer[count - 1].acceleration.x);
		cur_acceleration->y =
		    (int) (100 * buffer[count - 1].acceleration.y);
		cur_acceleration->z =
		    (int) (100 * buffer[count - 1].acceleration.z);

		fprintf(stdout, "Acceleration data: %d %d %d\n",
			cur_acceleration->x, cur_acceleration->y,
			cur_acceleration->z);
		if (DAEMON_TYPE == 1) {
			syscall(__NR_accevt_signal, cur_acceleration);
		} else {	/* original */
			syscall(__NR_set_acceleration, cur_acceleration);
		}
		free(cur_acceleration);
	}
	return err;
}



/*  DO NOT MODIFY BELOW THIS LINE  */
/*---------------------------------*/



static struct dev_acceleration *poll_sensor_data_emulator(void)
{
	int cur_x, cur_y, cur_z;
	struct dev_acceleration *ad;

	ad = (struct dev_acceleration *)
	    malloc(sizeof(struct dev_acceleration));

	if (!ad) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(1);
	}

	FILE *fp = fopen("/data/misc/acceleration", "r");

	if (!fp) {
		free(ad);
		return 0;
	}
	fscanf(fp, "%d, %d, %d", &cur_x, &cur_y, &cur_z);
	fclose(fp);
	ad->x = cur_x;
	ad->y = cur_y;
	ad->z = cur_z;

	return ad;
}




static int open_sensors(struct sensors_module_t **mSensorModule,
			struct sensors_poll_device_t **mSensorDevice)
{
	int err = hw_get_module(SENSORS_HARDWARE_MODULE_ID,
				(hw_module_t const **) mSensorModule);

	if (err) {
		printf("couldn't load %s module (%s)",
		       SENSORS_HARDWARE_MODULE_ID, strerror(-err));
	}

	if (!*mSensorModule)
		return -1;

	err = sensors_open(&((*mSensorModule)->common), mSensorDevice);

	if (err) {
		printf("couldn't open device for module %s (%s)",
		       SENSORS_HARDWARE_MODULE_ID, strerror(-err));
	}

	if (!*mSensorDevice)
		return -1;

	const struct sensor_t *list;
	ssize_t count =
	    (*mSensorModule)->get_sensors_list(*mSensorModule, &list);
	size_t i;

	for (i = 0; i < (size_t) count; i++)
		(*mSensorDevice)->activate(*mSensorDevice, list[i].handle,
					   1);
	return 0;
}

static void enumerate_sensors(const struct sensors_module_t *sensors)
{
	int nr, s;
	const struct sensor_t *slist = NULL;

	if (!sensors)
		printf("going to fail\n");

	nr = sensors->get_sensors_list((struct sensors_module_t *) sensors,
				       &slist);
	if (nr < 1 || slist == NULL) {
		printf("no sensors!\n");
		return;
	}

	for (s = 0; s < nr; s++) {
		printf("%s (%s) v%d\n\tHandle:%d, type:%d, max:%0.2f, "
		       "resolution:%0.2f \n", slist[s].name,
		       slist[s].vendor, slist[s].version, slist[s].handle,
		       slist[s].type, slist[s].maxRange,
		       slist[s].resolution);

		if (slist[s].type == SENSOR_TYPE_ACCELEROMETER)
			effective_linaccel_sensor = slist[s].handle;	/*the sensor ID */

	}
}
