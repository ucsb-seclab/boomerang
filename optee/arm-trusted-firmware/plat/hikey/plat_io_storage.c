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

#include <arch_helpers.h>
#include <assert.h>
#include <debug.h>
#include <dw_mmc.h>
#include <fastboot.h>
#include <io_block.h>
#include <io_driver.h>
#include <io_fip.h>
#include <io_memmap.h>
#include <io_storage.h>
#include <mmio.h>
#include <partitions.h>
#include <platform_def.h>
#include <semihosting.h>	/* For FOPEN_MODE_... */
#include <string.h>
#include "hikey_private.h"

#define LOADER_MAX_ENTRIES		2
#define PTABLE_MAX_ENTRIES		3
#define USER_MAX_ENTRIES		2

#define FLUSH_BASE			(DDR_BASE + 0x100000)

struct entry_head {
	unsigned char	magic[8];
	unsigned char	name[8];
	unsigned int	start;	/* lba */
	unsigned int	count;	/* lba */
	unsigned int	flag;
};

static const io_dev_connector_t *bl1_mem_dev_con;
static uintptr_t bl1_mem_dev_spec;
static uintptr_t loader_mem_dev_handle;
static uintptr_t bl1_mem_init_params;
static const io_dev_connector_t *fip_dev_con;
static uintptr_t fip_dev_spec;
static uintptr_t fip_dev_handle;
static const io_dev_connector_t *dw_mmc_dev_con;
static struct block_ops dw_mmc_ops;
static uintptr_t emmc_dev_handle;

#define SPARSE_FILL_BUFFER_ADDRESS	0x18000000
#define SPARSE_FILL_BUFFER_SIZE		0x08000000

/* Page 1024, since only a few pages before 2048 are used as partition table */
#define SERIALNO_OFFSET			(1024 * 512)

static const io_block_spec_t loader_mem_spec = {
	/* l-loader.bin that contains bl1.bin */
	.offset = LOADER_RAM_BASE,
	.length = BL1_RO_LIMIT - LOADER_RAM_BASE,
};

static const io_block_spec_t boot_emmc_spec = {
	.offset = MMC_LOADER_BASE,
	.length = BL1_RO_LIMIT - LOADER_RAM_BASE,
};

static const io_block_spec_t normal_emmc_spec = {
	.offset = MMC_BASE,
	.length = MMC_SIZE,
};

static io_block_spec_t fip_block_spec = {
	.offset = 0,
	.length = 0,
};

static const io_file_spec_t bl2_file_spec = {
	.path = BL2_IMAGE_NAME,
	.mode = FOPEN_MODE_RB
};

static const io_file_spec_t bl30_file_spec = {
	.path = BL30_IMAGE_NAME,
	.mode = FOPEN_MODE_RB
};

static const io_file_spec_t bl31_file_spec = {
	.path = BL31_IMAGE_NAME,
	.mode = FOPEN_MODE_RB
};

static const io_file_spec_t bl32_file_spec = {
	.path = BL32_IMAGE_NAME,
	.mode = FOPEN_MODE_RB
};

static const io_file_spec_t bl33_file_spec = {
	.path = BL33_IMAGE_NAME,
	.mode = FOPEN_MODE_RB
};

static int open_loader_mem(const uintptr_t spec);
static int open_fip(const uintptr_t spec);
static int open_dw_mmc(const uintptr_t spec);
static int open_dw_mmc_boot(const uintptr_t spec);

struct plat_io_policy {
	const char	*image_name;
	uintptr_t	*dev_handle;
	uintptr_t	image_spec;
	int		(*check)(const uintptr_t spec);
};

static const struct plat_io_policy policies[] = {
	{
		LOADER_MEM_NAME,
		&loader_mem_dev_handle,
		(uintptr_t)&loader_mem_spec,
		open_loader_mem
	}, {
		BOOT_EMMC_NAME,
		&emmc_dev_handle,
		(uintptr_t)&boot_emmc_spec,
		open_dw_mmc_boot
	}, {
		NORMAL_EMMC_NAME,
		&emmc_dev_handle,
		(uintptr_t)&normal_emmc_spec,
		open_dw_mmc
	}, {
		FIP_IMAGE_NAME,
		&emmc_dev_handle,
		(uintptr_t)&fip_block_spec,
		open_dw_mmc
	}, {
		BL2_IMAGE_NAME,
		&fip_dev_handle,
		(uintptr_t)&bl2_file_spec,
		open_fip
	}, {
		BL30_IMAGE_NAME,
		&fip_dev_handle,
		(uintptr_t)&bl30_file_spec,
		open_fip
	}, {
		BL31_IMAGE_NAME,
		&fip_dev_handle,
		(uintptr_t)&bl31_file_spec,
		open_fip
	}, {
		BL32_IMAGE_NAME,
		&fip_dev_handle,
		(uintptr_t)&bl32_file_spec,
		open_fip
	}, {
		BL33_IMAGE_NAME,
		&fip_dev_handle,
		(uintptr_t)&bl33_file_spec,
		open_fip
	}, {
		0, 0, 0, 0
	}
};

static int open_loader_mem(const uintptr_t spec)
{
	int result = IO_FAIL;
	uintptr_t image_handle;

	result = io_dev_init(loader_mem_dev_handle, bl1_mem_init_params);
	if (result == IO_SUCCESS) {
		result = io_open(loader_mem_dev_handle, spec, &image_handle);
		if (result == IO_SUCCESS) {
			io_close(image_handle);
		}
	}
	return result;
}

static int open_fip(const uintptr_t spec)
{
	int result = IO_FAIL;

	/* See if a Firmware Image Package is available */
	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_NAME);
	if (result == IO_SUCCESS) {
		INFO("Using FIP\n");
		/*TODO: Check image defined in spec is present in FIP. */
	}
	return result;
}


static int open_dw_mmc(const uintptr_t spec)
{
	int result = IO_FAIL;
	uintptr_t image_handle;

	/* indicate to select normal partition in eMMC */
	result = io_dev_init(emmc_dev_handle, 0);
	if (result == IO_SUCCESS) {
		result = io_open(emmc_dev_handle, spec, &image_handle);
		if (result == IO_SUCCESS) {
			/* INFO("Using DW MMC IO\n"); */
			io_close(image_handle);
		}
	}
	return result;
}

static int open_dw_mmc_boot(const uintptr_t spec)
{
	int result = IO_FAIL;
	uintptr_t image_handle;

	/* indicate to select boot partition in eMMC */
	result = io_dev_init(emmc_dev_handle, 1);
	if (result == IO_SUCCESS) {
		result = io_open(emmc_dev_handle, spec, &image_handle);
		if (result == IO_SUCCESS) {
			/* INFO("Using DW MMC IO\n"); */
			io_close(image_handle);
		}
	}
	return result;
}

void io_setup(void)
{
	int io_result = IO_FAIL;

	/* Register the IO devices on this platform */
	io_result = register_io_dev_fip(&fip_dev_con);
	assert(io_result == IO_SUCCESS);

	io_result = register_io_dev_block(&dw_mmc_dev_con);
	assert(io_result == IO_SUCCESS);

	io_result = register_io_dev_memmap(&bl1_mem_dev_con);
	assert(io_result == IO_SUCCESS);

	/* Open connections to devices and cache the handles */
	io_result = io_dev_open(fip_dev_con, fip_dev_spec, &fip_dev_handle);
	assert(io_result == IO_SUCCESS);

	dw_mmc_ops.init = init_mmc;
	dw_mmc_ops.read = mmc0_read;
	dw_mmc_ops.write = mmc0_write;
	io_result = io_dev_open(dw_mmc_dev_con, (uintptr_t)&dw_mmc_ops,
				&emmc_dev_handle);
	assert(io_result == IO_SUCCESS);

	io_result = io_dev_open(bl1_mem_dev_con, bl1_mem_dev_spec,
				&loader_mem_dev_handle);
	assert(io_result == IO_SUCCESS);

	/* Ignore improbable errors in release builds */
	(void)io_result;
}

/* Return an IO device handle and specification which can be used to access
 * an image. Use this to enforce platform load policy */
int plat_get_image_source(const char *image_name, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	int result = IO_FAIL;
	const struct plat_io_policy *policy;

	if ((image_name != NULL) && (dev_handle != NULL) &&
	    (image_spec != NULL)) {
		policy = policies;
		while (policy->image_name != NULL) {
			if (strcmp(policy->image_name, image_name) == 0) {
				result = policy->check(policy->image_spec);
				if (result == IO_SUCCESS) {
					*image_spec = policy->image_spec;
					*dev_handle = *(policy->dev_handle);
					break;
				}
			}
			policy++;
		}
	} else {
		result = IO_FAIL;
	}
	return result;
}

int update_fip_spec(void)
{
	struct ptentry *ptn;

	ptn = find_ptn("fastboot");
	if (!ptn) {
		WARN("failed to find partition fastboot\n");
		ptn = find_ptn("bios");
		if (!ptn) {
			WARN("failed to find partition bios\n");
			return IO_FAIL;
		}
	}
	VERBOSE("%s: name:%s, start:%llx, length:%llx\n",
		__func__, ptn->name, ptn->start, ptn->length);
	fip_block_spec.offset = ptn->start;
	fip_block_spec.length = ptn->length;
	return IO_SUCCESS;
}

static int fetch_entry_head(void *buf, int num, struct entry_head *hd)
{
	unsigned char magic[8] = "ENTRYHDR";
	if (hd == NULL)
		return IO_FAIL;
	memcpy((void *)hd, buf, sizeof(struct entry_head) * num);
	if (!strncmp((void *)hd->magic, (void *)magic, 8))
		return IO_SUCCESS;
	return IO_NOT_SUPPORTED;
}

static int flush_loader(void)
{
	struct entry_head entries[5];
	uintptr_t img_handle, spec;
	int result = IO_FAIL;
	size_t bytes_read, length;
	ssize_t offset;
	int i, fp;

	result = fetch_entry_head((void *)(FLUSH_BASE + 28),
				  LOADER_MAX_ENTRIES, entries);
	if (result) {
		WARN("failed to parse entries in loader image\n");
		return result;
	}

	spec = 0;
	for (i = 0, fp = 0; i < LOADER_MAX_ENTRIES; i++) {
		if (entries[i].flag != 1) {
			WARN("Invalid flag in entry:0x%x\n", entries[i].flag);
			return IO_NOT_SUPPORTED;
		}
		result = plat_get_image_source(BOOT_EMMC_NAME, &emmc_dev_handle,
					       &spec);
		if (result) {
			WARN("failed to open emmc boot area\n");
			return result;
		}
		/* offset in Boot Area1 */
		offset = MMC_LOADER_BASE + entries[i].start * 512;

		result = io_open(emmc_dev_handle, spec, &img_handle);
		if (result != IO_SUCCESS) {
			WARN("Failed to open memmap device\n");
			return result;
		}
		length = entries[i].count * 512;

		result = io_seek(img_handle, IO_SEEK_SET, offset);
		if (result)
			goto exit;

		if (i == 1)
			fp = (entries[1].start - entries[0].start) * 512;
		result = io_write(img_handle, FLUSH_BASE + fp, length,
				  &bytes_read);
		if ((result != IO_SUCCESS) || (bytes_read < length)) {
			WARN("Failed to write '%s' file (%i)\n",
			     LOADER_MEM_NAME, result);
			goto exit;
		}
		io_close(img_handle);
	}
	return result;
exit:
	io_close(img_handle);
	return result;
}

/*
 * Flush l-loader.bin (loader & bl1.bin) into Boot Area1 of eMMC.
 */
int flush_loader_image(void)
{
	uintptr_t bl1_image_spec;
	int result = IO_FAIL;
	size_t bytes_read, length;
	uintptr_t img_handle;

	result = plat_get_image_source(LOADER_MEM_NAME, &loader_mem_dev_handle,
				       &bl1_image_spec);

	result = io_open(loader_mem_dev_handle, bl1_image_spec, &img_handle);
	if (result != IO_SUCCESS) {
		WARN("Failed to open memmap device\n");
		goto exit;
	}
	length = loader_mem_spec.length;
	result = io_read(img_handle, FLUSH_BASE, length, &bytes_read);
	if ((result != IO_SUCCESS) || (bytes_read < length)) {
		WARN("Failed to load '%s' file (%i)\n", LOADER_MEM_NAME, result);
		goto exit;
	}
	io_close(img_handle);

	result = flush_loader();
	if (result != IO_SUCCESS) {
		io_dev_close(loader_mem_dev_handle);
		return result;
	}
exit:
	io_close(img_handle);
	io_dev_close(loader_mem_dev_handle);
	return result;
}

static int flush_single_image(const char *mmc_name, unsigned long img_addr,
				ssize_t offset, size_t length)
{
	uintptr_t img_handle, spec = 0;
	size_t bytes_read;
	int result = IO_FAIL;

	result = plat_get_image_source(mmc_name, &emmc_dev_handle,
				       &spec);
	if (result) {
		NOTICE("failed to open emmc user data area\n");
		return result;
	}

	result = io_open(emmc_dev_handle, spec, &img_handle);
	if (result != IO_SUCCESS) {
		NOTICE("Failed to open memmap device\n");
		return result;
	}

	result = io_seek(img_handle, IO_SEEK_SET, offset);
	if (result) {
		NOTICE("Failed to seek at offset:0x%x\n", offset);
		goto exit;
	}

	result = io_write(img_handle, img_addr, length,
			  &bytes_read);
	if ((result != IO_SUCCESS) || (bytes_read < length)) {
		NOTICE("Failed to write file (%i)\n", result);
		goto exit;
	}
exit:
	io_close(img_handle);
	return result;
}

static int is_sparse_image(unsigned long img_addr)
{
	if (*(uint32_t *)img_addr == SPARSE_HEADER_MAGIC)
		return 1;
	return 0;
}

static int do_unsparse(char *cmdbuf, unsigned long img_addr, unsigned long img_length)
{
	sparse_header_t *header = (sparse_header_t *)img_addr;
	chunk_header_t *chunk = NULL;
	struct ptentry *ptn;
	void *data = (void *)img_addr;
	uint64_t out_blks = 0, out_length = 0;
	uint64_t length;
	uint32_t fill_value;
	uint64_t left, count;
	int i, result;

	ptn = find_ptn(cmdbuf);
	if (!ptn) {
		NOTICE("failed to find partition %s\n", cmdbuf);
		return IO_FAIL;
	}
	length = (uint64_t)(header->total_blks) * (uint64_t)(header->blk_sz);
	if (length > ptn->length) {
		NOTICE("Unsparsed image length is %lld, pentry length is %lld.\n",
			length, ptn->length);
		return IO_FAIL;
	}

	data = (void *)((unsigned long)data + header->file_hdr_sz);
	for (i = 0; i < header->total_chunks; i++) {
		chunk = (chunk_header_t *)data;
		data = (void *)((unsigned long)data + sizeof(chunk_header_t));
		length = (uint64_t)chunk->chunk_sz * (uint64_t)header->blk_sz;

		switch (chunk->chunk_type) {
		case CHUNK_TYPE_RAW:
			result = flush_single_image(NORMAL_EMMC_NAME,
						    (unsigned long)data,
						    ptn->start + out_length, length);
			if (result < 0) {
				NOTICE("sparse: failed to flush raw chunk\n");
				return result;
			}
			out_blks += length / 512;
			out_length += length;
			/* next chunk is just after the raw data */
			data = (void *)((unsigned long)data + length);
			break;
		case CHUNK_TYPE_FILL:
			if (chunk->total_sz != (sizeof(unsigned int) + sizeof(chunk_header_t))) {
				NOTICE("sparse: bad chunk size\n");
				return IO_FAIL;
			}
			fill_value = *(unsigned int *)data;
			if (fill_value != 0) {
				NOTICE("sparse: filled value shouldn't be zero.\n");
			}
			memset((void *)SPARSE_FILL_BUFFER_ADDRESS,
				0, SPARSE_FILL_BUFFER_SIZE);
			left = length;
			while (left > 0) {
				if (left < SPARSE_FILL_BUFFER_SIZE)
					count = left;
				else
					count = SPARSE_FILL_BUFFER_SIZE;
				result = flush_single_image(NORMAL_EMMC_NAME,
							    SPARSE_FILL_BUFFER_ADDRESS,
							    ptn->start + out_length, count);
				if (result < 0) {
					WARN("sparse: failed to flush fill chunk\n");
					return result;
				}
				out_blks += count / 512;
				out_length += count;
				left = left - count;
			}
			/* next chunk is just after the filled data */
			data = (void *)((unsigned long)data + sizeof(unsigned int));
			break;
		case CHUNK_TYPE_DONT_CARE:
			if (chunk->total_sz != sizeof(chunk_header_t)) {
				NOTICE("sparse: unmatched chunk size\n");
				return IO_FAIL;
			}
			out_blks += length / 512;
			out_length += length;
			break;
		default:
			NOTICE("sparse: unrecognized type 0x%x\n", chunk->chunk_type);
			break;
		}
	}
	return 0;
}

/* Page 1024 is used to store serial number */
int flush_random_serialno(unsigned long addr, unsigned long length)
{
	int result;

	memset((void *)SPARSE_FILL_BUFFER_ADDRESS, 0, 512);
	memcpy((void *)SPARSE_FILL_BUFFER_ADDRESS, (void *)addr, length);
	result = flush_single_image(NORMAL_EMMC_NAME, SPARSE_FILL_BUFFER_ADDRESS,
				    SERIALNO_OFFSET, 512);
	return result;
}

char *load_serialno(void)
{
	uintptr_t img_handle, spec = 0;
	size_t bytes_read;
	struct random_serial_num *random = NULL;
	int result;

	result = plat_get_image_source(NORMAL_EMMC_NAME, &emmc_dev_handle,
				       &spec);
	if (result) {
		NOTICE("failed to open emmc user data area\n");
		return NULL;
	}

	result = io_open(emmc_dev_handle, spec, &img_handle);
	if (result != IO_SUCCESS) {
		NOTICE("Failed to open memmap device\n");
		return NULL;
	}

	result = io_seek(img_handle, IO_SEEK_SET, SERIALNO_OFFSET);
	if (result) {
		NOTICE("Failed to seek at offset 0\n");
		goto exit;
	}
	result = io_read(img_handle, SPARSE_FILL_BUFFER_ADDRESS, 512, &bytes_read);
	if ((result != IO_SUCCESS) || (bytes_read < 512)) {
		NOTICE("Failed to load '%s' file (%i)\n", LOADER_MEM_NAME, result);
		goto exit;
	}
	io_close(img_handle);

	random = (struct random_serial_num *)SPARSE_FILL_BUFFER_ADDRESS;
	if (random->magic != RANDOM_MAGIC)
		return NULL;

	return random->serialno;
exit:
	io_close(img_handle);
	return NULL;
}

/*
 * Flush bios.bin into User Data Area in eMMC
 */
int flush_user_images(char *cmdbuf, unsigned long img_addr,
		      unsigned long img_length)
{
	struct entry_head entries[5];
	struct ptentry *ptn;
	size_t length;
	ssize_t offset;
	int result = IO_FAIL;
	int i, fp;

	result = fetch_entry_head((void *)img_addr, USER_MAX_ENTRIES, entries);
	switch (result) {
	case IO_NOT_SUPPORTED:
		if (!strncmp(cmdbuf, "fastboot", 8) ||
		    !strncmp(cmdbuf, "bios", 4)) {
			update_fip_spec();
		}
		if (is_sparse_image(img_addr)) {
			result = do_unsparse(cmdbuf, img_addr, img_length);
		} else {
			ptn = find_ptn(cmdbuf);
			if (!ptn) {
				WARN("failed to find partition %s\n", cmdbuf);
				return IO_FAIL;
			}
			img_length = (img_length + 512 - 1) / 512 * 512;
			result = flush_single_image(NORMAL_EMMC_NAME, img_addr,
						    ptn->start, img_length);
		}
		break;
	case IO_SUCCESS:
		if (strncmp(cmdbuf, "ptable", 6)) {
			WARN("it's not for ptable\n");
			return IO_FAIL;
		}
		/* currently it's for partition table */
		/* the first block is for entry headers */
		fp = 512;

		for (i = 0; i < USER_MAX_ENTRIES; i++) {
			if (entries[i].flag != 0) {
				WARN("Invalid flag in entry:0x%x\n",
					entries[i].flag);
				return IO_NOT_SUPPORTED;
			}
			if (entries[i].count == 0)
				continue;
			length = entries[i].count * 512;
			offset = MMC_BASE + entries[i].start * 512;
			VERBOSE("i:%d, start:%x, count:%x\n",
				i, entries[i].start, entries[i].count);
			result = flush_single_image(NORMAL_EMMC_NAME,
						img_addr + fp, offset, length);
			fp += entries[i].count * 512;
		}
		get_partition();
		break;
	case IO_FAIL:
		WARN("failed to parse entries in user image.\n");
		return result;
	}
	return result;
}
