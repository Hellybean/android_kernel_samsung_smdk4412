/*************************************************************
 * 
 * Copyright (C), 2008-2012, OPPO Mobile Comm Corp., Ltd
 * 
 * Date Created: 2013-08-31 16:25
 * 
 * Author: Lycan.Wang@Prd.BasicDrv
 * 
 * Description : Speedup late_resume
 * 
 * ------------------ Revision History: ---------------------
 *      <author>        <date>          <desc>
 *
 *************************************************************/ 


#include <linux/earlysuspend.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/rtc.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>

#include "power.h"

#define _TAG "Ker_SULR"
#define su_log(fmt, ...)	\
    pr_emerg("[%s][%s]" fmt, _TAG, __func__, ##__VA_ARGS__);


static void speedup_early_suspend(struct work_struct *work);
static void speedup_late_resume(struct work_struct *work);
static DECLARE_WORK(speedup_es_work, speedup_early_suspend);
static DECLARE_WORK(speedup_lr_work, speedup_late_resume);
struct workqueue_struct *speedup_work_queue;

static DEFINE_MUTEX(speedup_lock);
static struct wake_lock speedup_wakelock;
static struct timer_list suspend_timer;

static DEFINE_SPINLOCK(state_lock);
static int speedup_status;
static int late_resume_status;

//only speedup in some situations
static int need_speedup_flag = 0;
//TODO workaround method for the DSI error 
static int in_speedup = 0;

struct list_head *speedup_early_suspend_handlers = NULL;

void need_speedup(void)
{
    need_speedup_flag = 1;
}

int is_in_speedup(void)
{
    return in_speedup;
}
EXPORT_SYMBOL(is_in_speedup);

int suspend_speedup(struct early_suspend *pos) 
{
    unsigned long irqflags;

    su_log(" lycan test %d\n", __LINE__);
    mutex_lock(&speedup_lock);

    spin_lock_irqsave(&state_lock, irqflags);
    late_resume_status = SPEEDUP_SUSPEND;
    speedup_status = SPEEDUP_SUSPEND;
    spin_unlock_irqrestore(&state_lock, irqflags);

    del_timer(&suspend_timer);

    mutex_unlock(&speedup_lock);
}

int resume_speedup(struct early_suspend *pos)
{
    unsigned long irqflags;
    int locked = mutex_trylock(&speedup_lock);
    int need_resume = 0;

    su_log(" lycan test %d\n", __LINE__);
    del_timer(&suspend_timer);

    spin_lock_irqsave(&state_lock, irqflags);
    late_resume_status = SPEEDUP_RESUME;
    if (speedup_status == SPEEDUP_SUSPEND) 
        need_resume = 1;
    spin_unlock_irqrestore(&state_lock, irqflags);

    if (need_resume) {
        su_log(" lycan test %d\n", __LINE__);
        pos->resume(pos);
    }

    if (locked)
        mutex_unlock(&speedup_lock);
}

void request_speedup_late_resume()
{
    su_log(" lycan test %d\n", __LINE__);
    if (need_speedup_flag) {
        if (queue_work(speedup_work_queue, &speedup_lr_work)) {
            //TODO 1500?
            wake_lock_timeout(&speedup_wakelock, 1500);
            su_log(" lycan test %d\n", __LINE__);
        }
        su_log(" lycan test %d\n", __LINE__);
    }
    need_speedup_flag = 0;
}


static void suspend_timer_fn(unsigned long data)
{
    su_log(" lycan test %d\n", __LINE__);
    queue_work(speedup_work_queue, &speedup_es_work);
} 

static void speedup_early_suspend(struct work_struct *work)
{
    struct early_suspend *pos;
    unsigned long irqflags;
    int need_early_suspend = 0;

    su_log(" lycan test %d\n", __LINE__);
    wake_lock(&speedup_wakelock);
    mutex_lock(&speedup_lock);

    spin_lock_irqsave(&state_lock, irqflags);
    if ((speedup_status == SPEEDUP_RESUME) && (late_resume_status == SPEEDUP_SUSPEND)) {
        speedup_status = SPEEDUP_SUSPEND;
        need_early_suspend = 1;
    }
    spin_unlock_irqrestore(&state_lock, irqflags);

    if (need_early_suspend) {
        su_log(" lycan test start %d\n", __LINE__);
        //ES list
        in_speedup = 1;
        list_for_each_entry(pos, speedup_early_suspend_handlers, link) {
            if (pos->suspend != NULL) {
                if (pos->need_speedup) {
                    su_log("ES handlers [%pf], level: %d\n", pos->suspend, pos->level);
                    pos->suspend(pos);
                }
            }
        }
        in_speedup = 0;
    }

    mutex_unlock(&speedup_lock);
    wake_unlock(&speedup_wakelock);
    su_log("%d\n", __LINE__);
}

static void speedup_late_resume(struct work_struct *work)
{
    unsigned long expires;  
    struct early_suspend *pos;
    unsigned long irqflags;
    int need_late_resume = 0;

    su_log("%d\n", __LINE__);
    mutex_lock(&speedup_lock);

    spin_lock_irqsave(&state_lock, irqflags);
    if ((speedup_status & late_resume_status) == SPEEDUP_SUSPEND) {
        speedup_status = SPEEDUP_RESUME;
        need_late_resume = 1;
    }
    spin_unlock_irqrestore(&state_lock, irqflags);

    if (need_late_resume) {
        setup_timer(&suspend_timer, suspend_timer_fn, 0 );
        expires = jiffies + msecs_to_jiffies(1000);
        mod_timer(&suspend_timer, expires);

        //LR list
        list_for_each_entry_reverse(pos, speedup_early_suspend_handlers, link) {
            if (pos->resume != NULL) {
                if (pos->need_speedup) {
                    //lycan test 
                    long long time = ktime_to_ms(ktime_get());
                    pos->resume(pos);
                    su_log("**** LR handlers [%pf], level: %d cost %lldms\n", pos->resume, pos->level, ktime_to_ms(ktime_get()) - time);
                }
            }
        }
    }
    
    mutex_unlock(&speedup_lock);
    su_log("%d\n", __LINE__);
}

int speedup_earlysuspend_init(void)
{
    su_log("%d\n", __LINE__);
    speedup_work_queue = create_singlethread_workqueue("speedup_work");
    wake_lock_init(&speedup_wakelock, WAKE_LOCK_SUSPEND, "Speedup LR Wakelock");
}

int speedup_earlysuspend_exit(void)
{
    destroy_workqueue(speedup_work_queue);
}
