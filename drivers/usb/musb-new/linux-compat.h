#ifndef __LINUX_COMPAT_H__
#define __LINUX_COMPAT_H__

#include <malloc.h>

#define __init
#define __devinit
#define __devinitdata
#define __devinitconst
#define __iomem
#define __deprecated

#define __releases(lock)
#define __acquires(lock)

typedef enum { false = 0, true = 1 } bool;

struct unused {};
typedef struct unused unused_t;

typedef int irqreturn_t;
typedef unused_t spinlock_t;

struct work_struct {};

struct timer_list {};
struct notifier_block {};

#define noop do {} while (0)
#define success 0

#define spin_lock_init(lock) noop
#define spin_lock(lock) noop
#define spin_unlock(lock) noop
#define spin_lock_irqsave(lock, flags) noop
#define spin_unlock_irqrestore(lock, flags) noop

#define setup_timer(timer, func, data) noop
#define schedule_work(work) noop
#define INIT_WORK(work, fun) noop

#define cpu_relax() noop

#define pr_debug(fmt, args...) debug(fmt, ##args)
#define dev_dbg(dev, fmt, args...)		\
	debug(fmt, ##args)
#define dev_vdbg(dev, fmt, args...)		\
	debug(fmt, ##args)
#define dev_info(dev, fmt, args...)		\
	printf(fmt, ##args)
#define dev_err(dev, fmt, args...)		\
	printf(fmt, ##args)
#define printk printf

#define KERN_DEBUG
#define KERN_NOTICE
#define KERN_ERR

#define min_t(type, x, y) ({                    \
        type __min1 = (x);                      \
        type __min2 = (y);                      \
        __min1 < __min2 ? __min1: __min2; })

#define kzalloc(size, gfp_t) calloc(size, 1)
#define kfree(ptr) free(ptr)

#define pm_runtime_get_sync(dev) noop
#define pm_runtime_put(dev) noop
#define pm_runtime_put_sync(dev) noop
#define pm_runtime_use_autosuspend(dev) noop
#define pm_runtime_set_autosuspend_delay(dev, delay) noop
#define pm_runtime_enable(dev) noop

#define MODULE_DESCRIPTION(desc)
#define MODULE_AUTHOR(author)
#define MODULE_LICENSE(license)
#define MODULE_ALIAS(alias)
#define module_param(name, type, perm)
#define MODULE_PARM_DESC(name, desc)
#define EXPORT_SYMBOL_GPL(name)

#define writesl(a, d, s) __raw_writesl((unsigned long)a, d, s)
#define readsl(a, d, s) __raw_readsl((unsigned long)a, d, s)
#define writesw(a, d, s) __raw_writesw((unsigned long)a, d, s)
#define readsw(a, d, s) __raw_readsw((unsigned long)a, d, s)
#define writesb(a, d, s) __raw_writesb((unsigned long)a, d, s)
#define readsb(a, d, s) __raw_readsb((unsigned long)a, d, s)

#define IRQ_NONE 0
#define IRQ_HANDLED 0

#define dev_set_drvdata(dev, data) noop

#define disable_irq_wake(irq) noop
#define enable_irq_wake(irq) -EINVAL
#define free_irq(irq, data) noop
#define request_irq(nr, f, flags, nm, data) success

#define device_init_wakeup(dev, a) noop

#define platform_data device_data

#define WARN_ON(cond)							\
	do {								\
		if (cond)						\
			printf("WARNING: something weird happened\n");	\
	} while (0)

/* generally not safe but we have only one use case and it's safe */
#define snprintf(s, n, fmt, args...)	sprintf(s, fmt, ##args)

#endif /* __LINUX_COMPAT_H__ */
