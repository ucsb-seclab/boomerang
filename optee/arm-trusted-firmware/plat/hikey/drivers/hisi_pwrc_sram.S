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

#include <arch.h>
#include <asm_macros.S>
#include <hisi_sram_map.h>

	.global pm_asm_code
	.global pm_asm_code_end
	.global v7_asm
	.global v7_asm_end

	.align	3
func pm_asm_code
	mov	x0, 0
	msr	oslar_el1, x0

	mrs	x0, s3_1_c15_c2_0
	bic	x0, x0, #0x1E000000
 	orr	x0, x0, #0x180000
	orr	x0, x0, #0xe000
	msr	s3_1_c15_c2_0, x0

	mrs	x3, actlr_el3
        orr	x3, x3, #(0x1<<5)
        msr	actlr_el3, x3

        mrs	x3, actlr_el2
        orr	x3, x3, #(0x1<<5)
        msr	actlr_el2, x3

	ldr	x3, =PWRCTRL_ACPU_ASM_D_ARM_PARA_AD
	mrs	x0, mpidr_el1
	and	x1, x0, #MPIDR_CPU_MASK
	and	x0, x0, #MPIDR_CLUSTER_MASK
	add	x0, x1, x0, LSR #6
pen:	ldr	x4, [x3, x0, LSL #3]
	cbz	x4, pen

	mov	x0, #0x0
	mov	x1, #0x0
	mov	x2, #0x0
	mov	x3, #0x0
	br	x4

	.ltorg

pm_asm_code_end:

	.align	3
	.section .rodata.v7_asm, "aS"
v7_asm:
	.word	0xE1A00000	// nop
	.word	0xE3A02003	// mov r2, #3
	.word	0xEE0C2F50	// mcr 15, 0, r2, cr12, cr0, {2}
	.word	0xE320F003	// wfi

	.ltorg
v7_asm_end:
