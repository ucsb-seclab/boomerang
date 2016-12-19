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

#include <debug.h>
#include <dw_mmc.h>
#include <errno.h>
#include <io_storage.h>
#include <mmio.h>
#include <partitions.h>
#include <platform_def.h>
#include <string.h>
#include "hikey_private.h"

#define EFI_ENTRIES		128
#define EFI_ENTRY_SIZE		(sizeof(struct efi_entry))
#define EFI_MBR_SIZE		512
#define EFI_HEADER_SIZE		512
#define EFI_TOTAL_SIZE		(EFI_MBR_SIZE + EFI_HEADER_SIZE +	\
				EFI_ENTRY_SIZE * EFI_ENTRIES)

struct efi_header {
	char		signature[8];
	uint32_t	revision;
	uint32_t	size;
	uint32_t	header_crc;
	uint32_t	reserved;
	uint64_t	current_lba;
	uint64_t	backup_lba;
	uint64_t	first_lba;
	uint64_t	last_lba;
	uint8_t		disk_uuid[16];
	/* starting LBA of array of partition entries */
	uint64_t	part_lba;
	/* number of partition entries in array */
	uint32_t	part_num;
	/* size of a single partition entry (usually 128) */
	uint32_t	part_size;
	uint32_t	part_crc;
};

struct efi_entry {
	uint8_t		type_uuid[16];
	uint8_t		uniq_uuid[16];
	uint64_t	first_lba;
	uint64_t	last_lba;
	uint64_t	attr;
	uint16_t	name[EFI_NAMELEN];
};

/* the first entry is dummy for ptable (covers both primary & secondary) */
static struct ptentry ptable[EFI_ENTRIES + 1];
static int entries;	/* partition entry entries */

static void dump_entries(void)
{
	int i;

	VERBOSE("Partition table with %d entries:\n", entries);
	for (i = 0; i < entries; i++) {
		VERBOSE("%s %llx-%llx\n", ptable[i].name,
			ptable[i].start,
			ptable[i].start + ptable[i].length - 4);
	}
}

static int convert_ascii_string(uint16_t *str_in, uint8_t *str_out)
{
	uint8_t *name = (uint8_t *)str_in;
	int i;

	if (name[0] == '\0' || !str_in || !str_out)
		return -EINVAL;
	for (i = 1; i < (EFI_NAMELEN << 1); i += 2) {
		if (name[i] != '\0')
			return -EINVAL;
	}
	for (i = 0; i < (EFI_NAMELEN << 1); i += 2) {
		str_out[i >> 1] = name[i];
		if (name[i] == '\0')
			break;
	}
	return 0;
}

static int parse_entry(uintptr_t buf)
{
	struct efi_entry *entry = (struct efi_entry *)buf;
	int ret;

	/* exhaused partition entry */
	if ((entry->first_lba == 0) && (entry->last_lba == 0))
		return 1;
	ret = convert_ascii_string(entry->name, (uint8_t *)ptable[entries].name);
	if (ret < 0)
		return ret;
	ptable[entries].start = (uint64_t)entry->first_lba * 512;
	ptable[entries].length = (uint64_t)(entry->last_lba - entry->first_lba + 1) * 512;
	entries++;
	return 0;
}

/* create dummy entry for ptable */
static void create_dummy_entry(void)
{
	int bytes;
	ptable[entries].start = 0;
	ptable[entries].length = 0;
	bytes = sprintf(ptable[entries].name, "ptable");
	ptable[entries].name[bytes] = '\0';
	entries++;
}

struct ptentry *find_ptn(const char *str)
{
	struct ptentry *ptn = NULL;
	int i;

	for (i = 0; i < entries; i++) {
		if (!strcmp(ptable[i].name, str)) {
			ptn = &ptable[i];
			break;
		}
	}
	return ptn;
}

int get_partition(void)
{
	int result = IO_FAIL;
	int i, ret, num_entries;
	size_t bytes_read;
	uintptr_t emmc_dev_handle, spec, img_handle;
	unsigned int buf[MMC_BLOCK_SIZE >> 2];
	struct efi_header *hd = NULL;

	create_dummy_entry();
	result = plat_get_image_source(NORMAL_EMMC_NAME, &emmc_dev_handle,
				       &spec);
	if (result) {
		WARN("failed to open eMMC normal partition\n");
		return result;
	}
	result = io_open(emmc_dev_handle, spec, &img_handle);
	if (result != IO_SUCCESS) {
		WARN("Failed to open eMMC device\n");
		return result;
	}
	result = io_seek(img_handle, IO_SEEK_SET, 0);
	if (result)
		goto exit;
	result = io_read(img_handle, (uintptr_t)buf, EFI_MBR_SIZE,
			 &bytes_read);
	if ((result != IO_SUCCESS) || (bytes_read < EFI_MBR_SIZE)) {
		WARN("Failed to read eMMC (%i)\n", result);
		goto exit;
	}
	/* check the magic number in last word */
	if (buf[(MMC_BLOCK_SIZE >> 2) - 1] != 0xaa550000) {
		WARN("Can't find MBR protection information\n");
		goto exit;
	}

	result = io_read(img_handle, (uintptr_t)buf, EFI_HEADER_SIZE,
			 &bytes_read);
	if ((result != IO_SUCCESS) || (bytes_read < EFI_HEADER_SIZE)) {
		WARN("Failed to read eMMC (%i)\n", result);
		goto exit;
	}
	hd = (struct efi_header *)((uintptr_t)buf);
	if (strncmp(hd->signature, "EFI PART", 8)) {
		WARN("Failed to find partition table\n");
		goto exit;
	}
	num_entries = hd->part_num;
	for (i = 0; i < num_entries; i++) {
		result = io_read(img_handle, (uintptr_t)buf, EFI_HEADER_SIZE,
				 &bytes_read);
		if ((result != IO_SUCCESS) || (bytes_read < EFI_HEADER_SIZE)) {
			WARN("Failed to read eMMC (%i)\n", result);
			goto exit;
		}
		/* each header contains four partition entries */
		ret = parse_entry((uintptr_t)buf);
		if (ret)
			break;
		ret = parse_entry((uintptr_t)buf + EFI_ENTRY_SIZE);
		if (ret)
			break;
		ret = parse_entry((uintptr_t)buf + EFI_ENTRY_SIZE * 2);
		if (ret)
			break;
		ret = parse_entry((uintptr_t)buf + EFI_ENTRY_SIZE * 3);
		if (ret)
			break;
	}
exit:
	io_close(img_handle);
	update_fip_spec();
	dump_entries();
	return result;
}
