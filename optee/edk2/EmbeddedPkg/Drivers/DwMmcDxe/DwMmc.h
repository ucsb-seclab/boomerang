/** @file
*
*  Copyright (c) 2014, Linaro Limited. All rights reserved.
*  Copyright (c) 2014, Hisilicon Limited. All rights reserved.
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


#ifndef __DWMMC_H__
#define __DWMMC_H__

#include <Protocol/EmbeddedGpio.h>

// DW MMC Registers
#define DWMMC_CTRL		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x000)
#define DWMMC_PWREN		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x004)
#define DWMMC_CLKDIV		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x008)
#define DWMMC_CLKSRC		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x00c)
#define DWMMC_CLKENA		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x010)
#define DWMMC_TMOUT		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x014)
#define DWMMC_CTYPE		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x018)
#define DWMMC_BLKSIZ		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x01c)
#define DWMMC_BYTCNT		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x020)
#define DWMMC_INTMASK		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x024)
#define DWMMC_CMDARG		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x028)
#define DWMMC_CMD		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x02c)
#define DWMMC_RESP0		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x030)
#define DWMMC_RESP1		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x034)
#define DWMMC_RESP2		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x038)
#define DWMMC_RESP3		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x03c)
#define DWMMC_RINTSTS		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x044)
#define DWMMC_STATUS		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x048)
#define DWMMC_FIFOTH		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x04c)
#define DWMMC_DEBNCE		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x064)
#define DWMMC_UHSREG		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x074)
#define DWMMC_BMOD		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x080)
#define DWMMC_DBADDR		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x088)
#define DWMMC_IDSTS		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x08c)
#define DWMMC_IDINTEN		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x090)
#define DWMMC_DSCADDR		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x094)
#define DWMMC_BUFADDR		((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0x098)
#define DWMMC_CARDTHRCTL	((UINT32)PcdGet32 (PcdDwMmcBaseAddress) + 0X100)

#define CMD_UPDATE_CLK				0x80202000
#define CMD_START_BIT				(1 << 31)

#define MMC_8BIT_MODE				(1 << 16)

#define BIT_CMD_RESPONSE_EXPECT			(1 << 6)
#define BIT_CMD_LONG_RESPONSE			(1 << 7)
#define BIT_CMD_CHECK_RESPONSE_CRC		(1 << 8)
#define BIT_CMD_DATA_EXPECTED			(1 << 9)
#define BIT_CMD_READ				(0 << 10)
#define BIT_CMD_WRITE				(1 << 10)
#define BIT_CMD_BLOCK_TRANSFER			(0 << 11)
#define BIT_CMD_STREAM_TRANSFER			(1 << 11)
#define BIT_CMD_SEND_AUTO_STOP			(1 << 12)
#define BIT_CMD_WAIT_PRVDATA_COMPLETE		(1 << 13)
#define BIT_CMD_STOP_ABORT_CMD			(1 << 14)
#define BIT_CMD_SEND_INIT			(1 << 15)
#define BIT_CMD_UPDATE_CLOCK_ONLY		(1 << 21)
#define BIT_CMD_READ_CEATA_DEVICE		(1 << 22)
#define BIT_CMD_CCS_EXPECTED			(1 << 23)
#define BIT_CMD_ENABLE_BOOT			(1 << 24)
#define BIT_CMD_EXPECT_BOOT_ACK			(1 << 25)
#define BIT_CMD_DISABLE_BOOT			(1 << 26)
#define BIT_CMD_MANDATORY_BOOT			(0 << 27)
#define BIT_CMD_ALTERNATE_BOOT			(1 << 27)
#define BIT_CMD_VOLT_SWITCH			(1 << 28)
#define BIT_CMD_USE_HOLD_REG			(1 << 29)
#define BIT_CMD_START				(1 << 31)

#define DWMMC_INT_EBE			(1 << 15)	/* End-bit Err */
#define DWMMC_INT_SBE			(1 << 13)	/* Start-bit  Err */
#define DWMMC_INT_HLE			(1 << 12)	/* Hardware-lock Err */
#define DWMMC_INT_FRUN			(1 << 11)	/* FIFO UN/OV RUN */
#define DWMMC_INT_DRT			(1 << 9)	/* Data timeout */
#define DWMMC_INT_RTO			(1 << 8)	/* Response timeout */
#define DWMMC_INT_DCRC			(1 << 7)	/* Data CRC err */
#define DWMMC_INT_RCRC			(1 << 6)	/* Response CRC err */
#define DWMMC_INT_RXDR			(1 << 5)
#define DWMMC_INT_TXDR			(1 << 4)
#define DWMMC_INT_DTO			(1 << 3)	/* Data trans over */
#define DWMMC_INT_CMD_DONE		(1 << 2)
#define DWMMC_INT_RE			(1 << 1)

#define DWMMC_IDMAC_DES0_DIC		(1 << 1)
#define DWMMC_IDMAC_DES0_LD		(1 << 2)
#define DWMMC_IDMAC_DES0_FS		(1 << 3)
#define DWMMC_IDMAC_DES0_CH		(1 << 4)
#define DWMMC_IDMAC_DES0_ER		(1 << 5)
#define DWMMC_IDMAC_DES0_CES		(1 << 30)
#define DWMMC_IDMAC_DES0_OWN		(1 << 31)
#define DWMMC_IDMAC_DES1_BS1(x)		((x) & 0x1fff)
#define DWMMC_IDMAC_DES2_BS2(x)		(((x) & 0x1fff) << 13)
#define DWMMC_IDMAC_SWRESET		(1 << 0)
#define DWMMC_IDMAC_FB			(1 << 1)
#define DWMMC_IDMAC_ENABLE		(1 << 7)

#define EMMC_FIX_RCA				6

/* bits in MMC0_CTRL */
#define DWMMC_CTRL_RESET		(1 << 0)
#define DWMMC_CTRL_FIFO_RESET		(1 << 1)
#define DWMMC_CTRL_DMA_RESET		(1 << 2)
#define DWMMC_CTRL_INT_EN		(1 << 4)
#define DWMMC_CTRL_DMA_EN		(1 << 5)
#define DWMMC_CTRL_IDMAC_EN		(1 << 25)
#define DWMMC_CTRL_RESET_ALL		(DWMMC_CTRL_RESET | DWMMC_CTRL_FIFO_RESET | DWMMC_CTRL_DMA_RESET)

#define DWMMC_STS_DATA_BUSY		(1 << 9)

#define DWMMC_FIFO_TWMARK(x)		(x & 0xfff)
#define DWMMC_FIFO_RWMARK(x)		((x & 0x1ff) << 16)
#define DWMMC_DMA_BURST_SIZE(x)		((x & 0x7) << 28)

#define DWMMC_CARD_RD_THR(x)		((x & 0xfff) << 16)
#define DWMMC_CARD_RD_THR_EN		(1 << 0)

#endif  // __DWMMC_H__
