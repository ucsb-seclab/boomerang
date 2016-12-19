/*
 * Copyright (c) 2014-2015, Linaro Ltd and Contributors. All rights reserved.
 * Copyright (c) 2014-2015, Hisilicon Ltd and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __HIKEY_PRIVATE_H__
#define __HIKEY_PRIVATE_H__

#include <bl_common.h>

/*******************************************************************************
 * This structure represents the superset of information that is passed to
 * BL3-1 e.g. while passing control to it from BL2 which is bl31_params
 * and other platform specific params
 ******************************************************************************/
typedef struct bl2_to_bl31_params_mem {
	struct bl31_params bl31_params;
	struct image_info bl31_image_info;
	struct image_info bl32_image_info;
	struct image_info bl33_image_info;
	struct entry_point_info bl33_ep_info;
	struct entry_point_info bl32_ep_info;
	struct entry_point_info bl31_ep_info;
} bl2_to_bl31_params_mem_t;

#define RANDOM_MAX		0x7fffffffffffffff
#define RANDOM_MAGIC		0x9a4dbeaf

struct random_serial_num {
	uint64_t	magic;
	uint64_t	data;
	char		serialno[32];
};

/*******************************************************************************
 * Function and variable prototypes
 ******************************************************************************/
void configure_mmu_el1(unsigned long total_base,
		       unsigned long total_size,
		       unsigned long ro_start,
		       unsigned long ro_limit,
		       unsigned long coh_start,
		       unsigned long coh_limit);
void configure_mmu_el3(unsigned long total_base,
		       unsigned long total_size,
		       unsigned long ro_start,
		       unsigned long ro_limit,
		       unsigned long coh_start,
		       unsigned long coh_limit);
extern int flush_loader_image(void);
extern int flush_user_images(char *cmdbuf, unsigned long addr,
			     unsigned long length);
extern int flush_random_serialno(unsigned long addr, unsigned long length);
extern void generate_serialno(struct random_serial_num *random);
extern int assign_serialno(char *cmdbuf, struct random_serial_num *random);
extern char *load_serialno(void);
extern void hi6220_pll_init(void);
extern void io_setup(void);
extern int plat_get_image_source(const char *image_name,
				 uintptr_t *dev_handle,
				 uintptr_t *image_spec);
extern void plat_gic_init(void);
extern void usb_download(void);

void plat_security_setup(void);

#endif /* __HIKEY_PRIVATE_H__ */
