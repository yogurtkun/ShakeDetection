/**
 * kernel/kernel/acceleration.c
 *
 * polls the accelerometer in the user space
 *
 * and writes it to the kernel
 */

#include <linux/syscall.h>
#include <linux/acceleration.h>
#include <linux/cred.h>
#include <asm-generic/uaccess.h>
#include <linux/string.h>

static struct dev_acceleration acc;
DEFINE_RWLOCK(mr_rwlock);

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
		return -EACCESS;
	}

	if(copy_from_user(tmp, acceleration, sizeof(struct dev_acceleration)))
		return -EFAULT;

	write_lock(&mr_rwlock);
	memcpy(&acc, tmp, sizeof(struct dev_acceleration));
	write_unlock(&mr_lock);

	return 0;
}