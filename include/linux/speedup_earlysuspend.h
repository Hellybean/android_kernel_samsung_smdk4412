/* include/linux/speedup_earlysuspend.h
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_SPEEDUP_EARLYSUSPEND_H
#define _LINUX_SPEEDUP_EARLYSUSPEND_H


#ifdef CONFIG_VENDOR_EDIT
struct early_suspend {
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct list_head link;
	int level;
	void (*suspend)(struct early_suspend *h);
	void (*resume)(struct early_suspend *h);
 	int need_speedup;
#endif /* CONFIG_VENDOR_EDIT */
};
#endif

#ifdef CONFIG_VENDOR_EDIT
//Lycan.Wang@Prd.BasicDrv, 2013-08-31 Add for speedup wakeup
#define SPEEDUP_SUSPEND 0
#define SPEEDUP_RESUME 1
int resume_speedup(struct early_suspend *pos);
int suspend_speedup(struct early_suspend *pos);
int speedup_earlysuspend_init(void);
int speedup_earlysuspend_exit(void);
void request_speedup_late_resume(void);
void need_speedup(void);
int is_in_speedup(void);

extern struct list_head *speedup_early_suspend_handlers;
#endif /* CONFIG_VENDOR_EDIT */
#endif

