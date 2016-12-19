/** @file
*
*  Copyright (c) 2015, Linaro Ltd. All rights reserved.
*  Copyright (c) 2015, Hisilicon Ltd. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __HI6220_REGS_PERI_H__
#define __HI6220_REGS_PERI_H__

#define HI6220_PERI_BASE                0xf7030000

#define SC_PERIPH_RSTEN3                (HI6220_PERI_BASE + 0x330)
#define SC_PERIPH_RSTDIS3               (HI6220_PERI_BASE + 0x334)
#define SC_PERIPH_RSTSTAT3              (HI6220_PERI_BASE + 0x338)

/* SC_PERIPH_RSTEN3/RSTDIS3/RSTSTAT3 */
#define PERIPH_RST3_CSSYS               (1 << 0)
#define PERIPH_RST3_I2C0                (1 << 1)
#define PERIPH_RST3_I2C1                (1 << 2)
#define PERIPH_RST3_I2C2                (1 << 3)
#define PERIPH_RST3_I2C3                (1 << 4)
#define PERIPH_RST3_UART1               (1 << 5)
#define PERIPH_RST3_UART2               (1 << 6)
#define PERIPH_RST3_UART3               (1 << 7)
#define PERIPH_RST3_UART4               (1 << 8)
#define PERIPH_RST3_SSP                 (1 << 9)
#define PERIPH_RST3_PWM                 (1 << 10)
#define PERIPH_RST3_BLPWM               (1 << 11)
#define PERIPH_RST3_TSENSOR             (1 << 12)
#define PERIPH_RST3_DAPB                (1 << 18)
#define PERIPH_RST3_HKADC               (1 << 19)
#define PERIPH_RST3_CODEC_SSI           (1 << 20)
#define PERIPH_RST3_PMUSSI1             (1 << 22)

#endif /* __HI6220_REGS_PERI_H__ */
