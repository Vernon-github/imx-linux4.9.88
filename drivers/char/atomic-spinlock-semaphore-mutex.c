#define DEBUG

#include <linux/module.h>
#include <linux/semaphore.h>

// atomic function
atomic_t atomic = ATOMIC_INIT(0);

// spinlock function
DEFINE_SPINLOCK(spinlock);

// semaphore function
struct semaphore sem;

// mutex function
DEFINE_MUTEX(mutexlock);

static int __init competition_init(void)
{
    unsigned long flags;

    pr_debug("%s: \n", __func__);

    // atomic function
    atomic_set(&atomic, 20);
    atomic_inc(&atomic);
    pr_debug("%s: test == %d\n", __func__, atomic_read(&atomic));

    // spinlock function
    spin_lock_irqsave(&spinlock, flags);
    pr_debug("%s: spinlock function\n", __func__); // can not sleep
    spin_unlock_irqrestore(&spinlock, flags);

    // semaphore function
    sema_init(&sem, 10);
    down(&sem);
    pr_debug("%s: semaphore function\n", __func__); // can sleep
    up(&sem);

    // mutex function
    mutex_lock(&mutexlock);
    pr_debug("%s: mutexlock function\n", __func__); // can sleep
    mutex_unlock(&mutexlock);

	return 0;
}

static void __exit competition_exit(void)
{
    pr_debug("%s: \n", __func__);
}

module_init(competition_init);
module_exit(competition_exit);

MODULE_AUTHOR("lincheng Yang <2669889282@qq.com>");
MODULE_DESCRIPTION("competition mode");
MODULE_LICENSE("GPL v2");
