#
# Copyright (c) 2014-2015, Linaro Ltd. All rights reserved.
# Copyright (c) 2014-2015, Hisilicon Ltd.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# Neither the name of ARM nor the names of its contributors may be used
# to endorse or promote products derived from this software without specific
# prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

# On Hikey, the TSP can execute either from Trusted SRAM or Trusted DRAM.
# Trusted DRAM is the default.
#
PLAT_TSP_LOCATION	:=	tdram
ifeq (${PLAT_TSP_LOCATION}, tsram)
  PLAT_TSP_LOCATION_ID := PLAT_TRUSTED_SRAM_ID
else ifeq (${PLAT_TSP_LOCATION}, tdram)
  PLAT_TSP_LOCATION_ID := PLAT_TRUSTED_DRAM_ID
else
  $(error "Unsupported PLAT_TSP_LOCATION value")
endif

CONSOLE_BASE		:=	PL011_UART3_BASE
CRASH_CONSOLE_BASE	:=	PL011_UART3_BASE

# Process flags
$(eval $(call add_define,PLAT_TSP_LOCATION_ID))
$(eval $(call add_define,CONSOLE_BASE))
$(eval $(call add_define,CRASH_CONSOLE_BASE))


PLAT_INCLUDES		:=	-Iplat/hikey/include/

PLAT_BL_COMMON_SOURCES	:=	drivers/arm/pl011/pl011_console.S	\
				drivers/io/io_block.c			\
				drivers/io/io_fip.c			\
				drivers/io/io_memmap.c			\
				drivers/io/io_storage.c			\
				lib/aarch64/xlat_tables.c		\
				plat/common/aarch64/plat_common.c	\
				plat/common/plat_gic.c			\
				plat/hikey/aarch64/hikey_common.c	\
				plat/hikey/aarch64/plat_helpers.S	\
				plat/hikey/plat_io_storage.c

BL1_SOURCES		+=	drivers/arm/cci400/cci400.c		\
				drivers/arm/gpio/gpio.c			\
				lib/cpus/aarch64/cortex_a53.S		\
				plat/common/aarch64/platform_up_stack.S	\
				plat/hikey/aarch64/bl1_plat_helpers.S	\
				plat/hikey/bl1_plat_setup.c		\
				plat/hikey/drivers/dw_mmc.c		\
				plat/hikey/drivers/hi6553.c		\
				plat/hikey/drivers/sp804_timer.c	\
				plat/hikey/partitions.c			\
				plat/hikey/pll.c			\
				plat/hikey/usb.c

BL2_SOURCES		+=	plat/common/aarch64/platform_up_stack.S	\
				plat/hikey/bl2_plat_setup.c		\
				plat/hikey/plat_security.c		\
				plat/hikey/drivers/dw_mmc.c		\
				plat/hikey/drivers/hi6553.c		\
				plat/hikey/drivers/hisi_dvfs.c		\
				plat/hikey/drivers/hisi_mcu.c           \
				plat/hikey/drivers/sp804_timer.c	\
				plat/hikey/partitions.c

BL31_SOURCES		+=	drivers/arm/cci400/cci400.c		\
				drivers/arm/gic/arm_gic.c		\
				drivers/arm/gic/gic_v2.c		\
				drivers/arm/gic/gic_v3.c		\
				drivers/arm/gpio/gpio.c			\
				lib/cpus/aarch64/cortex_a53.S		\
				plat/common/aarch64/platform_mp_stack.S	\
				plat/hikey/bl31_plat_setup.c		\
				plat/hikey/drivers/hisi_pwrc.c		\
				plat/hikey/drivers/hisi_pwrc_sram.S	\
				plat/hikey/drivers/hisi_ipc.c		\
				plat/hikey/drivers/sp804_timer.c	\
				plat/hikey/plat_pm.c			\
				plat/hikey/plat_topology.c

NEED_BL30		:=	yes
