/*
 * Copyright (c) 2014, Linaro Ltd and Contributors. All rights reserved.
 * Copyright (c) 2014, Hisilicon Ltd and Contributors. All rights reserved.
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

#ifndef __DWC_USB_H__
#define __DWC_USB_H__

#define USB_DMA

#define DWC_OTG_BASE			0xF72C0000

#define USB_NUM_ENDPOINTS		2
#define MAX_EPS_CHANNELS		16

#define BULK_OUT_EP			1
#define BULK_IN_EP			1

#define RX_REQ_LEN			512
#define MAX_PACKET_LEN			512

#define DATA_FIFO_CONFIG		(0x780 << GDFIFOCFG_EPINFOBASE_SHIFT |\
					 0x800 << GDFIFOCFG_GDFIFOCFG_SHIFT)
/* RX FIFO: 2048 bytes */
#define RX_SIZE				0x00000200
/* Non-periodic TX FIFO: 128 bytes. start address: 0x200 * 4. */
#define ENDPOINT_TX_SIZE		0x00200200

/* EP1  TX FIFO: 1024 bytes. start address: 0x220 * 4. */
/* EP2  TX FIFO: 1024 bytes. start address: 0x320 * 4. */
/* EP3  TX FIFO: 1024 bytes. start address: 0x420 * 4. */
/* EP4  TX FIFO: 1024 bytes. start address: 0x520 * 4. */
/* EP5  TX FIFO: 128  bytes. start address: 0x620 * 4. */
/* EP6  TX FIFO: 128  bytes. start address: 0x640 * 4. */
/* EP7  TX FIFO: 128  bytes. start address: 0x660 * 4. */
/* EP8  TX FIFO: 128  bytes. start address: 0x680 * 4. */
/* EP9  TX FIFO: 128  bytes. start address: 0x6a0 * 4. */
/* EP10 TX FIFO: 128  bytes. start address: 0x6c0 * 4. */
/* EP11 TX FIFO: 128  bytes. start address: 0x6e0 * 4. */
/* EP12 TX FIFO: 128  bytes. start address: 0x700 * 4. */
/* EP13 TX FIFO: 128  bytes. start address: 0x720 * 4. */
/* EP14 TX FIFO: 128  bytes. start address: 0x740 * 4. */
/* EP15 TX FIFO: 128  bytes. start address: 0x760 * 4. */

#define DATA_IN_ENDPOINT_TX_FIFO1	0x01000220
#define DATA_IN_ENDPOINT_TX_FIFO2	0x01000320
#define DATA_IN_ENDPOINT_TX_FIFO3	0x01000420
#define DATA_IN_ENDPOINT_TX_FIFO4	0x01000520
#define DATA_IN_ENDPOINT_TX_FIFO5	0x00200620
#define DATA_IN_ENDPOINT_TX_FIFO6	0x00200640
#define DATA_IN_ENDPOINT_TX_FIFO7	0x00200660
#define DATA_IN_ENDPOINT_TX_FIFO8	0x00200680
#define DATA_IN_ENDPOINT_TX_FIFO9	0x002006a0
#define DATA_IN_ENDPOINT_TX_FIFO10	0x002006c0
#define DATA_IN_ENDPOINT_TX_FIFO11	0x002006e0
#define DATA_IN_ENDPOINT_TX_FIFO12	0x00200700
#define DATA_IN_ENDPOINT_TX_FIFO13	0x00200720
#define DATA_IN_ENDPOINT_TX_FIFO14	0x00200740
#define DATA_IN_ENDPOINT_TX_FIFO15	0x00200760

typedef struct {
	unsigned char		type;
	unsigned char		request;
	unsigned short		value;
	unsigned short		index;
	unsigned short		length;
} setup_packet;

struct ept_queue_item {
	unsigned int		next;
	unsigned int		info;
};

struct usb_request {
	struct ept_queue_item	*item;
	void			*buf;
	unsigned int		length;
	void (*complete)(unsigned int actual, int status);
	void			*context;
};

/*DWC_OTG regsiter descriptor*/
/*Global CSR MAP*/
#define GLOBAL_CSR_BASE		(DWC_OTG_BASE)
/*Device mode CSR MAP*/
#define DEVICE_CSR_BASE		(DWC_OTG_BASE+0x800)
/*Device mode CSR MAP*/
#define DEVICE_INEP_BASE	(DWC_OTG_BASE+0x900)
/*Device mode CSR MAP*/
#define DEVICE_OUTEP_BASE	(DWC_OTG_BASE+0xB00)

/*** OTG LINK CORE REGISTERS ***/
/* Core Global Registers */
#define GOTGCTL			(DWC_OTG_BASE + 0x000)
#define GOTGINT			(DWC_OTG_BASE + 0x004)
#define GOTGINT_DBNCE_DONE		(1 << 19)
#define GOTGINT_A_DEV_TOUT_CHG		(1 << 18)
#define GOTGINT_HST_NEG_DET		(1 << 17)
#define GOTGINT_HST_NEG_SUC_STS_CHNG	(1 << 9)
#define GOTGINT_SES_REQ_SUC_STS_CHNG	(1 << 8)
#define GOTGINT_SES_END_DET		(1 << 2)

#define GAHBCFG			(DWC_OTG_BASE + 0x008)
#define GAHBCFG_P_TXF_EMP_LVL           (1 << 8)
#define GAHBCFG_NP_TXF_EMP_LVL          (1 << 7)
#define GAHBCFG_DMA_EN                  (1 << 5)
#define GAHBCFG_GLBL_INTR_EN            (1 << 0)
#define GAHBCFG_CTRL_MASK               (GAHBCFG_P_TXF_EMP_LVL | \
					 GAHBCFG_NP_TXF_EMP_LVL | \
					 GAHBCFG_DMA_EN | \
					 GAHBCFG_GLBL_INTR_EN)

#define GUSBCFG			(DWC_OTG_BASE + 0x00C)
#define GRSTCTL			(DWC_OTG_BASE + 0x010)
#define GRSTCTL_AHBIDLE		(1 << 31)
#define GRSTCTL_CSFTRST		(1 << 0)

#define GINTSTS			(DWC_OTG_BASE + 0x014)
#define GINTMSK			(DWC_OTG_BASE + 0x018)
#define GINTSTS_WKUPINT			(1 << 31)
#define GINTSTS_SESSREQINT		(1 << 30)
#define GINTSTS_DISCONNINT		(1 << 29)
#define GINTSTS_CONIDSTSCHNG		(1 << 28)
#define GINTSTS_LPMTRANRCVD		(1 << 27)
#define GINTSTS_PTXFEMP			(1 << 26)
#define GINTSTS_HCHINT			(1 << 25)
#define GINTSTS_PRTINT			(1 << 24)
#define GINTSTS_RESETDET		(1 << 23)
#define GINTSTS_FET_SUSP		(1 << 22)
#define GINTSTS_INCOMPL_IP		(1 << 21)
#define GINTSTS_INCOMPL_SOIN		(1 << 20)
#define GINTSTS_OEPINT			(1 << 19)
#define GINTSTS_IEPINT			(1 << 18)
#define GINTSTS_EPMIS			(1 << 17)
#define GINTSTS_RESTOREDONE		(1 << 16)
#define GINTSTS_EOPF			(1 << 15)
#define GINTSTS_ISOUTDROP		(1 << 14)
#define GINTSTS_ENUMDONE		(1 << 13)
#define GINTSTS_USBRST			(1 << 12)
#define GINTSTS_USBSUSP			(1 << 11)
#define GINTSTS_ERLYSUSP		(1 << 10)
#define GINTSTS_I2CINT			(1 << 9)
#define GINTSTS_ULPI_CK_INT		(1 << 8)
#define GINTSTS_GOUTNAKEFF		(1 << 7)
#define GINTSTS_GINNAKEFF		(1 << 6)
#define GINTSTS_NPTXFEMP		(1 << 5)
#define GINTSTS_RXFLVL			(1 << 4)
#define GINTSTS_SOF			(1 << 3)
#define GINTSTS_OTGINT			(1 << 2)
#define GINTSTS_MODEMIS			(1 << 1)
#define GINTSTS_CURMODE_HOST		(1 << 0)

#define GRXSTSR			(DWC_OTG_BASE + 0x01C)
#define GRXSTSP			(DWC_OTG_BASE + 0x020)
#define GRXFSIZ			(DWC_OTG_BASE + 0x024)
#define GNPTXFSIZ		(DWC_OTG_BASE + 0x028)
#define GNPTXSTS		(DWC_OTG_BASE + 0x02C)

#define GHWCFG1			(DWC_OTG_BASE + 0x044)
#define GHWCFG2			(DWC_OTG_BASE + 0x048)
#define GHWCFG3			(DWC_OTG_BASE + 0x04c)
#define GHWCFG4			(DWC_OTG_BASE + 0x050)
#define GLPMCFG			(DWC_OTG_BASE + 0x054)

#define GDFIFOCFG		(DWC_OTG_BASE + 0x05c)
#define GDFIFOCFG_EPINFOBASE_MASK	(0xffff << 16)
#define GDFIFOCFG_EPINFOBASE_SHIFT	16
#define GDFIFOCFG_GDFIFOCFG_MASK	(0xffff << 0)
#define GDFIFOCFG_GDFIFOCFG_SHIFT	0


#define HPTXFSIZ		(DWC_OTG_BASE + 0x100)
#define DIEPTXF(x)		(DWC_OTG_BASE + 0x100 + 4 * (x))
#define DIEPTXF1		(DWC_OTG_BASE + 0x104)
#define DIEPTXF2		(DWC_OTG_BASE + 0x108)
#define DIEPTXF3		(DWC_OTG_BASE + 0x10C)
#define DIEPTXF4		(DWC_OTG_BASE + 0x110)
#define DIEPTXF5		(DWC_OTG_BASE + 0x114)
#define DIEPTXF6		(DWC_OTG_BASE + 0x118)
#define DIEPTXF7		(DWC_OTG_BASE + 0x11C)
#define DIEPTXF8		(DWC_OTG_BASE + 0x120)
#define DIEPTXF9		(DWC_OTG_BASE + 0x124)
#define DIEPTXF10		(DWC_OTG_BASE + 0x128)
#define DIEPTXF11		(DWC_OTG_BASE + 0x12C)
#define DIEPTXF12		(DWC_OTG_BASE + 0x130)
#define DIEPTXF13		(DWC_OTG_BASE + 0x134)
#define DIEPTXF14		(DWC_OTG_BASE + 0x138)
#define DIEPTXF15		(DWC_OTG_BASE + 0x13C)

/*** HOST MODE REGISTERS ***/
/* Host Global Registers */
#define HCFG			(DWC_OTG_BASE + 0x400)
#define HFIR			(DWC_OTG_BASE + 0x404)
#define HFNUM			(DWC_OTG_BASE + 0x408)
#define HPTXSTS			(DWC_OTG_BASE + 0x410)
#define HAINT			(DWC_OTG_BASE + 0x414)
#define HAINTMSK		(DWC_OTG_BASE + 0x418)

/* Host Port Control and Status Registers */
#define HPRT			(DWC_OTG_BASE + 0x440)

/* Host Channel-Specific Registers */
#define HCCHAR(x)		(DWC_OTG_BASE + 0x500 + 0x20 * (x))
#define HCSPLT(x)		(DWC_OTG_BASE + 0x504 + 0x20 * (x))
#define HCINT(x)		(DWC_OTG_BASE + 0x508 + 0x20 * (x))
#define HCINTMSK(x)		(DWC_OTG_BASE + 0x50C + 0x20 * (x))
#define HCTSIZ(x)		(DWC_OTG_BASE + 0x510 + 0x20 * (x))
#define HCDMA(x)		(DWC_OTG_BASE + 0x514 + 0x20 * (x))
#define HCCHAR0			(DWC_OTG_BASE + 0x500)
#define HCSPLT0			(DWC_OTG_BASE + 0x504)
#define HCINT0			(DWC_OTG_BASE + 0x508)
#define HCINTMSK0		(DWC_OTG_BASE + 0x50C)
#define HCTSIZ0			(DWC_OTG_BASE + 0x510)
#define HCDMA0			(DWC_OTG_BASE + 0x514)
#define HCCHAR1			(DWC_OTG_BASE + 0x520)
#define HCSPLT1			(DWC_OTG_BASE + 0x524)
#define HCINT1			(DWC_OTG_BASE + 0x528)
#define HCINTMSK1		(DWC_OTG_BASE + 0x52C)
#define HCTSIZ1			(DWC_OTG_BASE + 0x530)
#define HCDMA1			(DWC_OTG_BASE + 0x534)
#define HCCHAR2			(DWC_OTG_BASE + 0x540)
#define HCSPLT2			(DWC_OTG_BASE + 0x544)
#define HCINT2			(DWC_OTG_BASE + 0x548)
#define HCINTMSK2		(DWC_OTG_BASE + 0x54C)
#define HCTSIZ2			(DWC_OTG_BASE + 0x550)
#define HCDMA2			(DWC_OTG_BASE + 0x554)
#define HCCHAR3			(DWC_OTG_BASE + 0x560)
#define HCSPLT3			(DWC_OTG_BASE + 0x564)
#define HCINT3			(DWC_OTG_BASE + 0x568)
#define HCINTMSK3   		(DWC_OTG_BASE + 0x56C)
#define HCTSIZ3     		(DWC_OTG_BASE + 0x570)
#define HCDMA3      		(DWC_OTG_BASE + 0x574)
#define HCCHAR4     		(DWC_OTG_BASE + 0x580)
#define HCSPLT4     		(DWC_OTG_BASE + 0x584)
#define HCINT4      		(DWC_OTG_BASE + 0x588)
#define HCINTMSK4   		(DWC_OTG_BASE + 0x58C)
#define HCTSIZ4     		(DWC_OTG_BASE + 0x590)
#define HCDMA4      		(DWC_OTG_BASE + 0x594)
#define HCCHAR5     		(DWC_OTG_BASE + 0x5A0)
#define HCSPLT5     		(DWC_OTG_BASE + 0x5A4)
#define HCINT5      		(DWC_OTG_BASE + 0x5A8)
#define HCINTMSK5   		(DWC_OTG_BASE + 0x5AC)
#define HCTSIZ5     		(DWC_OTG_BASE + 0x5B0)
#define HCDMA5      		(DWC_OTG_BASE + 0x5B4)
#define HCCHAR6     		(DWC_OTG_BASE + 0x5C0)
#define HCSPLT6     		(DWC_OTG_BASE + 0x5C4)
#define HCINT6      		(DWC_OTG_BASE + 0x5C8)
#define HCINTMSK6   		(DWC_OTG_BASE + 0x5CC)
#define HCTSIZ6     		(DWC_OTG_BASE + 0x5D0)
#define HCDMA6      		(DWC_OTG_BASE + 0x5D4)
#define HCCHAR7     		(DWC_OTG_BASE + 0x5E0)
#define HCSPLT7     		(DWC_OTG_BASE + 0x5E4)
#define HCINT7      		(DWC_OTG_BASE + 0x5E8)
#define HCINTMSK7   		(DWC_OTG_BASE + 0x5EC)
#define HCTSIZ7     		(DWC_OTG_BASE + 0x5F0)
#define HCDMA7      		(DWC_OTG_BASE + 0x5F4)
#define HCCHAR8     		(DWC_OTG_BASE + 0x600)
#define HCSPLT8     		(DWC_OTG_BASE + 0x604)
#define HCINT8      		(DWC_OTG_BASE + 0x608)
#define HCINTMSK8   		(DWC_OTG_BASE + 0x60C)
#define HCTSIZ8     		(DWC_OTG_BASE + 0x610)
#define HCDMA8      		(DWC_OTG_BASE + 0x614)
#define HCCHAR9     		(DWC_OTG_BASE + 0x620)
#define HCSPLT9     		(DWC_OTG_BASE + 0x624)
#define HCINT9      		(DWC_OTG_BASE + 0x628)
#define HCINTMSK9   		(DWC_OTG_BASE + 0x62C)
#define HCTSIZ9     		(DWC_OTG_BASE + 0x630)
#define HCDMA9      		(DWC_OTG_BASE + 0x634)
#define HCCHAR10    		(DWC_OTG_BASE + 0x640)
#define HCSPLT10    		(DWC_OTG_BASE + 0x644)
#define HCINT10     		(DWC_OTG_BASE + 0x648)
#define HCINTMSK10  		(DWC_OTG_BASE + 0x64C)
#define HCTSIZ10    		(DWC_OTG_BASE + 0x650)
#define HCDMA10     		(DWC_OTG_BASE + 0x654)
#define HCCHAR11    		(DWC_OTG_BASE + 0x660)
#define HCSPLT11    		(DWC_OTG_BASE + 0x664)
#define HCINT11     		(DWC_OTG_BASE + 0x668)
#define HCINTMSK11  		(DWC_OTG_BASE + 0x66C)
#define HCTSIZ11    		(DWC_OTG_BASE + 0x670)
#define HCDMA11     		(DWC_OTG_BASE + 0x674)
#define HCCHAR12    		(DWC_OTG_BASE + 0x680)
#define HCSPLT12    		(DWC_OTG_BASE + 0x684)
#define HCINT12     		(DWC_OTG_BASE + 0x688)
#define HCINTMSK12  		(DWC_OTG_BASE + 0x68C)
#define HCTSIZ12    		(DWC_OTG_BASE + 0x690)
#define HCDMA12     		(DWC_OTG_BASE + 0x694)
#define HCCHAR13    		(DWC_OTG_BASE + 0x6A0)
#define HCSPLT13    		(DWC_OTG_BASE + 0x6A4)
#define HCINT13     		(DWC_OTG_BASE + 0x6A8)
#define HCINTMSK13  		(DWC_OTG_BASE + 0x6AC)
#define HCTSIZ13    		(DWC_OTG_BASE + 0x6B0)
#define HCDMA13     		(DWC_OTG_BASE + 0x6B4)
#define HCCHAR14    		(DWC_OTG_BASE + 0x6C0)
#define HCSPLT14    		(DWC_OTG_BASE + 0x6C4)
#define HCINT14     		(DWC_OTG_BASE + 0x6C8)
#define HCINTMSK14  		(DWC_OTG_BASE + 0x6CC)
#define HCTSIZ14    		(DWC_OTG_BASE + 0x6D0)
#define HCDMA14     		(DWC_OTG_BASE + 0x6D4)
#define HCCHAR15    		(DWC_OTG_BASE + 0x6E0)
#define HCSPLT15    		(DWC_OTG_BASE + 0x6E4)
#define HCINT15     		(DWC_OTG_BASE + 0x6E8)
#define HCINTMSK15  		(DWC_OTG_BASE + 0x6EC)
#define HCTSIZ15    		(DWC_OTG_BASE + 0x6F0)
#define HCDMA15     		(DWC_OTG_BASE + 0x6F4)

/*** DEVICE MODE REGISTERS ***/
/* Device Global Registers */
#define DCFG        		(DWC_OTG_BASE + 0x800)
#define DCFG_EPMISCNT_MASK		(0x1f << 18)
#define DCFG_EPMISCNT_SHIFT		18
#define DCFG_NZ_STS_OUT_HSHK		(1 << 2)

#define DCTL        		(DWC_OTG_BASE + 0x804)
#define DSTS        		(DWC_OTG_BASE + 0x808)
#define DIEPMSK     		(DWC_OTG_BASE + 0x810)
#define DOEPMSK     		(DWC_OTG_BASE + 0x814)
#define DAINT       		(DWC_OTG_BASE + 0x818)
#define DAINTMSK    		(DWC_OTG_BASE + 0x81C)
#define DAINT_OUTEP_SHIFT		16
#define DAINT_OUTEP(_x)			(1 << ((_x) + 16))
#define DAINT_INEP(_x)			(1 << (_x))

#define DTKNQR1     		(DWC_OTG_BASE + 0x820)
#define DTKNQR2     		(DWC_OTG_BASE + 0x824)
#define DVBUSDIS    		(DWC_OTG_BASE + 0x828)
#define DVBUSPULSE  		(DWC_OTG_BASE + 0x82C)
#define DTHRCTL     		(DWC_OTG_BASE + 0x830)

/* Device Logical IN Endpoint-Specific Registers */
#define DIEPCTL(x)  		(DWC_OTG_BASE + 0x900 + 0x20 * (x))
#define DIEPINT(x)  		(DWC_OTG_BASE + 0x908 + 0x20 * (x))
#define DIEPTSIZ(x) 		(DWC_OTG_BASE + 0x910 + 0x20 * (x))
#define DIEPDMA(x)  		(DWC_OTG_BASE + 0x914 + 0x20 * (x))
#define DTXFSTS(x)  		(DWC_OTG_BASE + 0x918 + 0x20 * (x))

#define DIEPCTL0    		(DWC_OTG_BASE + 0x900)
#define DIEPINT0    		(DWC_OTG_BASE + 0x908)
#define DIEPTSIZ0   		(DWC_OTG_BASE + 0x910)
#define DIEPDMA0    		(DWC_OTG_BASE + 0x914)
#define DIEPCTL1    		(DWC_OTG_BASE + 0x920)
#define DIEPINT1    		(DWC_OTG_BASE + 0x928)
#define DIEPTSIZ1   		(DWC_OTG_BASE + 0x930)
#define DIEPDMA1    		(DWC_OTG_BASE + 0x934)
#define DIEPCTL2    		(DWC_OTG_BASE + 0x940)
#define DIEPINT2    		(DWC_OTG_BASE + 0x948)
#define DIEPTSIZ2  		(DWC_OTG_BASE + 0x950)
#define DIEPDMA2    		(DWC_OTG_BASE + 0x954)
#define DIEPCTL3    		(DWC_OTG_BASE + 0x960)
#define DIEPINT3    		(DWC_OTG_BASE + 0x968)
#define DIEPTSIZ3   		(DWC_OTG_BASE + 0x970)
#define DIEPDMA3    		(DWC_OTG_BASE + 0x974)
#define DIEPCTL4    		(DWC_OTG_BASE + 0x980)
#define DIEPINT4    		(DWC_OTG_BASE + 0x988)
#define DIEPTSIZ4   		(DWC_OTG_BASE + 0x990)
#define DIEPDMA4    		(DWC_OTG_BASE + 0x994)
#define DIEPCTL5    		(DWC_OTG_BASE + 0x9A0)
#define DIEPINT5    		(DWC_OTG_BASE + 0x9A8)
#define DIEPTSIZ5   		(DWC_OTG_BASE + 0x9B0)
#define DIEPDMA5    		(DWC_OTG_BASE + 0x9B4)
#define DIEPCTL6    		(DWC_OTG_BASE + 0x9C0)
#define DIEPINT6    		(DWC_OTG_BASE + 0x9C8)
#define DIEPTSIZ6   		(DWC_OTG_BASE + 0x9D0)
#define DIEPDMA6    		(DWC_OTG_BASE + 0x9D4)
#define DIEPCTL7    		(DWC_OTG_BASE + 0x9E0)
#define DIEPINT7    		(DWC_OTG_BASE + 0x9E8)
#define DIEPTSIZ7   		(DWC_OTG_BASE + 0x9F0)
#define DIEPDMA7    		(DWC_OTG_BASE + 0x9F4)
#define DIEPCTL8    		(DWC_OTG_BASE + 0xA00)
#define DIEPINT8    		(DWC_OTG_BASE + 0xA08)
#define DIEPTSIZ8   		(DWC_OTG_BASE + 0xA10)
#define DIEPDMA8    		(DWC_OTG_BASE + 0xA14)
#define DIEPCTL9    		(DWC_OTG_BASE + 0xA20)
#define DIEPINT9    		(DWC_OTG_BASE + 0xA28)
#define DIEPTSIZ9   		(DWC_OTG_BASE + 0xA30)
#define DIEPDMA9    		(DWC_OTG_BASE + 0xA34)
#define DIEPCTL10   		(DWC_OTG_BASE + 0xA40)
#define DIEPINT10   		(DWC_OTG_BASE + 0xA48)
#define DIEPTSIZ10  		(DWC_OTG_BASE + 0xA50)
#define DIEPDMA10   		(DWC_OTG_BASE + 0xA54)
#define DIEPCTL11   		(DWC_OTG_BASE + 0xA60)
#define DIEPINT11   		(DWC_OTG_BASE + 0xA68)
#define DIEPTSIZ11  		(DWC_OTG_BASE + 0xA70)
#define DIEPDMA11   		(DWC_OTG_BASE + 0xA74)
#define DIEPCTL12   		(DWC_OTG_BASE + 0xA80)
#define DIEPINT12   		(DWC_OTG_BASE + 0xA88)
#define DIEPTSIZ12  		(DWC_OTG_BASE + 0xA90)
#define DIEPDMA12   		(DWC_OTG_BASE + 0xA94)
#define DIEPCTL13   		(DWC_OTG_BASE + 0xAA0)
#define DIEPINT13   		(DWC_OTG_BASE + 0xAA8)
#define DIEPTSIZ13  		(DWC_OTG_BASE + 0xAB0)
#define DIEPDMA13   		(DWC_OTG_BASE + 0xAB4)
#define DIEPCTL14   		(DWC_OTG_BASE + 0xAC0)
#define DIEPINT14   		(DWC_OTG_BASE + 0xAC8)
#define DIEPTSIZ14  		(DWC_OTG_BASE + 0xAD0)
#define DIEPDMA14   		(DWC_OTG_BASE + 0xAD4)
#define DIEPCTL15   		(DWC_OTG_BASE + 0xAE0)
#define DIEPINT15   		(DWC_OTG_BASE + 0xAE8)
#define DIEPTSIZ15  		(DWC_OTG_BASE + 0xAF0)
#define DIEPDMA15   		(DWC_OTG_BASE + 0xAF4)

/* Device Logical OUT Endpoint-Specific Registers */
#define DOEPCTL(x)  		(DWC_OTG_BASE + 0xB00 + 0x20 * (x))
#define DXEPCTL_EPENA			(1 << 31)
#define DXEPCTL_EPDIS			(1 << 30)
#define DXEPCTL_SETD1PID		(1 << 29)
#define DXEPCTL_SETODDFR		(1 << 29)
#define DXEPCTL_SETD0PID		(1 << 28)
#define DXEPCTL_SETEVENFR		(1 << 28)
#define DXEPCTL_SNAK			(1 << 27)
#define DXEPCTL_CNAK			(1 << 26)
#define DXEPCTL_NAKSTS			(1 << 17)
#define DXEPCTL_DPID			(1 << 16)
#define DXEPCTL_EOFRNUM			(1 << 16)
#define DXEPCTL_USBACTEP		(1 << 15)
#define DXEPCTL_NEXTEP_MASK		(0xf << 11)
#define DXEPCTL_NEXTEP_SHIFT		11
#define DXEPCTL_NEXTEP_LIMIT		0xf
#define DXEPCTL_NEXTEP(_x)		((_x) << 11)


#define DOEPINT(x)  		(DWC_OTG_BASE + 0xB08 + 0x20 * (x))
#define DXEPINT_INEPNAKEFF              (1 << 6)
#define DXEPINT_BACK2BACKSETUP          (1 << 6)
#define DXEPINT_INTKNEPMIS              (1 << 5)
#define DXEPINT_INTKNTXFEMP             (1 << 4)
#define DXEPINT_OUTTKNEPDIS             (1 << 4)
#define DXEPINT_TIMEOUT                 (1 << 3)
#define DXEPINT_SETUP                   (1 << 3)
#define DXEPINT_AHBERR                  (1 << 2)
#define DXEPINT_EPDISBLD                (1 << 1)
#define DXEPINT_XFERCOMPL               (1 << 0)

#define DOEPTSIZ(x) 		(DWC_OTG_BASE + 0xB10 + 0x20 * (x))
#define DXEPTSIZ_MC_MASK		(0x3 << 29)
#define DXEPTSIZ_MC_SHIFT		29
#define DXEPTSIZ_MC_LIMIT		0x3
#define DXEPTSIZ_MC(_x)			((_x) << 29)
#define DXEPTSIZ_PKTCNT_MASK		(0x3ff << 19)
#define DXEPTSIZ_PKTCNT_SHIFT		19
#define DXEPTSIZ_PKTCNT_LIMIT		0x3ff
#define DXEPTSIZ_PKTCNT_GET(_v)		(((_v) >> 19) & 0x3ff)
#define DXEPTSIZ_PKTCNT(_x)		((_x) << 19)
#define DXEPTSIZ_XFERSIZE_MASK		(0x7ffff << 0)
#define DXEPTSIZ_XFERSIZE_SHIFT		0
#define DXEPTSIZ_XFERSIZE_LIMIT		0x7ffff
#define DXEPTSIZ_XFERSIZE_GET(_v)	(((_v) >> 0) & 0x7ffff)
#define DXEPTSIZ_XFERSIZE(_x)		((_x) << 0)

#define DOEPDMA(x)  		(DWC_OTG_BASE + 0xB14 + 0x20 * (x))
#define DOEPCTL0    		(DWC_OTG_BASE + 0xB00)
#define DOEPINT0    		(DWC_OTG_BASE + 0xB08)
#define DOEPTSIZ0   		(DWC_OTG_BASE + 0xB10)
#define DOEPTSIZ0_SUPCNT_MASK		(0x3 << 29)
#define DOEPTSIZ0_SUPCNT_SHIFT		29
#define DOEPTSIZ0_SUPCNT_LIMIT		0x3
#define DOEPTSIZ0_SUPCNT(_x)		((_x) << 29)
#define DOEPTSIZ0_PKTCNT		(1 << 19)
#define DOEPTSIZ0_XFERSIZE_MASK		(0x7f << 0)
#define DOEPTSIZ0_XFERSIZE_SHIFT	0

#define DOEPDMA0    		(DWC_OTG_BASE + 0xB14)
#define DOEPCTL1    		(DWC_OTG_BASE + 0xB20)
#define DOEPINT1    		(DWC_OTG_BASE + 0xB28)
#define DOEPTSIZ1   		(DWC_OTG_BASE + 0xB30)
#define DOEPDMA1    		(DWC_OTG_BASE + 0xB34)
#define DOEPCTL2    		(DWC_OTG_BASE + 0xB40)
#define DOEPINT2    		(DWC_OTG_BASE + 0xB48)
#define DOEPTSIZ2   		(DWC_OTG_BASE + 0xB50)
#define DOEPDMA2    		(DWC_OTG_BASE + 0xB54)
#define DOEPCTL3    		(DWC_OTG_BASE + 0xB60)
#define DOEPINT3    		(DWC_OTG_BASE + 0xB68)
#define DOEPTSIZ3   		(DWC_OTG_BASE + 0xB70)
#define DOEPDMA3    		(DWC_OTG_BASE + 0xB74)
#define DOEPCTL4    		(DWC_OTG_BASE + 0xB80)
#define DOEPINT4    		(DWC_OTG_BASE + 0xB88)
#define DOEPTSIZ4   		(DWC_OTG_BASE + 0xB90)
#define DOEPDMA4    		(DWC_OTG_BASE + 0xB94)
#define DOEPCTL5    		(DWC_OTG_BASE + 0xBA0)
#define DOEPINT5    		(DWC_OTG_BASE + 0xBA8)
#define DOEPTSIZ5   		(DWC_OTG_BASE + 0xBB0)
#define DOEPDMA5    		(DWC_OTG_BASE + 0xBB4)
#define DOEPCTL6    		(DWC_OTG_BASE + 0xBC0)
#define DOEPINT6    		(DWC_OTG_BASE + 0xBC8)
#define DOEPTSIZ6   		(DWC_OTG_BASE + 0xBD0)
#define DOEPDMA6    		(DWC_OTG_BASE + 0xBD4)
#define DOEPCTL7    		(DWC_OTG_BASE + 0xBE0)
#define DOEPINT7    		(DWC_OTG_BASE + 0xBE8)
#define DOEPTSIZ7   		(DWC_OTG_BASE + 0xBF0)
#define DOEPDMA7    		(DWC_OTG_BASE + 0xBF4)
#define DOEPCTL8    		(DWC_OTG_BASE + 0xC00)
#define DOEPINT8    		(DWC_OTG_BASE + 0xC08)
#define DOEPTSIZ8   		(DWC_OTG_BASE + 0xC10)
#define DOEPDMA8    		(DWC_OTG_BASE + 0xC14)
#define DOEPCTL9    		(DWC_OTG_BASE + 0xC20)
#define DOEPINT9    		(DWC_OTG_BASE + 0xC28)
#define DOEPTSIZ9   		(DWC_OTG_BASE + 0xC30)
#define DOEPDMA9    		(DWC_OTG_BASE + 0xC34)
#define DOEPCTL10   		(DWC_OTG_BASE + 0xC40)
#define DOEPINT10   		(DWC_OTG_BASE + 0xC48)
#define DOEPTSIZ10  		(DWC_OTG_BASE + 0xC50)
#define DOEPDMA10   		(DWC_OTG_BASE + 0xC54)
#define DOEPCTL11   		(DWC_OTG_BASE + 0xC60)
#define DOEPINT11   		(DWC_OTG_BASE + 0xC68)
#define DOEPTSIZ11  		(DWC_OTG_BASE + 0xC70)
#define DOEPDMA11   		(DWC_OTG_BASE + 0xC74)
#define DOEPCTL12   		(DWC_OTG_BASE + 0xC80)
#define DOEPINT12   		(DWC_OTG_BASE + 0xC88)
#define DOEPTSIZ12  		(DWC_OTG_BASE + 0xC90)
#define DOEPDMA12   		(DWC_OTG_BASE + 0xC94)
#define DOEPCTL13   		(DWC_OTG_BASE + 0xCA0)
#define DOEPINT13   		(DWC_OTG_BASE + 0xCA8)
#define DOEPTSIZ13  		(DWC_OTG_BASE + 0xCB0)
#define DOEPDMA13   		(DWC_OTG_BASE + 0xCB4)
#define DOEPCTL14   		(DWC_OTG_BASE + 0xCC0)
#define DOEPINT14   		(DWC_OTG_BASE + 0xCC8)
#define DOEPTSIZ14  		(DWC_OTG_BASE + 0xCD0)
#define DOEPDMA14   		(DWC_OTG_BASE + 0xCD4)
#define DOEPCTL15   		(DWC_OTG_BASE + 0xCE0)
#define DOEPINT15   		(DWC_OTG_BASE + 0xCE8)
#define DOEPTSIZ15  		(DWC_OTG_BASE + 0xCF0)
#define DOEPDMA15   		(DWC_OTG_BASE + 0xCF4)

/* Power and Clock Gating Register */
#define PCGCCTL			(DWC_OTG_BASE + 0xE00)

#define EP0FIFO			(DWC_OTG_BASE + 0x1000)

#define PERI_CTRL16_PICOPHY_SIDDQ_BIT		(1<<0)
#define PERI_CTRL16_PICOPHY_TXPREEMPHASISTUNE	(1<<31)
#define PERI_CTRL15_HSICPHY_SIDDQ_BIT		(1<<16)
#define PERI_CTRL14_NANOPHY_SIDDQ_BIT		(1<<0)
#define PERI_CTRL0_USB2DVC_NANOPHY_BIT		(1<<7)
#define NANOPHY_DMPULLDOWN    (1 << 6)    /* bit[6]：nanophy_dmpulldown；为1'b0 */
#define NANOPHY_DPPULLDOWN    (1 << 5)    /* bit[5]：nanophy_dppulldown；为1'b0 */

#define EN_LDO4_INT (1 << 4)
#define EN_LDO8_INT (1 << 4)

/* SCPEREN1/DIS1 */
#define GT_CLK_USBHSICPHY480            (1<<26)
#define GT_CLK_USBHSICPHY               (1<<25)
#define GT_CLK_USBPICOPHY               (1<<24)
#define GT_CLK_USBNANOPHY               (1<<23)
/* SCPEREN3/DIS3 */
#define GT_CLK_USB2HST                  (1<<18)
#define GT_CLK_USB2DVC                  (1<<17)
/* SCPERRSTEN3 */
#define IP_RST_PICOPHY_POR              (1<<31)
#define IP_RST_HSICPHY_POR              (1<<30)
#define IP_RST_NANOPHY_POR              (1<<29)
#define IP_RST_USB2DVC_PHY              (1<<28)
#define IP_RST_USB2H_UTMI1              (1<<21)
#define IP_RST_USB2H_UTMI0              (1<<20)
#define IP_RST_USB2H_PHY                (1<<19)
#define IP_RST_USB2HST                  (1<<18)
#define IP_RST_USB2DVC                  (1<<17)
/* SCPERRSTEN1 */
#define IP_RST_HSICPHY                  (1<<25)
#define IP_RST_PICOPHY                  (1<<24)
#define IP_RST_NANOPHY                  (1<<23)

/*
 * USB directions
 *
 * This bit flag is used in endpoint descriptors' bEndpointAddress field.
 * It's also one of three fields in control requests bRequestType.
 */
#define USB_DIR_OUT                     0               /* to device */
#define USB_DIR_IN                      0x80            /* to host */

/*
 * Descriptor types ... USB 2.0 spec table 9.5
 */
#define USB_DT_DEVICE                   0x01
#define USB_DT_CONFIG                   0x02
#define USB_DT_STRING                   0x03
#define USB_DT_INTERFACE                0x04
#define USB_DT_ENDPOINT                 0x05
#define USB_DT_DEVICE_QUALIFIER         0x06
#define USB_DT_OTHER_SPEED_CONFIG       0x07
#define USB_DT_INTERFACE_POWER          0x08
/* these are from a minor usb 2.0 revision (ECN) */
#define USB_DT_OTG                      0x09
#define USB_DT_DEBUG                    0x0a
#define USB_DT_INTERFACE_ASSOCIATION    0x0b
/* these are from the Wireless USB spec */
#define USB_DT_SECURITY                 0x0c
#define USB_DT_KEY                      0x0d
#define USB_DT_ENCRYPTION_TYPE          0x0e
#define USB_DT_BOS                      0x0f
#define USB_DT_DEVICE_CAPABILITY        0x10
#define USB_DT_WIRELESS_ENDPOINT_COMP   0x11
#define USB_DT_WIRE_ADAPTER             0x21
#define USB_DT_RPIPE                    0x22
#define USB_DT_CS_RADIO_CONTROL         0x23

/*
 * USB recipients, the third of three bRequestType fields
 */
#define USB_RECIP_MASK                  0x1f
#define USB_RECIP_DEVICE                0x00
#define USB_RECIP_INTERFACE             0x01
#define USB_RECIP_ENDPOINT              0x02
#define USB_RECIP_OTHER                 0x03

/* IN/OUT will STALL */
#define USB_ENDPOINT_HALT      			0

/*
 * Endpoints
 */
#define USB_ENDPOINT_NUMBER_MASK        0x0f    /* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK           0x80

#define USB_ENDPOINT_XFERTYPE_MASK      0x03    /* in bmAttributes */
#define USB_ENDPOINT_XFER_CONTROL       0
#define USB_ENDPOINT_XFER_ISOC          1
#define USB_ENDPOINT_XFER_BULK          2
#define USB_ENDPOINT_XFER_INT           3
#define USB_ENDPOINT_MAX_ADJUSTABLE     0x80

/*
 * Standard requests, for the bRequest field of a SETUP packet.
 *
 * These are qualified by the bRequestType field, so that for example
 * TYPE_CLASS or TYPE_VENDOR specific feature flags could be retrieved
 * by a GET_STATUS request.
 */
#define USB_REQ_GET_STATUS              0x00
#define USB_REQ_CLEAR_FEATURE           0x01
#define USB_REQ_SET_FEATURE             0x03
#define USB_REQ_SET_ADDRESS             0x05
#define USB_REQ_GET_DESCRIPTOR          0x06
#define USB_REQ_SET_DESCRIPTOR          0x07
#define USB_REQ_GET_CONFIGURATION       0x08
#define USB_REQ_SET_CONFIGURATION       0x09
#define USB_REQ_GET_INTERFACE           0x0A
#define USB_REQ_SET_INTERFACE           0x0B
#define USB_REQ_SYNCH_FRAME             0x0C

/* USB_DT_DEVICE: Device descriptor */
struct usb_device_descriptor {
        unsigned char  bLength;
        unsigned char  bDescriptorType;

        unsigned short bcdUSB;
        unsigned char  bDeviceClass;
        unsigned char  bDeviceSubClass;
        unsigned char  bDeviceProtocol;
        unsigned char  bMaxPacketSize0;
        unsigned short idVendor;
        unsigned short idProduct;
        unsigned short bcdDevice;
        unsigned char  iManufacturer;
        unsigned char  iProduct;
        unsigned char  iSerialNumber;
        unsigned char  bNumConfigurations;
} __attribute__ ((packed));

#define USB_DT_DEVICE_SIZE              18

/*
 * Device and/or Interface Class codes
 * as found in bDeviceClass or bInterfaceClass
 * and defined by www.usb.org documents
 */
#define USB_CLASS_PER_INTERFACE         0       /* for DeviceClass */
#define USB_CLASS_AUDIO                 1
#define USB_CLASS_COMM                  2
#define USB_CLASS_HID                   3
#define USB_CLASS_PHYSICAL              5
#define USB_CLASS_STILL_IMAGE           6
#define USB_CLASS_PRINTER               7
#define USB_CLASS_MASS_STORAGE          8
#define USB_CLASS_HUB                   9
#define USB_CLASS_CDC_DATA              0x0a
#define USB_CLASS_CSCID                 0x0b    /* chip+ smart card */
#define USB_CLASS_CONTENT_SEC           0x0d    /* content security */
#define USB_CLASS_VIDEO                 0x0e
#define USB_CLASS_WIRELESS_CONTROLLER   0xe0
#define USB_CLASS_MISC                  0xef
#define USB_CLASS_APP_SPEC              0xfe
#define USB_CLASS_VENDOR_SPEC           0xff

/*-------------------------------------------------------------------------*/

/* USB_DT_CONFIG: Configuration descriptor information.
 *
 * USB_DT_OTHER_SPEED_CONFIG is the same descriptor, except that the
 * descriptor type is different.  Highspeed-capable devices can look
 * different depending on what speed they're currently running.  Only
 * devices with a USB_DT_DEVICE_QUALIFIER have any OTHER_SPEED_CONFIG
 * descriptors.
 */
struct usb_config_descriptor {
       unsigned char  bLength;
       unsigned char  bDescriptorType;

       unsigned short wTotalLength;
       unsigned char  bNumInterfaces;
       unsigned char  bConfigurationValue;
       unsigned char  iConfiguration;
       unsigned char  bmAttributes;
       unsigned char  bMaxPower;
} __attribute__((packed));

#define USB_DT_CONFIG_SIZE              9

/* from config descriptor bmAttributes */
#define USB_CONFIG_ATT_ONE              (1 << 7)        /* must be set */
#define USB_CONFIG_ATT_SELFPOWER        (1 << 6)        /* self powered */
#define USB_CONFIG_ATT_WAKEUP           (1 << 5)        /* can wakeup */
#define USB_CONFIG_ATT_BATTERY          (1 << 4)        /* battery powered */

/*-------------------------------------------------------------------------*/

/* USB_DT_STRING: String descriptor */
struct usb_string_descriptor {
        unsigned char  bLength;
        unsigned char  bDescriptorType;

        unsigned short wString[16];             /* UTF-16LE encoded */
} __attribute__((packed));

/*-------------------------------------------------------------------------*/
/* USB_DT_INTERFACE: Interface descriptor */
struct usb_interface_descriptor {
        unsigned char  bLength;
        unsigned char  bDescriptorType;

        unsigned char  bInterfaceNumber;
        unsigned char  bAlternateSetting;
        unsigned char  bNumEndpoints;
        unsigned char  bInterfaceClass;
        unsigned char  bInterfaceSubClass;
        unsigned char  bInterfaceProtocol;
        unsigned char  iInterface;
};

#define USB_DT_INTERFACE_SIZE           9

/*-------------------------------------------------------------------------*/

/* USB_DT_ENDPOINT: Endpoint descriptor */
struct usb_endpoint_descriptor {
        unsigned char  bLength;
        unsigned char  bDescriptorType;

        unsigned char  bEndpointAddress;
        unsigned char  bmAttributes;
        unsigned short wMaxPacketSize;
        unsigned char  bInterval;
} __attribute__ ((packed));

#define USB_DT_ENDPOINT_SIZE            7
#define USB_DT_ENDPOINT_AUDIO_SIZE      9       /* Audio extension */

extern int usb_need_reset;

/**
 * This union represents the bit fields in the DMA Descriptor
 * status quadlet. Read the quadlet into the <i>d32</i> member then
 * set/clear the bits using the <i>b</i>it, <i>b_iso_out</i> and
 * <i>b_iso_in</i> elements.
 */
typedef union dev_dma_desc_sts {
		/** raw register data */
	unsigned int d32;
		/** quadlet bits */
	struct {
		/** Received number of bytes */
		unsigned bytes:16;
		/** NAK bit - only for OUT EPs */
		unsigned nak:1;
		unsigned reserved17_22:6;
		/** Multiple Transfer - only for OUT EPs */
		unsigned mtrf:1;
		/** Setup Packet received - only for OUT EPs */
		unsigned sr:1;
		/** Interrupt On Complete */
		unsigned ioc:1;
		/** Short Packet */
		unsigned sp:1;
		/** Last */
		unsigned l:1;
		/** Receive Status */
		unsigned sts:2;
		/** Buffer Status */
		unsigned bs:2;
	} b;

//#ifdef DWC_EN_ISOC
		/** iso out quadlet bits */
	struct {
		/** Received number of bytes */
		unsigned rxbytes:11;

		unsigned reserved11:1;
		/** Frame Number */
		unsigned framenum:11;
		/** Received ISO Data PID */
		unsigned pid:2;
		/** Interrupt On Complete */
		unsigned ioc:1;
		/** Short Packet */
		unsigned sp:1;
		/** Last */
		unsigned l:1;
		/** Receive Status */
		unsigned rxsts:2;
		/** Buffer Status */
		unsigned bs:2;
	} b_iso_out;

		/** iso in quadlet bits */
	struct {
		/** Transmited number of bytes */
		unsigned txbytes:12;
		/** Frame Number */
		unsigned framenum:11;
		/** Transmited ISO Data PID */
		unsigned pid:2;
		/** Interrupt On Complete */
		unsigned ioc:1;
		/** Short Packet */
		unsigned sp:1;
		/** Last */
		unsigned l:1;
		/** Transmit Status */
		unsigned txsts:2;
		/** Buffer Status */
		unsigned bs:2;
	} b_iso_in;
//#endif                                /* DWC_EN_ISOC */
} dev_dma_desc_sts_t;

/**
 * DMA Descriptor structure
 *
 * DMA Descriptor structure contains two quadlets:
 * Status quadlet and Data buffer pointer.
 */
typedef struct dwc_otg_dev_dma_desc {
	/** DMA Descriptor status quadlet */
	dev_dma_desc_sts_t status;
	/** DMA Descriptor data buffer pointer */
	unsigned int buf;
} dwc_otg_dev_dma_desc_t;

extern void usb_reinit(void);

#endif	/* __DWC_USB_H__*/
