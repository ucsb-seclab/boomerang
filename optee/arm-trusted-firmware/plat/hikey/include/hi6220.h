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

#ifndef __HI6220_H__
#define __HI6220_H__

#include <hi6220_regs_acpu.h>
#include <hi6220_regs_ao.h>
#include <hi6220_regs_peri.h>
#include <hi6220_regs_pmctrl.h>

#include <hisi_mcu.h>
#include <hisi_sram_map.h>

#define MEDIA_CTRL_BASE				0xf4410000
#define MEDIA_SUBSYS_CTRL2			(MEDIA_CTRL_BASE + 0x508)
#define MEDIA_SUBSYS_NOC_DFS			(MEDIA_CTRL_BASE + 0x510)
#define MEDIA_SUBSYS_CTRL5			(MEDIA_CTRL_BASE + 0x51c)

#define MMC0_BASE				0xf723d000
#define MMC1_BASE				0xf723e000

#define EDMAC_BASE				0xf7370000
#define EDMAC_SEC_CTRL				(EDMAC_BASE + 0x694)
#define EDMAC_AXI_CONF(x)			(EDMAC_BASE + 0x820 + (x << 6))

#define PMUSSI_BASE				0xf8000000

#define TIMER0_BASE				0xf8008000
#define TIMER00_LOAD				(TIMER0_BASE + 0x000)
#define TIMER00_VALUE				(TIMER0_BASE + 0x004)
#define TIMER00_CONTROL				(TIMER0_BASE + 0x008)
#define TIMER00_BGLOAD				(TIMER0_BASE + 0x018)

#define GPIO0_BASE				0xf8011000
#define GPIO1_BASE				0xf8012000
#define GPIO2_BASE				0xf8013000
#define GPIO3_BASE				0xf8014000
#define GPIO4_BASE				0xf7020000
#define GPIO5_BASE				0xf7021000
#define GPIO6_BASE				0xf7022000
#define GPIO7_BASE				0xf7023000
#define GPIO8_BASE				0xf7024000
#define GPIO9_BASE				0xf7025000
#define GPIO10_BASE				0xf7026000
#define GPIO11_BASE				0xf7027000
#define GPIO12_BASE				0xf7028000
#define GPIO13_BASE				0xf7029000
#define GPIO14_BASE				0xf702a000
#define GPIO15_BASE				0xf702b000
#define GPIO16_BASE				0xf702c000
#define GPIO17_BASE				0xf702d000
#define GPIO18_BASE				0xf702e000
#define GPIO19_BASE				0xf702f000

extern void init_acpu_dvfs(void);

#endif	/* __HI6220_H__ */
