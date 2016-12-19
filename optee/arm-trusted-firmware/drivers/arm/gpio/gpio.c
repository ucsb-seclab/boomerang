/*
 * Copyright (c) 2014-2015, Linaro Ltd and Contributors. All rights reserved.
 * Copyright (c) 2014-2015, Hisilicon Ltd and Contributors. All rights reserved.
 *
 * GPIO driver for PL061
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
#include <gpio.h>
#include <mmio.h>

#define MAX_GPIO_DEVICES	32
#define GPIOS_PER_DEV		8
#define GPIO_DIR		0x400

#define BIT(nr)			(1UL << (nr))

struct gpio_device_t {
	unsigned int	base[MAX_GPIO_DEVICES];
	unsigned int	count;
};

static struct gpio_device_t gpio_dev;

/* return 0 for failure */
static unsigned int find_gc_base(unsigned int gpio)
{
	int gc;

	gc = gpio / GPIOS_PER_DEV;
	if (gc >= gpio_dev.count)
		return 0;
	return gpio_dev.base[gc];
}

int gpio_direction_input(unsigned int gpio)
{
	unsigned int gc_base, offset, data;

	gc_base = find_gc_base(gpio);
	if (!gc_base)
		return -EINVAL;
	offset = gpio % GPIOS_PER_DEV;

	data = mmio_read_8(gc_base + GPIO_DIR);
	data &= ~(1 << offset);
	mmio_write_8(gc_base + GPIO_DIR, data);
	return 0;
}

int gpio_direction_output(unsigned int gpio)
{
	unsigned int gc_base, offset, data;

	gc_base = find_gc_base(gpio);
	if (!gc_base)
		return -EINVAL;
	offset = gpio % 8;

	data = mmio_read_8(gc_base + GPIO_DIR);
	data |= 1 << offset;
	mmio_write_8(gc_base + GPIO_DIR, data);
	return 0;
}

int gpio_get_value(unsigned int gpio)
{
	unsigned int gc_base, offset;

	gc_base = find_gc_base(gpio);
	if (!gc_base)
		return -EINVAL;
	offset = gpio % 8;

	return !!mmio_read_8(gc_base + (BIT(offset + 2)));
}

int gpio_set_value(unsigned int gpio, unsigned int value)
{
	unsigned int gc_base, offset;

	gc_base = find_gc_base(gpio);
	if (!gc_base)
		return -EINVAL;
	offset = gpio % 8;
	mmio_write_8(gc_base + (BIT(offset + 2)), !!value << offset); 
	return 0;
}

int gpio_register_device(unsigned int base)
{
	int i;
	if (gpio_dev.count > MAX_GPIO_DEVICES)
		return -EINVAL;
	for (i = 0; i < gpio_dev.count; i++) {
		if (gpio_dev.base[i] == base) {
			WARN("%s: duplicated gpio base\n", __func__);
			return -EINVAL;
		}
	}
	gpio_dev.base[gpio_dev.count] = base;
	gpio_dev.count++;
	return 0;
}
