#ifndef _LINUX_ACCELERATION_H
#define _LINUX_ACCELERATION_H

#define NOISE 10
#define WINDOW 20


struct dev_acceleration {
	int x; /* acceleration along X-axis */
	int y; /* acceleration along Y-axis */
	int z; /* acceleration along Z-axis */
}; 

#endif
