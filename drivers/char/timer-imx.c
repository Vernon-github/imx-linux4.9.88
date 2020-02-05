#define DEBUG

#include <linux/module.h>
#include <linux/timer.h>

struct timer_dev {
    struct timer_list timer;

};
struct timer_dev timerDev;

void timer_function(unsigned long data)
{
    struct timer_dev *ptimerDev = (struct timer_dev *)data;
    struct timer_list *ptimer = &ptimerDev->timer;

    pr_debug("%s: ", __func__);

    mod_timer(ptimer, jiffies + msecs_to_jiffies(2000));
}

static int __init timer_init(void)
{
    struct timer_list *ptimer = &timerDev.timer;

    pr_debug("%s: ", __func__);

    init_timer(ptimer);
    ptimer->function = timer_function;
    ptimer->data = (unsigned long)&timerDev;
    ptimer->expires = jiffies + msecs_to_jiffies(2000);

    add_timer(ptimer);

	return 0;
}

static void __exit timer_exit(void)
{
    struct timer_list *ptimer = &timerDev.timer;

    pr_debug("%s: ", __func__);

    del_timer(ptimer);
}

module_init(timer_init);
module_exit(timer_exit);

MODULE_AUTHOR("lincheng Yang <2669889282@qq.com>");
MODULE_DESCRIPTION("timer mode");
MODULE_LICENSE("GPL v2");
