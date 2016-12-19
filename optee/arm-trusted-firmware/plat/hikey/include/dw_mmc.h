/*
 * Copyright (c) 2014, Hisilicon Ltd.
 * Copyright (c) 2014, Linaro Ltd.
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

#ifndef __DW_MMC_H__
#define __DW_MMC_H__

#include <stdint.h>

#define MMC0_CTRL				(MMC0_BASE + 0x000)
#define MMC0_CLKDIV				(MMC0_BASE + 0x008)
#define MMC0_CLKSRC				(MMC0_BASE + 0x00c)
#define MMC0_CLKENA				(MMC0_BASE + 0x010)
#define MMC0_TMOUT				(MMC0_BASE + 0x014)
#define MMC0_CTYPE				(MMC0_BASE + 0x018)
#define MMC0_BLKSIZ				(MMC0_BASE + 0x01c)
#define MMC0_BYTCNT				(MMC0_BASE + 0x020)
#define MMC0_INTMASK				(MMC0_BASE + 0x024)
#define MMC0_CMDARG				(MMC0_BASE + 0x028)
#define MMC0_CMD				(MMC0_BASE + 0x02c)
#define MMC0_RESP0				(MMC0_BASE + 0x030)
#define MMC0_RESP1				(MMC0_BASE + 0x034)
#define MMC0_RESP2				(MMC0_BASE + 0x038)
#define MMC0_RESP3				(MMC0_BASE + 0x03c)
#define MMC0_RINTSTS				(MMC0_BASE + 0x044)
#define MMC0_STATUS				(MMC0_BASE + 0x048)
#define MMC0_FIFOTH				(MMC0_BASE + 0x04c)
#define MMC0_DEBNCE				(MMC0_BASE + 0x064)
#define MMC0_UHSREG				(MMC0_BASE + 0x074)
#define MMC0_BMOD				(MMC0_BASE + 0x080)
#define MMC0_DBADDR				(MMC0_BASE + 0x088)
#define MMC0_IDSTS				(MMC0_BASE + 0x08c)
#define MMC0_IDINTEN				(MMC0_BASE + 0x090)
#define MMC0_DSCADDR				(MMC0_BASE + 0x094)
#define MMC0_BUFADDR				(MMC0_BASE + 0x098)
#define MMC0_CARDTHRCTL				(MMC0_BASE + 0X100)

#define CMD_UPDATE_CLK				0x80202000
#define CMD_START_BIT				(1 << 31)

#define MMC_8BIT_MODE				(1 << 16)

#define MMC_BLOCK_SIZE				512

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

#define MMC_INT_EBE			(1 << 15)	/* End-bit Err */
#define MMC_INT_SBE			(1 << 13)	/* Start-bit  Err */
#define MMC_INT_HLE			(1 << 12)	/* Hardware-lock Err */
#define MMC_INT_FRUN			(1 << 11)	/* FIFO UN/OV RUN */
#define MMC_INT_DRT			(1 << 9)	/* Data timeout */
#define MMC_INT_RTO			(1 << 8)	/* Response timeout */
#define MMC_INT_DCRC			(1 << 7)	/* Data CRC err */
#define MMC_INT_RCRC			(1 << 6)	/* Response CRC err */
#define MMC_INT_RXDR			(1 << 5)
#define MMC_INT_TXDR			(1 << 4)
#define MMC_INT_DTO			(1 << 3)	/* Data trans over */
#define MMC_INT_CMD_DONE		(1 << 2)
#define MMC_INT_RE			(1 << 1)

#define EMMC_FIX_RCA				6

/* bits in MMC0_CTRL */
#define MMC_CTRL_RESET				(1 << 0)
#define MMC_FIFO_RESET				(1 << 1)
#define MMC_DMA_RESET				(1 << 2)
#define MMC_INT_EN				(1 << 4)
#define MMC_DMA_EN				(1 << 25)

#define MMC_STS_DATA_BUSY			(1 << 9)

#define MMC_STATUS_CURRENT_STATE_MASK	(0xf << 9)
#define MMC_STATUS_CURRENT_STATE_SHIFT	9
#define MMC_STATUS_READY_FOR_DATA	(1 << 8)
#define MMC_STATUS_SWITCH_ERROR		(1 << 7)

#define MMC_STATE_IDLE			0
#define MMC_STATE_READY			1
#define MMC_STATE_IDENT			2
#define MMC_STATE_STBY			3
#define MMC_STATE_TRAN			4
#define MMC_STATE_DATA			5
#define MMC_STATE_RCV			6
#define MMC_STATE_PRG			7
#define MMC_STATE_DIS			8
#define MMC_STATE_BTST			9
#define MMC_STATE_SLP			10

#define EXT_CSD_CACHE_CTRL		33
#define EXT_CSD_PARTITION_CONFIG	179

#define PART_CFG_BOOT_PARTITION1_ENABLE	(1 << 3)
#define PART_CFG_PARTITION1_ACCESS	(1 << 0)

#define MMC_IDMAC_ENABLE			(1 << 7)
#define MMC_IDMAC_FB				(1 << 1)
#define MMC_IDMAC_SWRESET			(1 << 0)

#define MMC_FIFO_TWMARK(x)			(x & 0xfff)
#define MMC_FIFO_RWMARK(x)			((x & 0x1ff) << 16)
#define MMC_DMA_BURST_SIZE(x)			((x & 0x7) << 28)

#define MMC_CARD_RD_THR(x)			((x & 0xfff) << 16)
#define MMC_CARD_RD_THR_EN			(1 << 0)

extern int init_mmc(void);
extern int mmc0_read(unsigned long, size_t, unsigned long, uint32_t);
extern int mmc0_write(unsigned long, size_t, unsigned long, uint32_t);

#endif /* __DW_MMC_H */
