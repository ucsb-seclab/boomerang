/*
 * Copyright (c) 2015, Linaro Ltd and Contributors. All rights reserved.
 * Copyright (c) 2015, Hisilicon Ltd and Contributors. All rights reserved.
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

#include <debug.h>
#include <stdint.h>
#include <strings.h>
#include <platform_def.h>

#define PORTNUM_MAX		5

#define MDDRC_SECURITY_BASE	0xF7121000

struct int_en_reg {
	unsigned in_en:1;
	unsigned reserved:31;
};

struct rgn_map_reg {
	unsigned rgn_base_addr:24;
	unsigned rgn_size:6;
	unsigned reserved:1;
	unsigned rgn_en:1;
};

struct rgn_attr_reg {
	unsigned sp:4;
	unsigned security_inv:1;
	unsigned reserved_0:3;
	unsigned mid_en:1;
	unsigned mid_inv:1;
	unsigned reserved_1:6;
	unsigned rgn_en:1;
	unsigned subrgn_disable:16;
};

static volatile struct int_en_reg *get_int_en_reg(uint32_t base)
{
	uint64_t addr = base + 0x20;
	return (struct int_en_reg *)addr;
}

static volatile struct rgn_map_reg *get_rgn_map_reg(uint32_t base, int region, int port)
{
	uint64_t addr = base + 0x100 + 0x10 * region + 0x400 * port;
	return (struct rgn_map_reg *)addr;
}

static volatile struct rgn_attr_reg *get_rgn_attr_reg(uint32_t base, int region,
					     int port)
{
	uint64_t addr = base + 0x104 + 0x10 * region + 0x400 * port;
	return (struct rgn_attr_reg *)addr;
}

static int is_power_of_two(uint32_t x)
{
	return ((x != 0) && !(x & (x - 1)));
}

/*
 * Configure secure memory region
 * region_size must be a power of 2 and at least 64KB
 * region_base must be region_size aligned
 */
static void sec_protect(uint32_t region_base, uint32_t region_size)
{
	volatile struct int_en_reg *int_en_reg ;
	volatile struct rgn_map_reg *rgn_map_reg;
	volatile struct rgn_attr_reg *rgn_attr_reg;
	uint32_t i = 0;

	if (!is_power_of_two(region_size) || region_size < 0x10000) {
		ERROR("Secure region size is not a power of 2 >= 64KB\n");
		return;
	}
	if (region_base & (region_size - 1)) {
		ERROR("Secure region address is not aligned to region size\n");
		return;
	}

	INFO("BL2: TrustZone: protecting %u bytes of memory at 0x%x\n", region_size,
	     region_base);

	int_en_reg = get_int_en_reg(MDDRC_SECURITY_BASE);
	int_en_reg->in_en = 0x1;

	for (i = 0; i < PORTNUM_MAX; i++) {
		rgn_map_reg = get_rgn_map_reg(MDDRC_SECURITY_BASE, 1, i);
		rgn_attr_reg = get_rgn_attr_reg(MDDRC_SECURITY_BASE, 1, i);
		rgn_map_reg->rgn_base_addr = region_base >> 16;
		rgn_attr_reg->subrgn_disable = 0x0;
		rgn_attr_reg->sp = (i == 3) ? 0xC : 0x0;
		rgn_map_reg->rgn_size = __builtin_ffs(region_size) - 2;
		rgn_map_reg->rgn_en = 0x1;
	}
}

/*******************************************************************************
 * Initialize the secure environment.
 ******************************************************************************/
void plat_security_setup(void)
{
	sec_protect(DRAM_SEC_BASE, DRAM_SEC_SIZE);
}
