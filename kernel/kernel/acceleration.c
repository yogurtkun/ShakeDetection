#include <linux/mm.h>
#include <linux/module.h>
#include <linux/nmi.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/highmem.h>
#include <asm/mmu_context.h>
#include <linux/interrupt.h>
#include <linux/capability.h>
#include <linux/completion.h>
#include <linux/kernel_stat.h>
#include <linux/debug_locks.h>
#include <linux/perf_event.h>
#include <linux/security.h>
#include <linux/notifier.h>
#include <linux/profile.h>
#include <linux/freezer.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/pid_namespace.h>
#include <linux/smp.h>
#include <linux/threads.h>
#include <linux/timer.h>
#include <linux/rcupdate.h>
#include <linux/cpu.h>
#include <linux/cpuset.h>
#include <linux/percpu.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sysctl.h>
#include <linux/syscalls.h>
#include <linux/times.h>
#include <linux/tsacct_kern.h>
#include <linux/kprobes.h>
#include <linux/delayacct.h>
#include <linux/unistd.h>
#include <linux/pagemap.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/debugfs.h>
#include <linux/ctype.h>
#include <linux/ftrace.h>
#include <linux/slab.h>
#include <linux/init_task.h>
#include <linux/binfmts.h>
#include <linux/context_tracking.h>

#include <sched/sched.h>
#include <linux/acceleration.h>
#include <linux/kfifo.h>

#include <asm/switch_to.h>
#include <asm/tlb.h>
#include <asm/irq_regs.h>
#include <asm/mutex.h>

#include "workqueue_internal.h"
#include "smpboot.h"

#include <trace/events/sched.h>

static struct kfifo acceleration_queue;
static DEFINE_RWLOCK(acceleration_q_lock);

SYSCALL_DEFINE1(accevt_signal,struct dev_acceleration *,acceleration){

	/*Check the privilegios*/
	if (current_cred()->uid != 0)
	{
		return -EACCES;
	}

	if (!kfifo_initialized(&acceleration_queue)){
		if (! kfifo_alloc(&acceleration_queue,sizeof(struct dev_acceleration)* (WINDOW+1),GFP_KERNEL) ){
			return -ENOMEM;
		}
	}

	struct dev_acceleration * acc_data = kmalloc(sizeof(struct dev_acceleration),GFP_KERNEL);
	struct dev_acceleration * del_data = kmalloc(sizeof(struct dev_acceleration),GFP_KERNEL);

	if(copy_from_user(acc_data,acceleration,sizeof(struct dev_acceleration)))
		return -EFAULT;

	write_lock(&acceleration_q_lock);
	if (kfifo_len(&acceleration_queue) == (WINDOW+1)*sizeof(struct dev_acceleration))
	{
		kfifo_out(&acceleration_queue,del_data,sizeof(struct dev_acceleration));
	}

	kfifo_in(&acceleration_queue,acc_data,sizeof(struct dev_acceleration));

	write_unlock(&acceleration_q_lock);

	kfree(acc_data);
	kfree(del_data);


	struct dev_acceleration * fifo_data = kmalloc(sizeof(struct dev_acceleration)*(WINDOW + 1),GFP_KERNEL);
	kfifo_out_peek(&acceleration_queue,fifo_data,kfifo_len(&acceleration_queue));

	

	int i = 0;

	for (i = 0 ; i < kfifo_len(&acceleration_queue)/sizeof(struct dev_acceleration) ; i ++)
	{

		printk("%d %d %d\n",test[i].x,test[i].y,test[i].z);
	}
	printk("********\n");

	kfree(test);

/**
 * kernel/kernel/acceleration.c
 *
 * polls the accelerometer in the user space
 *
 * and writes it to the kernel
 */


static struct dev_acceleration acc;
DEFINE_RWLOCK(lock);

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