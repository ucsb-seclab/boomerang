/** @file
*
*  Copyright (c) 2014-2015, Linaro Limited. All rights reserved.
*  Copyright (c) 2014-2015, Hisilicon Limited. All rights reserved.
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

#ifndef __HI6220_H__
#define __HI6220_H__

/***********************************************************************************
// Platform Memory Map
************************************************************************************/

// SOC peripherals (UART, I2C, I2S, USB, etc)
#define HI6220_PERIPH_BASE			0xF4000000
#define HI6220_PERIPH_SZ			0x05800000

#define MDDRC_AXI_BASE				0xF7120000
#define AXI_REGION_MAP_OFFSET(x)		(0x100 + (x) * 0x10)

#endif	/* __HI6220_H__ */
