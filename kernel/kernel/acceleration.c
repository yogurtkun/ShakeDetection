/**
 * kernel/kernel/acceleration.c
 *
 * polls the accelerometer in the user space
 *
 * and writes it to the kernel
 */

#include <linux/syscalls.h>
#include <linux/acceleration.h>
#include <linux/cred.h>
#include <asm/uaccess.h>
#include <linux/string.h>

#include <linux/list.h>

static struct dev_acceleration acc;
DEFINE_RWLOCK(lock);

static LIST_HEAD(event_list);
DEFINE_RWLOCK(el_rwlock);

/**
 * sys_set_acceleration - set the current device acceleration
 * @pid: pid of the process
 * @len: length in bytes of the bitmask pointed to by user_mask_ptr
 * @user_mask_ptr: user-space pointer to the new cpu mask
 */

SYSCALL_DEFINE1(set_acceleration, struct dev_acceleration __user *, acceleration)
{
	
	struct dev_acceleration *tmp;

	if (current_uid()!=0) {
		return -EACCES;
	}

	if(copy_from_user(tmp, acceleration, sizeof(struct dev_acceleration)))
		return -EFAULT;

	write_lock(&lock);
	memcpy(&acc, tmp, sizeof(struct dev_acceleration));
	write_unlock(&lock);

	return 0;
}

/**
 * sys_accevt_create - Create an event based on motion.
 */

// int accevt_create(struct acc_motion __user *acceleration);
SYSCALL_DEFINE1(accevt_create, struct acc_motion __user *, acceleration)
{
	/* find an available event id */
	int eid;
	struct motion_event *e;
	for (eid = 0; eid < MAX_INT; ++eid) {
		int found = 1;
		list_for_each_entry(e, &event_list, list) {
			if (eid == e->eid) {
				found = 0;
				break;
			}
		}
		if (found)
			break;
	}

	/* construct the struct event*/
	e = kmalloc(sizeof(struct motion_event), GFP_KERNEL);
	e->eid = eid;
	e->triggered = 0;
	e->baseline = kmalloc(sizeof(struct acc_motion), GFP_KERNEL);
	INIT_LIST_HEAD(&e->list);
	if (copy_from_user(e->baseline, accleration, sizeof(struct acc_motion)))
		return -EFAULT;

	/* add the node to the linked list */
	list_add(&e->list, &event_list);
	return eid;
}
