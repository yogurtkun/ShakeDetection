#ifndef _LINUX_ACCELERATION_H
#define _LINUX_ACCELERATION_H

#define NOISE 10
#define WINDOW 20

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


#endif

