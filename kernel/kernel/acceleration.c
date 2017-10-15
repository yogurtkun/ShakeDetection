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

#include <linux/kernel.h>

#include <trace/events/sched.h>

#include <linux/slab.h>
#include <linux/list.h>
#include <linux/types.h>

static struct kfifo acceleration_queue;
static DEFINE_RWLOCK(acceleration_q_lock);

static LIST_HEAD(event_list);
static DEFINE_RWLOCK(event_list_lock);

SYSCALL_DEFINE1(accevt_signal,struct dev_acceleration *,acceleration){

	printk("Hello from parent-2\n");
	/*Check the privilegios*/
	if (current_cred()->uid != 0)
	{
		return -EACCES;
	}

	printk("Hello from parent-1\n");

	if (!kfifo_initialized(&acceleration_queue)){
		if (kfifo_alloc(&acceleration_queue,sizeof(struct dev_acceleration)* (WINDOW+1),GFP_KERNEL) ){
			return -ENOMEM;
		}
	}

	printk("Hello from parent0\n");

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

	printk("Hello from parent1\n");

	kfree(acc_data);
	kfree(del_data);


	struct dev_acceleration * fifo_data = kmalloc(sizeof(struct dev_acceleration)*(WINDOW + 1),GFP_KERNEL);

	read_lock(&acceleration_q_lock);
	kfifo_out_peek(&acceleration_queue,fifo_data,kfifo_len(&acceleration_queue));
	read_unlock(&acceleration_q_lock);

	/*This is test program*/
	// int i = 0;

	// for (i = 0 ; i < kfifo_len(&acceleration_queue)/sizeof(struct dev_acceleration) ; i ++)
	// {

	// 	printk("%d %d %d\n",fifo_data[i].x,fifo_data[i].y,fifo_data[i].z);
	// }
	// printk("********\n");

	struct motion_event * e;
	list_for_each_entry(e,&event_list,list){
		if (e->eid == 0)
			break;
	}

	kfree(fifo_data);

	printk("Hello from parent\n");

	e->triggered = 1;
	wake_up(&e->wait_queue);
}
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

/**
 * sys_accevt_create - Create an event based on motion.
 */


// int accevt_create(struct acc_motion __user *acceleration);
SYSCALL_DEFINE1(accevt_create, struct acc_motion __user *, acceleration)
{
	/* find an available event id */
	int eid;
	struct motion_event *e;
	read_lock(&event_list_lock);
	for (eid = 0; eid < INT_MAX; ++eid) {
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
	read_unlock(&event_list_lock);

	/* construct the struct event*/
	e = kmalloc(sizeof(struct motion_event), GFP_KERNEL);
	e->eid = eid;
	e->triggered = 0;
	e->destroyed = 0;
	atomic_set(&e->ref_count, 0);
	init_waitqueue_head(&e->wait_queue);
	e->baseline = kmalloc(sizeof(struct acc_motion), GFP_KERNEL);
	INIT_LIST_HEAD(&e->list);
	rwlock_init(&e->rwlock);
	if (copy_from_user(e->baseline, acceleration, sizeof(struct acc_motion)))
		return -EFAULT;

	/* add the node to the linked list */
	write_lock(&event_list_lock);
	list_add(&e->list, &event_list);
	write_unlock(&event_list_lock);
	return eid;
}

struct motion_event * find_event(int event_id){
	struct motion_event * e;
	list_for_each_entry(e,&event_list,list){
		if (e->eid == event_id)
			return e;
	}
	return NULL;
}

/* Block a process on an event.
 * It takes the event_id as parameter. The event_id requires verification.
 * Return 0 on success and the appropriate error on failure.
 * system call number 251
 */

SYSCALL_DEFINE1(accevt_wait, int , event_id){

	read_lock(&event_list_lock);
	struct motion_event * wait_event = find_event(event_id);
	read_unlock(&event_list_lock);

	if (wait_event == NULL)
	{
		return -EINVAL;
	}

	DEFINE_WAIT(wait);

	write_lock(&wait_event->rwlock);
	add_wait_queue(&wait_event->wait_queue,&wait);
	atomic_inc(&wait_event->ref_count);
	write_unlock(&wait_event->rwlock);

	printk("Hello from child to start\n");

	while(1){

		read_lock(&wait_event->rwlock);
		if (wait_event->triggered){
			read_unlock(&wait_event->rwlock);
			break;
		}
		read_unlock(&wait_event->rwlock);

		prepare_to_wait(&wait_event->wait_queue,&wait,TASK_INTERRUPTIBLE);
		schedule();
	}

	printk("Hello from child end\n");

	write_lock(&event_list_lock);
	write_lock(&wait_event->rwlock);
	finish_wait(&wait_event->wait_queue,&wait);

	atomic_dec(&wait_event->ref_count);
	int ref_times = atomic_read(&wait_event->ref_count);


	if (wait_event->destroyed && ref_times == 0){
		list_del(&wait_event->list);
		kfree(wait_event->baseline);
		kfree(wait_event);

		write_unlock(&wait_event->rwlock);
		write_unlock(&event_list_lock);

		return -EINVAL;
	}
	else{
		write_unlock(&wait_event->rwlock);
		write_unlock(&event_list_lock);

		printk("Exit success!\n");

		return 0;
	}

	return 0;
}