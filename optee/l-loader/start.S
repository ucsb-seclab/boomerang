	.text

/*
 * The head of l-loader is defined in below.
 * struct l_loader_head {
 *	unsigned int	first_instr;
 *	unsigned char	magic[16];	@ BOOTMAGICNUMBER!
 *	unsigned int	l_loader_start;
 *	unsigned int	l_loader_end;
 * };
 */

#define CPU0_CTRL_OFFSET		0x100
#define CPU7_CTRL_OFFSET		0x800
#define CPU0_RVBARADDR_OFFSET		0x158
#define CPU7_RVBARADDR_OFFSET		0x858

#define CPU_CTRL_AARCH64_MODE		(1 << 7)

#define SC_PERIPH_CLKEN3		0x230
#define SC_PERIPH_RSTDIS3		0x334
	.global	_start
_start:
	b	reset
@ Android magic number: "BOOTMAGICNUMBER!"
android_magic:
	.word	0x544f4f42
	.word	0x4947414d
	.word	0x4d554e43
	.word	0x21524542
	.word	LLOADER_START		@ LLOADER_START in RAM
	.word	0			@ LLOADER_END in RAM

entries:
	@ 5 entries with 7 words
	.space	140

	.align	7

reset:
	ldr	r8, =(0xf9800000 + 0x700)
	str	r0, [r8]		@ download mode (1:usb,2:uart,0:boot)

	ldr	r4, =0xf6504000		@ ACPU_CTRL register base
	@ set RVBAR for cpu0
	ldr	r5, =CPU0_RVBARADDR_OFFSET
	ldr	r6, =LLOADER_BL1_BIN
	mov	r6, r6, lsr #2
	str	r6, [r4, r5]
1:
	ldr	r0, [r4, r5]
	cmp	r0, r6
	bne	1b

	mov	r5, #CPU0_CTRL_OFFSET
	mov	r6, #CPU7_CTRL_OFFSET
2:
	ldr	r0, [r4, r5]		@ Load ACPU_SC_CPUx_CTRL
	orr	r0, r0, #CPU_CTRL_AARCH64_MODE
	str	r0, [r4, r5]		@ Save to ACPU_SC_CPUx_CTRL
	ldr	r0, [r4, r5]

	add	r5, r5, #0x100		@ Iterate ACPU_SC_CPUx_CTRL
	cmp	r5, r6
	ble	2b

	/*
	 * Prepare UART2 & UART3 without baud rate initialization.
	 * So always output on UART0 in l-loader.
	 */
	ldr	r4, =0xf70100e0		@ UART2_RXD IOMG register
	mov	r0, #0
	str	r0, [r4]
	str	r0, [r4, #4]		@ UART2_TXD IOMG register
	ldr	r0, [r4]

	ldr	r4, =0xf7010188		@ UART3_RXD IOMG register
	mov	r0, #1
	str	r0, [r4]
	str	r0, [r4, #4]		@ UART3_TXD IOMG register
	ldr	r1, [r4]

	ldr	r4, =0xf7030000		@ PERI_CTRL register base
	@ By default, CLK_TXCO is the parent of CLK_UART3 in SC_CLK_SEL0

	ldr	r5, =SC_PERIPH_RSTDIS3	@ unreset
	ldr	r6, =SC_PERIPH_CLKEN3	@ enable PCLK
	mov	r0, #(3 << 6)		@ bit'6' & bit'7' (UART2 & UART3)
	str	r0, [r4, r5]
	str	r0, [r4, r6]

	@ execute warm reset to switch aarch64
	mov	r2, #3
	mcr	p15, 0, r2, c12, c0, 2
	wfi
panic:
	b	panic

str_aarch64:
	.asciz	"\nSwitch to aarch64 mode. CPU0 executes at 0x"
