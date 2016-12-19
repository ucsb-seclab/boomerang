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

#include <assert.h>
#include <bl_common.h>
#include <debug.h>
#include <errno.h>
#include <io_block.h>
#include <io_driver.h>
#include <io_storage.h>
#include <mmio.h>
#include <stdint.h>
#include <string.h>

/* As we need to be able to keep state for seek, only one file can be open
 * at a time. Make this a structure and point to the entity->info. When we
 * can malloc memory we can change this to support more open files.
 */
typedef struct {
	/* Use the 'in_use' flag as any value for base and file_pos could be
	 * valid.
	 */
	int		in_use;
	uintptr_t	base;
	size_t		file_pos;
	uint32_t	flags;
} file_state_t;

struct block_info {
	struct block_ops	ops;
	int			init;
	uint32_t		flags;
};

static file_state_t current_file = {0};

static struct block_info block_info;

static int block_open(io_dev_info_t *dev_info, const uintptr_t spec,
		      io_entity_t *entity);
static int block_seek(io_entity_t *entity, int mode, ssize_t offset);
static int block_read(io_entity_t *entity, uintptr_t buffer,
		      size_t length, size_t *length_read);
static int block_write(io_entity_t *entity, uintptr_t buffer,
		       size_t length, size_t *length_written);
static int block_close(io_entity_t *entity);

static int blk_dev_init(io_dev_info_t *dev_info,
			const uintptr_t init_params);
static int blk_dev_open(const uintptr_t dev_spec, io_dev_info_t **dev_info);
static int blk_dev_close(io_dev_info_t *dev_info);

/* Identify the device type as block */
io_type_t device_type_block(void)
{
	return IO_TYPE_BLOCK;
}

static const io_dev_connector_t blk_dev_connector = {
	.dev_open = blk_dev_open
};

static const io_dev_funcs_t blk_dev_funcs = {
	.type = device_type_block,
	.open = block_open,
	.seek = block_seek,
	.size = NULL,
	.read = block_read,
	.write = block_write,
	.close = block_close,
	.dev_init = blk_dev_init,
	.dev_close = blk_dev_close,
};


/* No state associated with this device so structure can be const */
static const io_dev_info_t blk_dev_info = {
	.funcs = &blk_dev_funcs,
	.info = (uintptr_t)&block_info,
};

/* Open a connection to the block device */
static int blk_dev_open(const uintptr_t dev_spec __attribute__((unused)),
			   io_dev_info_t **dev_info)
{
	struct block_ops	*funcs, *block_spec;

	assert(dev_info != NULL);
	*dev_info = (io_dev_info_t *)&blk_dev_info; /* cast away const */

	if (dev_spec) {
		funcs = &block_info.ops;
		block_spec = (struct block_ops *)dev_spec;
		funcs->init = block_spec->init;
		funcs->read = block_spec->read;
		funcs->write = block_spec->write;
	}

	return IO_SUCCESS;
}

/* Close a connection to the block device */
static int blk_dev_close(io_dev_info_t *dev_info)
{
	/* NOP */
	/* TODO: Consider tracking open files and cleaning them up here */
	return IO_SUCCESS;
}


/* Open a file on the block device */
/* TODO: Can we do any sensible limit checks on requested memory */
static int block_open(io_dev_info_t *dev_info, const uintptr_t spec,
			     io_entity_t *entity)
{
	int result = IO_FAIL;
	const io_block_spec_t *block_spec = (io_block_spec_t *)spec;
	struct block_info *info = (struct block_info *)(dev_info->info);

	/* Since we need to track open state for seek() we only allow one open
	 * spec at a time. When we have dynamic memory we can malloc and set
	 * entity->info.
	 */
	if (current_file.in_use == 0) {
		assert(block_spec != NULL);
		assert(entity != NULL);

		current_file.in_use = 1;
		current_file.base = block_spec->offset;
		/* File cursor offset for seek and incremental reads etc. */
		current_file.file_pos = 0;
		current_file.flags = info->flags;
		entity->info = (uintptr_t)&current_file;
		result = IO_SUCCESS;
	} else {
		WARN("A block device is already active. Close first.\n");
		result = IO_RESOURCES_EXHAUSTED;
	}

	return result;
}

/* Seek to a particular file offset on the block device */
static int block_seek(io_entity_t *entity, int mode, ssize_t offset)
{
	int result = IO_FAIL;

	/* We only support IO_SEEK_SET for the moment. */
	if (mode == IO_SEEK_SET) {
		assert(entity != NULL);

		/* TODO: can we do some basic limit checks on seek? */
		((file_state_t *)entity->info)->file_pos = offset;
		result = IO_SUCCESS;
	} else {
		result = IO_FAIL;
	}

	return result;
}


/* Read data from a file on the block device */
static int block_read(io_entity_t *entity, uintptr_t buffer,
		      size_t length, size_t *length_read)
{
	file_state_t *fp;
	int result;

	assert(entity != NULL);
	assert(buffer != (uintptr_t)NULL);
	assert(length_read != NULL);

	fp = (file_state_t *)entity->info;

	if (!block_info.ops.read) {
		ERROR("There's no read function on the block device.\n");
		return IO_NOT_SUPPORTED;
	}
	result = block_info.ops.read(fp->base + fp->file_pos, length,
				     buffer, fp->flags);
	if (result) {
		WARN("Failed to read block offset 0x%x\n",
		     fp->base + fp->file_pos);
		return result;
	}

	*length_read = length;
	/* advance the file 'cursor' for incremental reads */
	fp->file_pos += length;

	return IO_SUCCESS;
}

static int block_write(io_entity_t *entity, uintptr_t buffer,
		       size_t length, size_t *length_written)
{
	file_state_t *fp;
	int result;

	assert(entity != NULL);
	assert(buffer != (uintptr_t)NULL);
	assert(length_written != NULL);

	fp = (file_state_t *)entity->info;

	if (!block_info.ops.write) {
		ERROR("There's no write function on the block device.\n");
		return IO_NOT_SUPPORTED;
	}
	result = block_info.ops.write(fp->base + fp->file_pos, length,
				      buffer, fp->flags);
	if (result) {
		WARN("Failed to write block offset 0x%x\n",
		     fp->base + fp->file_pos);
		return result;
	}

	*length_written = length;
	/* advance the file 'cursor' for incremental reads */
	fp->file_pos += length;

	return IO_SUCCESS;
}

/* Close a file on the BLOCK device */
static int block_close(io_entity_t *entity)
{
	assert(entity != NULL);

	entity->info = 0;

	/* This would be a mem free() if we had malloc.*/
	memset((void *)&current_file, 0, sizeof(current_file));

	return IO_SUCCESS;
}

static int blk_dev_init(io_dev_info_t *dev_info, const uintptr_t init_params)
{
	struct block_info *info = (struct block_info *)(dev_info->info);

	if (!info->init) {
		if (block_info.ops.init)
			block_info.ops.init();
		info->init = 1;
	}
	info->flags = init_params;
	return IO_SUCCESS;
}

/* Exported functions */

/* Register the block driver with the IO abstraction */
int register_io_dev_block(const io_dev_connector_t **dev_con)
{
	int result = IO_FAIL;
	assert(dev_con != NULL);

	result = io_register_device(&blk_dev_info);
	if (result == IO_SUCCESS)
		*dev_con = &blk_dev_connector;

	return result;
}
