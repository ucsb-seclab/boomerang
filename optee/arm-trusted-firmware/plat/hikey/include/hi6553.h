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

#ifndef __HI6553_H__
#define __HI6553_H__

#define DISABLE6_XO_CLK				0x036

#define DISABLE6_XO_CLK_BB			(1 << 0)
#define DISABLE6_XO_CLK_CONN			(1 << 1)
#define DISABLE6_XO_CLK_NFC			(1 << 2)
#define DISABLE6_XO_CLK_RF1			(1 << 3)
#define DISABLE6_XO_CLK_RF2			(1 << 4)

#define VERSION_REG				0x000
#define ENABLE2_LDO1_8				0x029
#define DISABLE2_LDO1_8				0x02a
#define ONOFF_STATUS2_LDO1_8			0x02b
#define ENABLE3_LDO9_16				0x02c
#define DISABLE3_LDO9_16			0x02d
#define ONOFF_STATUS3_LDO9_16			0x02e
#define ENABLE4_LDO17_22			0x02f
#define DISABLE4_LDO17_22			0x030
#define ONOFF_STATUS4_LDO17_22			0x031
#define PERI_EN_MARK				0x040
#define BUCK2_REG1				0x04a
#define BUCK2_REG5				0x04e
#define BUCK2_REG6				0x04f
#define BUCK3_REG3				0x054
#define BUCK3_REG5				0x056
#define BUCK3_REG6				0x057
#define BUCK4_REG2				0x05b
#define BUCK4_REG5				0x05e
#define BUCK4_REG6				0x05f
#define CLK_TOP0				0x063
#define CLK_TOP3				0x066
#define CLK_TOP4				0x067
#define VSET_BUCK2_ADJ				0x06d
#define VSET_BUCK3_ADJ				0x06e
#define LDO7_REG_ADJ				0x078
#define LDO10_REG_ADJ				0x07b
#define LDO15_REG_ADJ				0x080
#define LDO19_REG_ADJ				0x084
#define LDO20_REG_ADJ				0x085
#define LDO22_REG_ADJ				0x087
#define DR_LED_CTRL				0x098
#define DR_OUT_CTRL				0x099
#define DR3_ISET				0x09a
#define DR3_START_DEL				0x09b
#define DR4_ISET				0x09c
#define DR4_START_DEL				0x09d
#define DR345_TIM_CONF0				0x0a0
#define NP_REG_ADJ1				0x0be
#define NP_REG_CHG				0x0c0
#define BUCK01_CTRL2				0x0d9
#define BUCK0_CTRL1				0x0dd
#define BUCK0_CTRL5				0x0e1
#define BUCK0_CTRL7				0x0e3
#define BUCK1_CTRL1				0x0e8
#define BUCK1_CTRL5				0x0ec
#define BUCK1_CTRL7				0x0ef
#define CLK19M2_600_586_EN			0x0fe

#define LED_START_DELAY_TIME			0x00
#define LED_ELEC_VALUE				0x07
#define LED_LIGHT_TIME				0xf0
#define LED_GREEN_ENABLE			(1 << 1)
#define LED_OUT_CTRL				0x00

#define PMU_HI6552_V300				0x30
#define PMU_HI6552_V310				0x31

extern unsigned char hi6553_read_8(unsigned int offset);
extern void hi6553_write_8(unsigned int offset, unsigned int value);

#endif	/* __HI6553_H__ */
