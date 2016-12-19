#ifndef PP_CHECK_H
#define PP_CHECK_H

#include <linux/atomic.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/printk.h>
#include <linux/ioctl.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/syscalls.h>
#include <asm/smp_plat.h>

#include "optee_private.h"


int boomerang_prep_mem_buffer(void *buffer, size_t size, bool is_write, struct optee_session *session, void **buff_paaddr, void **buff_paaddr1);
		
void boomerang_release_mem_buffers(struct list_head *buflist);
#endif

