/*
 * Copyright (c) 2014 Linaro Ltd.
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
	. = 0xf9800800;
	LLOADER_START = .;

	.text :
	{
		*(.text)
		*(.rodata)
	}

	.data ALIGN(4):
	{
		*(.data)
	}

	. = ALIGN(4);

	.bss ALIGN(4):
	{
		*(.bss)
	}

	LLOADER_BL1_BIN = 0xf9801000;
}
