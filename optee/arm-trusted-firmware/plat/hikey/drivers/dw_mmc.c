/*
 * Copyright (c) 2014-2015, Linaro Ltd. All rights reserved.
 * Copyright (c) 2014-2015, Hisilicon Ltd. All rights reserved.
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

#include <console.h>
#include <debug.h>
#include <errno.h>
#include <mmio.h>
#include <string.h>
#include <sp804_timer.h>
#include <dw_mmc.h>
#include <partitions.h>
#include <platform_def.h>
#include <hi6220.h>
#include <hi6553.h>

#define MMC_PLL			100000000

#define IDMAC_DES0_DIC		(1 << 1)
#define IDMAC_DES0_LD		(1 << 2)
#define IDMAC_DES0_FS		(1 << 3)
#define IDMAC_DES0_CH		(1 << 4)
#define IDMAC_DES0_ER		(1 << 5)
#define IDMAC_DES0_CES		(1 << 30)
#define IDMAC_DES0_OWN		(1 << 31)

#define IDMAC_DES1_BS1(x)	((x) & 0x1fff)
#define IDMAC_DES2_BS2(x)	(((x) & 0x1fff) << 13)

struct idmac_desc {
	unsigned int		des0;
	unsigned int		des1;
	unsigned int		des2;
	unsigned int		des3;
};

static inline int mmc_state(unsigned int data)
{
	return ((data & MMC_STATUS_CURRENT_STATE_MASK) >>
		MMC_STATUS_CURRENT_STATE_SHIFT);
}

static inline int wait_data_ready(void)
{
	unsigned int data;

	while (1) {
		data = mmio_read_32(MMC0_RINTSTS);
		if (data & (MMC_INT_DCRC | MMC_INT_DRT | MMC_INT_SBE |
		    MMC_INT_EBE)) {
			NOTICE("unwanted interrupts:0x%x\n", data);
			return -EINVAL;
		}
		if (data & MMC_INT_DTO)
			break;
	}
	/* clear interrupts */
	mmio_write_32(MMC0_RINTSTS, ~0);
	return 0;
}

static int update_mmc0_clock(void)
{
	unsigned int data;

	/* CMD_UPDATE_CLK */
	data = BIT_CMD_WAIT_PRVDATA_COMPLETE | BIT_CMD_UPDATE_CLOCK_ONLY |
		BIT_CMD_START;
	mmio_write_32(MMC0_CMD, data);
	while (1) {
		data = mmio_read_32(MMC0_CMD);
		if (!(data & CMD_START_BIT))
			break;
		data = mmio_read_32(MMC0_RINTSTS);
		if (data & MMC_INT_HLE) {
			NOTICE("fail to update mmc clock frequency\n");
			return -EINVAL;
		}
	}
	return 0;
}

static int set_mmc0_clock(int rate)
{
	int ret, divider, found = 0;
	unsigned int data;

	for (divider = 1; divider < 256; divider++) {
		if ((MMC_PLL / (2 * divider)) <= rate) {
			found = 1;
			break;
		}
	}
	if (!found)
		return -EINVAL;

	/* wait until mmc is idle */
	do {
		data = mmio_read_32(MMC0_STATUS);
	} while (data & MMC_STS_DATA_BUSY);

	/* Disable mmc clock first */
	mmio_write_32(MMC0_CLKENA, 0);
	do {
		ret = update_mmc0_clock();
	} while (ret);

	/* enable mmc clock */
	do {
		mmio_write_32(MMC0_CLKENA, 1);
		mmio_write_32(MMC0_CLKSRC, 0);
		mmio_write_32(MMC0_CLKDIV, divider);
		ret = update_mmc0_clock();
	} while (ret);
	return 0;
}

static void set_mmc0_io(void)
{
	mmio_write_32(MMC0_CTYPE, MMC_8BIT_MODE);
	mmio_write_32(MMC0_TMOUT, ~0);	/* maxium timeout value */
	mmio_write_32(MMC0_DEBNCE, 0x00ffffff);
	mmio_write_32(MMC0_BLKSIZ, MMC_BLOCK_SIZE);
	mmio_write_32(MMC0_BYTCNT, 256 * 1024);
}

static int mmc0_send_cmd(unsigned int cmd, unsigned int arg, unsigned int *buf)
{
	unsigned int data, err_mask;

	if (!buf) {
		NOTICE("buf is invalid\n");
		return -EFAULT;
	}

	mmio_write_32(MMC0_CMDARG, arg);

	/* clear interrupts */
	mmio_write_32(MMC0_RINTSTS, ~0);

	switch (cmd) {
	case 0:
		data = BIT_CMD_SEND_INIT;
		break;
	case 1:
		data = BIT_CMD_RESPONSE_EXPECT;
		break;
	case 2:
		data = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_LONG_RESPONSE |
			BIT_CMD_CHECK_RESPONSE_CRC | BIT_CMD_SEND_INIT;
		break;
	case 3:
		data = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
			BIT_CMD_SEND_INIT;
		break;
	case 8:
		data = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
			BIT_CMD_DATA_EXPECTED | BIT_CMD_READ |
			BIT_CMD_WAIT_PRVDATA_COMPLETE;
		break;
	case 9:
		data = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
			BIT_CMD_LONG_RESPONSE;
		break;
	case 12:
		data = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
			BIT_CMD_STOP_ABORT_CMD;
		break;
	case 17:
	case 18:
		data = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
			BIT_CMD_DATA_EXPECTED | BIT_CMD_READ |
			BIT_CMD_WAIT_PRVDATA_COMPLETE;
		break;
	case 24:
	case 25:
		data = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
			BIT_CMD_DATA_EXPECTED | BIT_CMD_WRITE |
			BIT_CMD_WAIT_PRVDATA_COMPLETE;
		break;
	case 30:
		data = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC |
			BIT_CMD_DATA_EXPECTED;
		break;
	case 7:
		if (arg)
			data = BIT_CMD_RESPONSE_EXPECT |
				BIT_CMD_CHECK_RESPONSE_CRC;
		else
			data = 0;
		break;
	default:
		data = BIT_CMD_RESPONSE_EXPECT | BIT_CMD_CHECK_RESPONSE_CRC;
		break;
	}
	data |= (cmd & 0x3f) | BIT_CMD_USE_HOLD_REG | BIT_CMD_START;
	mmio_write_32(MMC0_CMD, data);
	err_mask = MMC_INT_EBE | MMC_INT_HLE | MMC_INT_RTO | MMC_INT_RCRC |
		   MMC_INT_RE;
	do {
		data = mmio_read_32(MMC0_RINTSTS);
		if (data & err_mask) {
			NOTICE("mmc: error status 0x%x\n", data);
			return -EIO;
		}
	} while (!(data & MMC_INT_CMD_DONE));

	buf[0] = mmio_read_32(MMC0_RESP0);
	if ((cmd == 2) || (cmd == 9)) {
		buf[1] = mmio_read_32(MMC0_RESP1);
		buf[2] = mmio_read_32(MMC0_RESP2);
		buf[3] = mmio_read_32(MMC0_RESP3);
	}
	return 0;
}

/* Only print error message if it meets failure? */
static void mmc0_check_tran_mode(void)
{
	unsigned int buf[4];
	int ret;

	mmio_write_32(MMC0_RINTSTS, ~0);

	while (1) {
		ret = mmc0_send_cmd(13, EMMC_FIX_RCA << 16, buf);
		if (ret) {
			NOTICE("failed on command 13\n");
			return;
		}
		if (((buf[0] >> 9) & 0xf) == 4)
			return;
	}
}

static int mmc0_update_ext_csd(int index, int value)
{
	unsigned int arg, data, buf[4];
	int ret;

	arg = 3 << 24;
	arg |= (index & 0xff) << 16;
	arg |= (value & 0xff) << 8;
	arg |= 1;
	memset(buf, 0, 4 * sizeof(buf[0]));

	ret = mmc0_send_cmd(6, arg, buf);
	if (ret) {
		NOTICE("failed to send command 6\n");
		return ret;
	}

	/* wait busy de-assert */
	while (1) {
		data = mmio_read_32(MMC0_STATUS);
		if (!(data & MMC_STS_DATA_BUSY))
			break;
	}

	do {
		ret = mmc0_send_cmd(13, EMMC_FIX_RCA << 16, buf);
		if (ret) {
			NOTICE("failed to send command 13\n");
			return ret;
		}

		if (buf[0] & MMC_STATUS_SWITCH_ERROR) {
			NOTICE("maybe switch mmc mode error\n");
			return -1;
		}
	} while (mmc_state(buf[0]) == MMC_STATE_PRG);

	return 0;
}

#define EXTCSD_BUS_WIDTH		183

static int mmc0_set_clock_and_width(int rate, int width)
{
	int ret;

	switch (width) {
	case 0:
		mmio_write_32(MMC0_CTYPE, 0);
		ret = mmc0_update_ext_csd(EXTCSD_BUS_WIDTH, 0);
		break;
	case 8:
		mmio_write_32(MMC0_CTYPE, 1 << 16);
		ret = mmc0_update_ext_csd(EXTCSD_BUS_WIDTH, 2 + 4);
		mmio_write_32(MMC0_UHSREG, 1 << 16);
		break;
	default:
		NOTICE("wrong bus width:%d\n", width);
		return -EINVAL;
	}
	if (ret) {
		NOTICE("return failure on %s, %d\n", __func__, __LINE__);
		return ret;
	}

	set_mmc0_clock(rate);
	return 0;
}

static int manu_id;

#define EXTCSD_HS_TIMING		185

#ifdef EMMC_READ_EXT_CSD
static int mmc0_read_ext_csd(unsigned int dst_start);
#endif
static int enum_mmc0_card(void)
{
	unsigned int buf[4], cid[4];
	int ret = 0, i, version;

	/* CMD0: reset to IDLE */
	ret = mmc0_send_cmd(0, 0, buf);
	if (ret) {
		NOTICE("failed to send IDLE command\n");
		return ret;
	}

	while (1) {
		udelay(100);
		/* CMD1: READY */
		ret = mmc0_send_cmd(1, 0x40ff8000, buf);
		if (ret) {
			NOTICE("failed to send READY command\n");
			return ret;
		}
		if (buf[0] & 0x80000000)
			break;
	}

	/* CMD2: IDENT */
	ret = mmc0_send_cmd(2, 0, buf);
	if (ret) {
		NOTICE("failed to send IDENT command\n");
		return ret;
	}
	VERBOSE("manuid:");
	for (i = 0; i < 4; i++) {
		cid[i] = buf[i];
		VERBOSE(" 0x%x", cid[i]);
	}
	VERBOSE("\n");

	/* CMD3: STBY */
	ret = mmc0_send_cmd(3, EMMC_FIX_RCA << 16, buf);
	if (ret) {
		NOTICE("failed to send STBY command\n");
		return ret;
	}

	/* CMD9: get CSD */
	ret = mmc0_send_cmd(9, EMMC_FIX_RCA << 16, buf);
	if (ret) {
		NOTICE("failed to get CSD\n");
		return ret;
	}
	VERBOSE("CSD: %x-%x-%x-%x\n", buf[0], buf[1], buf[2], buf[3]);
	version = (buf[3] >> 26) & 0xf;
	switch (version) {
	case 0:	/* MMC v1.0-v1.2 */
	case 1:	/* MMC v1.4 */
		manu_id = (cid[3] >> 8) & 0xffffff;
		break;
	case 2:	/* MMC v2.0-v2.2 */
	case 3:	/* MMC v3.1-v3.3 */
	case 4:	/* MMC v4 */
		manu_id = (cid[3] >> 24) & 0xff;
		break;
	default:
		WARN("wrong mmc version (%d) is specified.\n", version);
		break;
	}

	VERBOSE("mmc version:%d\n", version);
	/* CMD7: TRAN */
	ret = mmc0_send_cmd(7, EMMC_FIX_RCA << 16, buf);
	if (ret) {
		NOTICE("failed to send TRAN command\n");
		return ret;
	}
	mmc0_check_tran_mode();

	mmc0_set_clock_and_width(400000, 0);
#ifdef EMMC_READ_EXT_CSD
	mmc0_read_ext_csd(0x50000);
#endif
	ret = mmc0_update_ext_csd(EXTCSD_HS_TIMING, 1);
	if (ret) {
		NOTICE("alter HS mode fail\n");
	}

	ret = mmc0_set_clock_and_width(50000000, 8);
	return ret;
}

static int enable_mmc0(void)
{
	unsigned int data;

	/* reset mmc0 */
	data = MMC_CTRL_RESET | MMC_FIFO_RESET | MMC_DMA_RESET;
	mmio_write_32(MMC0_CTRL, data);
	/* wait until reset operation finished */
	do {
		data = mmio_read_32(MMC0_CTRL);
	} while (data);

	data = MMC_INT_EN | MMC_DMA_EN;
	mmio_write_32(MMC0_CTRL, data);

	mmio_write_32(MMC0_INTMASK, 0x0);
	mmio_write_32(MMC0_RINTSTS, ~0);
	mmio_write_32(MMC0_IDINTEN, ~0);
	mmio_write_32(MMC0_IDSTS, ~0);

	mmio_write_32(MMC0_BLKSIZ, MMC_BLOCK_SIZE);
	mmio_write_32(MMC0_BMOD, MMC_IDMAC_SWRESET);
	do {
		data = mmio_read_32(MMC0_BMOD);
	} while (data & MMC_IDMAC_SWRESET);

	data |= MMC_IDMAC_ENABLE | MMC_IDMAC_FB;
	mmio_write_32(MMC0_BMOD, data);

	data = MMC_DMA_BURST_SIZE(2) | MMC_FIFO_TWMARK(8) | MMC_FIFO_RWMARK(7);
	mmio_write_32(MMC0_FIFOTH, data);
	data = MMC_CARD_RD_THR(512) | MMC_CARD_RD_THR_EN;
	mmio_write_32(MMC0_CARDTHRCTL, data);

	udelay(100);
	set_mmc0_clock(378000);
	udelay(100);

	set_mmc0_io();
	return 0;
}

#define MMC_BLOCK_SIZE			512
#define MMC_DMA_MAX_BUFFER_SIZE		(512 * 8)

#ifdef EMMC_READ_EXT_CSD
static int mmc0_read_ext_csd(unsigned int dst_start)
{
	unsigned int blk_cnt, bytes, desc_num, buf[4], data;
	struct idmac_desc *desc = NULL;
	int i, ret, last_idx;
	uintptr_t src_addr, dst_addr = dst_start;

	blk_cnt = 1;
	bytes = blk_cnt * MMC_BLOCK_SIZE;
	memset((void *)MMC_DATA_BASE, 0, bytes);

	mmio_write_32(MMC0_BYTCNT, bytes);

	mmio_write_32(MMC0_RINTSTS, ~0);

	desc_num = (bytes + MMC_DMA_MAX_BUFFER_SIZE - 1) /
		   MMC_DMA_MAX_BUFFER_SIZE;

	desc = (struct idmac_desc *)MMC_DESC_BASE;

	for (i = 0; i < desc_num; i++) {
		(desc + i)->des0 = IDMAC_DES0_OWN | IDMAC_DES0_CH |
				   IDMAC_DES0_DIC;
		(desc + i)->des1 = IDMAC_DES1_BS1(MMC_DMA_MAX_BUFFER_SIZE);
		/* buffer address */
		(desc + i)->des2 = MMC_DATA_BASE + MMC_DMA_MAX_BUFFER_SIZE * i;
		/* next descriptor address */
		(desc + i)->des3 = MMC_DESC_BASE +
				   (sizeof(struct idmac_desc) * (i + 1));
	}
	/* first descriptor */
	desc->des0 |= IDMAC_DES0_FS;
	/* last descriptor */
	last_idx = desc_num - 1;
	(desc + last_idx)->des0 |= IDMAC_DES0_LD;
	(desc + last_idx)->des0 &= ~(IDMAC_DES0_DIC | IDMAC_DES0_CH);
	(desc + last_idx)->des1 = IDMAC_DES1_BS1(bytes - (last_idx *
				  MMC_DMA_MAX_BUFFER_SIZE));
	/* set next descriptor address as 0 */
	(desc + last_idx)->des3 = 0;

	mmio_write_32(MMC0_DBADDR, MMC_DESC_BASE);

	/* read extended CSD */
	ret = mmc0_send_cmd(8, EMMC_FIX_RCA << 16, buf);
	if (ret) {
		NOTICE("failed to send CMD8\n");
		mmio_write_32(MMC0_RINTSTS, ~0);
		return -EFAULT;
	}

	ret = wait_data_ready();
	if (ret)
		return ret;

	if (blk_cnt > 1) {
		ret = mmc0_send_cmd(12, EMMC_FIX_RCA << 16, buf);
		if (ret) {
			NOTICE("failed to send Stop Transmission command\n");
			return ret;
		}
		mmio_write_32(MMC0_RINTSTS, ~0);
	}
	src_addr = MMC_DATA_BASE;
	memcpy((void *)dst_addr, (void *)src_addr, MMC_BLOCK_SIZE);

	return 0;
}
#endif

int mmc0_read(unsigned long src_start, size_t src_size,
		unsigned long dst_start, uint32_t boot_partition)
{
	unsigned int src_blk_start = src_start / MMC_BLOCK_SIZE;
	unsigned int src_blk_cnt, offset, bytes, desc_num, buf[4];
	struct idmac_desc *desc = NULL;
	int i, ret, last_idx;
	uintptr_t src_addr, dst_addr = dst_start;

	if (boot_partition) {
		/* switch to boot partition 1 */
		ret = mmc0_update_ext_csd(EXT_CSD_PARTITION_CONFIG,
					  PART_CFG_BOOT_PARTITION1_ENABLE |
					  PART_CFG_PARTITION1_ACCESS);
		if (ret) {
			NOTICE("fail to switch eMMC boot partition\n");
			return ret;
		}
	}
	offset = src_start % MMC_BLOCK_SIZE;
	src_blk_cnt = (src_size + offset + MMC_BLOCK_SIZE - 1) / MMC_BLOCK_SIZE;
	bytes = src_blk_cnt * MMC_BLOCK_SIZE;

	mmio_write_32(MMC0_BYTCNT, bytes);

	mmio_write_32(MMC0_RINTSTS, ~0);

	desc_num = (bytes + MMC_DMA_MAX_BUFFER_SIZE - 1) /
		   MMC_DMA_MAX_BUFFER_SIZE;

	desc = (struct idmac_desc *)MMC_DESC_BASE;

	for (i = 0; i < desc_num; i++) {
		(desc + i)->des0 = IDMAC_DES0_OWN | IDMAC_DES0_CH |
				   IDMAC_DES0_DIC;
		(desc + i)->des1 = IDMAC_DES1_BS1(MMC_DMA_MAX_BUFFER_SIZE);
		/* buffer address */
		(desc + i)->des2 = MMC_DATA_BASE + MMC_DMA_MAX_BUFFER_SIZE * i;
		/* next descriptor address */
		(desc + i)->des3 = MMC_DESC_BASE +
				   (sizeof(struct idmac_desc) * (i + 1));
	}
	/* first descriptor */
	desc->des0 |= IDMAC_DES0_FS;
	/* last descriptor */
	last_idx = desc_num - 1;
	(desc + last_idx)->des0 |= IDMAC_DES0_LD;
	(desc + last_idx)->des0 &= ~(IDMAC_DES0_DIC | IDMAC_DES0_CH);
	(desc + last_idx)->des1 = IDMAC_DES1_BS1(bytes - (last_idx *
				  MMC_DMA_MAX_BUFFER_SIZE));
	/* set next descriptor address as 0 */
	(desc + last_idx)->des3 = 0;

	mmio_write_32(MMC0_DBADDR, MMC_DESC_BASE);

	ret = mmc0_send_cmd(23, src_blk_cnt & 0xffff, buf);
	if (ret) {
		NOTICE("failed to send CMD23\n");
		mmio_write_32(MMC0_RINTSTS, ~0);
		return -EFAULT;
	}
	/* multiple read */
	ret = mmc0_send_cmd(18, src_blk_start, buf);
	if (ret) {
		NOTICE("failed to send CMD18\n");
		mmio_write_32(MMC0_RINTSTS, ~0);
		return -EFAULT;
	}

	ret = wait_data_ready();
	if (ret)
		return ret;

	src_addr = MMC_DATA_BASE + offset;
	memcpy((void *)dst_addr, (void *)src_addr, src_size);

	if (boot_partition) {
		/* switch back to normal partition */
		ret = mmc0_update_ext_csd(EXT_CSD_PARTITION_CONFIG,
					  PART_CFG_BOOT_PARTITION1_ENABLE);
		if (ret)
			NOTICE("fail to switch eMMC normal partition\n");
	}
	return ret;
}

static int write_multi_blocks(unsigned int lba, unsigned int count,
			      unsigned int buffer, unsigned int boot_partition)
{
	unsigned int bytes, resp_buf[4], desc_num;
	struct idmac_desc *desc = NULL;
	int ret, last_idx, i;

	if (buffer % 4) {
		NOTICE("invalid buffer address:0x%x\n", buffer);
		return -EINVAL;
	}
	if (boot_partition) {
		/* switch to boot partition 1 */
		ret = mmc0_update_ext_csd(EXT_CSD_PARTITION_CONFIG,
					  PART_CFG_BOOT_PARTITION1_ENABLE |
					  PART_CFG_PARTITION1_ACCESS);
		if (ret) {
			NOTICE("fail to switch eMMC boot partition\n");
			return ret;
		}
	}
	bytes = MMC_BLOCK_SIZE * count;

	mmio_write_32(MMC0_BYTCNT, bytes);
	mmio_write_32(MMC0_RINTSTS, ~0);

	desc_num = (bytes + MMC_DMA_MAX_BUFFER_SIZE - 1) /
		   MMC_DMA_MAX_BUFFER_SIZE;

	desc = (struct idmac_desc *)MMC_DESC_BASE;

	for (i = 0; i < desc_num; i++) {
		(desc + i)->des0 = IDMAC_DES0_OWN | IDMAC_DES0_CH |
				   IDMAC_DES0_DIC;
		(desc + i)->des1 = IDMAC_DES1_BS1(MMC_DMA_MAX_BUFFER_SIZE);
		/* buffer address */
		(desc + i)->des2 = buffer + MMC_DMA_MAX_BUFFER_SIZE * i;
		/* next descriptor address */
		(desc + i)->des3 = MMC_DESC_BASE +
				   (sizeof(struct idmac_desc) * (i + 1));
	}
	/* first descriptor */
	desc->des0 |= IDMAC_DES0_FS;
	/* last descriptor */
	last_idx = desc_num - 1;
	(desc + last_idx)->des0 |= IDMAC_DES0_LD;
	(desc + last_idx)->des0 &= ~(IDMAC_DES0_DIC | IDMAC_DES0_CH);
	(desc + last_idx)->des1 = IDMAC_DES1_BS1(bytes - (last_idx *
				  MMC_DMA_MAX_BUFFER_SIZE));
	/* set next descriptor address as 0 */
	(desc + last_idx)->des3 = 0;

	mmio_write_32(MMC0_DBADDR, MMC_DESC_BASE);

	ret = mmc0_send_cmd(25, lba, resp_buf);
	if (ret) {
		NOTICE("failed to send CMD25\n");
		mmio_write_32(MMC0_RINTSTS, ~0);
		return -EFAULT;
	}
	ret = wait_data_ready();
	if (ret)
		return ret;

	ret = mmc0_send_cmd(12, EMMC_FIX_RCA << 16, resp_buf);
	if (ret) {
		NOTICE("failed to send CMD12\n");
		mmio_write_32(MMC0_RINTSTS, ~0);
		return -EFAULT;
	}

	do {
		ret = mmc0_send_cmd(13, EMMC_FIX_RCA << 16, resp_buf);
		if (ret) {
			NOTICE("failed to send command 13\n");
			return ret;
		}
	} while (!(resp_buf[0] & MMC_STATUS_READY_FOR_DATA) ||
		 (mmc_state(resp_buf[0] != MMC_STATE_TRAN)));

	if (boot_partition) {
		/* switch back to normal partition */
		ret = mmc0_update_ext_csd(EXT_CSD_PARTITION_CONFIG,
					  PART_CFG_BOOT_PARTITION1_ENABLE);
		if (ret)
			NOTICE("fail to switch eMMC normal partition\n");
	}
	return ret;
}

int mmc0_write(unsigned long mmc_start, size_t size,
		unsigned long buffer, uint32_t boot_partition)
{
	unsigned int mmc_blk_start = mmc_start / MMC_BLOCK_SIZE;
	unsigned int mmc_blk_cnt, offset;

	offset = mmc_start % MMC_BLOCK_SIZE;
	mmc_blk_cnt = (size + offset + MMC_BLOCK_SIZE - 1) / MMC_BLOCK_SIZE;

	return write_multi_blocks(mmc_blk_start, mmc_blk_cnt, buffer,
				  boot_partition);
}

int init_mmc(void)
{
	int ret;

	enable_mmc0();

	ret = enum_mmc0_card();
	if (ret)
		return ret;

	/* set boot mode to 8-bit */
	mmc0_update_ext_csd(177, 2);
	/* response to RESET signal */
	mmc0_update_ext_csd(162, 1);
	/* set access userdata area */
	mmc0_update_ext_csd(EXT_CSD_PARTITION_CONFIG,
			    PART_CFG_BOOT_PARTITION1_ENABLE);

	mmio_write_32(MMC0_RINTSTS, ~0);

	return 0;
}
