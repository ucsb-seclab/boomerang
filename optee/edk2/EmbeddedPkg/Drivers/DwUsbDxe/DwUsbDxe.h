/** @file

  Copyright (c) 2015, Linaro Limited. All rights reserved.
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DW_USB_DXE_H__
#define __DW_USB_DXE_H__

#define DW_USB_BASE FixedPcdGet32 (PcdDwUsbBaseAddress)
#define USB_PHY_BASE FixedPcdGet32 (PcdSysCtrlBaseAddress)

#define SC_PERIPH_CTRL4			0x00c

#define CTRL4_PICO_SIDDQ		BIT6
#define CTRL4_PICO_OGDISABLE		BIT8
#define CTRL4_PICO_VBUSVLDEXT		BIT10
#define CTRL4_PICO_VBUSVLDEXTSEL	BIT11
#define CTRL4_OTG_PHY_SEL		BIT21

#define SC_PERIPH_CTRL5			0x010

#define CTRL5_USBOTG_RES_SEL		BIT3
#define CTRL5_PICOPHY_ACAENB		BIT4
#define CTRL5_PICOPHY_BC_MODE		BIT5
#define CTRL5_PICOPHY_CHRGSEL		BIT6
#define CTRL5_PICOPHY_VDATSRCEND	BIT7
#define CTRL5_PICOPHY_VDATDETENB	BIT8
#define CTRL5_PICOPHY_DCDENB		BIT9
#define CTRL5_PICOPHY_IDDIG		BIT10

#define SC_PERIPH_CTRL8			0x018
#define SC_PERIPH_RSTEN0		0x300
#define SC_PERIPH_RSTDIS0		0x304

#define RST0_USBOTG_BUS			BIT4
#define RST0_POR_PICOPHY		BIT5
#define RST0_USBOTG			BIT6
#define RST0_USBOTG_32K			BIT7

#define EYE_PATTERN_PARA		0x7053348c

#define PHY_READ_REG32(Offset) MmioRead32 (USB_PHY_BASE + Offset)
#define PHY_WRITE_REG32(Offset, Val)  MmioWrite32 (USB_PHY_BASE + Offset, Val)


#define READ_REG32(Offset) MmioRead32 (DW_USB_BASE + Offset)
#define READ_REG16(Offset) (UINT16) READ_REG32 (Offset)
#define WRITE_REG32(Offset, Val)  MmioWrite32 (DW_USB_BASE + Offset, Val)
#define WRITE_REG16(Offset, Val)  MmioWrite32 (DW_USB_BASE + Offset, (UINT32) Val)
#define WRITE_REG8(Offset, Val)   MmioWrite32 (DW_USB_BASE + Offset, (UINT32) Val)

// Max packet size in bytes (For Full Speed USB 64 is the only valid value)
#define MAX_PACKET_SIZE_CONTROL     64

#define MAX_PACKET_SIZE_BULK        512

// 8 Endpoints, in and out. Don't count the Endpoint 0 setup buffer
#define DW_NUM_ENDPOINTS               16

// Endpoint Indexes
#define DW_EP0SETUP                    0x20
#define DW_EP0RX                       0x00
#define DW_EP0TX                       0x01
#define DW_EP1RX                       0x02
#define DW_EP1TX                       0x03

// DcInterrupt bits
#define DW_DC_INTERRUPT_BRESET         BIT0
#define DW_DC_INTERRUPT_SOF            BIT1
#define DW_DC_INTERRUPT_PSOF           BIT2
#define DW_DC_INTERRUPT_SUSP           BIT3
#define DW_DC_INTERRUPT_RESUME         BIT4
#define DW_DC_INTERRUPT_HS_STAT        BIT5
#define DW_DC_INTERRUPT_DMA            BIT6
#define DW_DC_INTERRUPT_VBUS           BIT7
#define DW_DC_INTERRUPT_EP0SETUP       BIT8
#define DW_DC_INTERRUPT_EP0RX          BIT10
#define DW_DC_INTERRUPT_EP0TX          BIT11
#define DW_DC_INTERRUPT_EP1RX          BIT12
#define DW_DC_INTERRUPT_EP1TX          BIT13
// All valid peripheral controller interrupts
#define DW_DC_INTERRUPT_MASK           0x003FFFDFF

#define DW_ADDRESS                     0x200
#define DW_ADDRESS_DEVEN               BIT7

#define DW_MODE                        0x20C
#define DW_MODE_DATA_BUS_WIDTH         BIT8
#define DW_MODE_CLKAON                 BIT7
#define DW_MODE_SFRESET                BIT4
#define DW_MODE_WKUPCS                 BIT2

#define DW_ENDPOINT_MAX_PACKET_SIZE    0x204

#define DW_ENDPOINT_TYPE               0x208
#define DW_ENDPOINT_TYPE_NOEMPKT       BIT4
#define DW_ENDPOINT_TYPE_ENABLE        BIT3

#define DW_INTERRUPT_CONFIG            0x210
// Interrupt config value to only interrupt on ACK of IN and OUT tokens
#define DW_INTERRUPT_CONFIG_ACK_ONLY   BIT2 | BIT5 | BIT6

#define DW_DC_INTERRUPT                0x218
#define DW_DC_INTERRUPT_ENABLE         0x214

#define DW_CTRL_FUNCTION               0x228
#define DW_CTRL_FUNCTION_VENDP         BIT3
#define DW_CTRL_FUNCTION_DSEN          BIT2
#define DW_CTRL_FUNCTION_STATUS        BIT1

#define DW_DEVICE_UNLOCK               0x27C
#define DW_DEVICE_UNLOCK_MAGIC         0xAA37

#define DW_SW_RESET_REG                0x30C
#define DW_SW_RESET_ALL                BIT0

#define DW_DEVICE_ID                   0x370

#define DW_OTG_CTRL_SET                0x374
#define DW_OTG_CTRL_CLR                OTG_CTRL_SET + 2
#define DW_OTG_CTRL_OTG_DISABLE        BIT10
#define DW_OTG_CTRL_VBUS_CHRG          BIT6
#define DW_OTG_CTRL_VBUS_DISCHRG       BIT5
#define DW_OTG_CTRL_DM_PULLDOWN        BIT2
#define DW_OTG_CTRL_DP_PULLDOWN        BIT1
#define DW_OTG_CTRL_DP_PULLUP          BIT0

#define DW_OTG_STATUS                  0x378
#define DW_OTG_STATUS_B_SESS_END       BIT7
#define DW_OTG_STATUS_A_B_SESS_VLD     BIT1

#define DW_OTG_INTERRUPT_LATCH_SET     0x37C
#define DW_OTG_INTERRUPT_LATCH_CLR     0x37E
#define DW_OTG_INTERRUPT_ENABLE_RISE   0x384

#define DW_DMA_ENDPOINT_INDEX          0x258

#define DW_ENDPOINT_INDEX              0x22c
#define DW_DATA_PORT                   0x220
#define DW_BUFFER_LENGTH               0x21c

// Device ID Values
#define PHILLIPS_VENDOR_ID_VAL 0x04cc
#define DW_PRODUCT_ID_VAL 0x1761
#define DW_DEVICE_ID_VAL ((ISP1761_PRODUCT_ID_VAL << 16) |\
                               PHILLIPS_VENDOR_ID_VAL)

#define DWC_OTG_BASE		       DW_USB_BASE

#define USB_NUM_ENDPOINTS          2
#define MAX_EPS_CHANNELS 		   16

#define BULK_OUT_EP 			   1
#define BULK_IN_EP				   1

#define RX_REQ_LEN 				   512
#define MAX_PACKET_LEN 			   512

#define DATA_FIFO_CONFIG 		   0x0F801000
/* RX FIFO: 2048 bytes */
#define RX_SIZE          		   0x00000200
/* Non-periodic TX FIFO: 128 bytes. start address: 0x200 * 4. */
#define ENDPOINT_TX_SIZE 		   0x01000200

/* EP1  TX FIFO: 1024 bytes. start address: 0x300 * 4. */
/* EP2  TX FIFO: 1024 bytes. start address: 0x400 * 4. */
/* EP3  TX FIFO: 1024 bytes. start address: 0x500 * 4. */
/* EP4  TX FIFO: 1024 bytes. start address: 0x600 * 4. */
/* EP5  TX FIFO: 1024 bytes. start address: 0x700 * 4. */
/* EP6  TX FIFO: 1024 bytes. start address: 0x800 * 4. */
/* EP7  TX FIFO: 1024 bytes. start address: 0x900 * 4. */
/* EP8  TX FIFO: 1024 bytes. start address: 0xA00 * 4. */
/* EP9  TX FIFO: 1024 bytes. start address: 0xB00 * 4. */
/* EP10 TX FIFO: 1024 bytes. start address: 0xC00 * 4. */
/* EP11 TX FIFO: 512  bytes. start address: 0xD00 * 4. */
/* EP12 TX FIFO: 512  bytes. start address: 0xD80 * 4. */
/* EP13 TX FIFO: 512  bytes. start address: 0xE00 * 4. */
/* EP14 TX FIFO: 512  bytes. start address: 0xE80 * 4. */
/* EP15 TX FIFO: 512  bytes. start address: 0xF00 * 4. */

#define DATA_IN_ENDPOINT_TX_FIFO1  0x01000300
#define DATA_IN_ENDPOINT_TX_FIFO2  0x01000400
#define DATA_IN_ENDPOINT_TX_FIFO3  0x01000500
#define DATA_IN_ENDPOINT_TX_FIFO4  0x01000600
#define DATA_IN_ENDPOINT_TX_FIFO5  0x01000700
#define DATA_IN_ENDPOINT_TX_FIFO6  0x01000800
#define DATA_IN_ENDPOINT_TX_FIFO7  0x01000900
#define DATA_IN_ENDPOINT_TX_FIFO8  0x01000A00
#define DATA_IN_ENDPOINT_TX_FIFO9  0x01000B00
#define DATA_IN_ENDPOINT_TX_FIFO10 0x01000C00
#define DATA_IN_ENDPOINT_TX_FIFO11 0x00800D00
#define DATA_IN_ENDPOINT_TX_FIFO12 0x00800D80
#define DATA_IN_ENDPOINT_TX_FIFO13 0x00800E00
#define DATA_IN_ENDPOINT_TX_FIFO14 0x00800E80
#define DATA_IN_ENDPOINT_TX_FIFO15 0x00800F00

/*DWC_OTG regsiter descriptor*/
/*Device mode CSR MAP*/
#define DEVICE_CSR_BASE			(0x800)
/*Device mode CSR MAP*/
#define DEVICE_INEP_BASE		(0x900)
/*Device mode CSR MAP*/
#define DEVICE_OUTEP_BASE		(0xB00)

/*** OTG LINK CORE REGISTERS ***/
/* Core Global Registers */
#define GOTGCTL     			(0x000)
#define GOTGINT     			(0x004)
#define GAHBCFG     			(0x008)
#define GUSBCFG     			(0x00C)
#define GRSTCTL     			(0x010)
#define GINTSTS				(0x014)
#define GINTMSK     			(0x018)
#define GRXSTSR     			(0x01C)
#define GRXSTSP     			(0x020)
#define GRXFSIZ     			(0x024)
#define GNPTXFSIZ   			(0x028)
#define GNPTXSTS    			(0x02C)

#define GHWCFG1     			(0x044)
#define GHWCFG2     			(0x048)
#define GHWCFG3     			(0x04c)
#define GHWCFG4     			(0x050)
#define GLPMCFG     			(0x054)

#define GDFIFOCFG     			(0x05c)

#define HPTXFSIZ    			(0x100)
#define DIEPTXF(x) 			(0x100 + 4 * (x))
#define DIEPTXF1   			(0x104)
#define DIEPTXF2   			(0x108)
#define DIEPTXF3   			(0x10C)
#define DIEPTXF4   			(0x110)
#define DIEPTXF5   			(0x114)
#define DIEPTXF6   			(0x118)
#define DIEPTXF7   			(0x11C)
#define DIEPTXF8   			(0x120)
#define DIEPTXF9   			(0x124)
#define DIEPTXF10  			(0x128)
#define DIEPTXF11  			(0x12C)
#define DIEPTXF12  			(0x130)
#define DIEPTXF13  			(0x134)
#define DIEPTXF14  			(0x138)
#define DIEPTXF15  			(0x13C)

/*** HOST MODE REGISTERS ***/
/* Host Global Registers */
#define HCFG       			(0x400)
#define HFIR       			(0x404)
#define HFNUM      			(0x408)
#define HPTXSTS    			(0x410)
#define HAINT      			(0x414)
#define HAINTMSK   			(0x418)

/* Host Port Control and Status Registers */
#define HPRT        			(0x440)

/* Host Channel-Specific Registers */
#define HCCHAR(x)   			(0x500 + 0x20 * (x))
#define HCSPLT(x)   			(0x504 + 0x20 * (x))
#define HCINT(x)    			(0x508 + 0x20 * (x))
#define HCINTMSK(x) 			(0x50C + 0x20 * (x))
#define HCTSIZ(x)   			(0x510 + 0x20 * (x))
#define HCDMA(x)    			(0x514 + 0x20 * (x))
#define HCCHAR0     			(0x500)
#define HCSPLT0     			(0x504)
#define HCINT0      			(0x508)
#define HCINTMSK0   			(0x50C)
#define HCTSIZ0     			(0x510)
#define HCDMA0      			(0x514)
#define HCCHAR1     			(0x520)
#define HCSPLT1     			(0x524)
#define HCINT1      			(0x528)
#define HCINTMSK1   			(0x52C)
#define HCTSIZ1     			(0x530)
#define HCDMA1      			(0x534)
#define HCCHAR2     			(0x540)
#define HCSPLT2     			(0x544)
#define HCINT2      			(0x548)
#define HCINTMSK2   			(0x54C)
#define HCTSIZ2     			(0x550)
#define HCDMA2      			(0x554)
#define HCCHAR3     			(0x560)
#define HCSPLT3     			(0x564)
#define HCINT3      			(0x568)
#define HCINTMSK3   			(0x56C)
#define HCTSIZ3     			(0x570)
#define HCDMA3      			(0x574)
#define HCCHAR4     			(0x580)
#define HCSPLT4     			(0x584)
#define HCINT4      			(0x588)
#define HCINTMSK4   			(0x58C)
#define HCTSIZ4     			(0x590)
#define HCDMA4      			(0x594)
#define HCCHAR5     			(0x5A0)
#define HCSPLT5     			(0x5A4)
#define HCINT5      			(0x5A8)
#define HCINTMSK5   			(0x5AC)
#define HCTSIZ5     			(0x5B0)
#define HCDMA5      			(0x5B4)
#define HCCHAR6     			(0x5C0)
#define HCSPLT6     			(0x5C4)
#define HCINT6      			(0x5C8)
#define HCINTMSK6   			(0x5CC)
#define HCTSIZ6     			(0x5D0)
#define HCDMA6      			(0x5D4)
#define HCCHAR7     			(0x5E0)
#define HCSPLT7     			(0x5E4)
#define HCINT7      			(0x5E8)
#define HCINTMSK7   			(0x5EC)
#define HCTSIZ7     			(0x5F0)
#define HCDMA7      			(0x5F4)
#define HCCHAR8     			(0x600)
#define HCSPLT8     			(0x604)
#define HCINT8      			(0x608)
#define HCINTMSK8   			(0x60C)
#define HCTSIZ8     			(0x610)
#define HCDMA8      			(0x614)
#define HCCHAR9     			(0x620)
#define HCSPLT9     			(0x624)
#define HCINT9      			(0x628)
#define HCINTMSK9   			(0x62C)
#define HCTSIZ9     			(0x630)
#define HCDMA9      			(0x634)
#define HCCHAR10    			(0x640)
#define HCSPLT10    			(0x644)
#define HCINT10     			(0x648)
#define HCINTMSK10  			(0x64C)
#define HCTSIZ10    			(0x650)
#define HCDMA10     			(0x654)
#define HCCHAR11    			(0x660)
#define HCSPLT11    			(0x664)
#define HCINT11     			(0x668)
#define HCINTMSK11  			(0x66C)
#define HCTSIZ11    			(0x670)
#define HCDMA11     			(0x674)
#define HCCHAR12    			(0x680)
#define HCSPLT12    			(0x684)
#define HCINT12     			(0x688)
#define HCINTMSK12  			(0x68C)
#define HCTSIZ12    			(0x690)
#define HCDMA12     			(0x694)
#define HCCHAR13    			(0x6A0)
#define HCSPLT13    			(0x6A4)
#define HCINT13     			(0x6A8)
#define HCINTMSK13  			(0x6AC)
#define HCTSIZ13    			(0x6B0)
#define HCDMA13     			(0x6B4)
#define HCCHAR14    			(0x6C0)
#define HCSPLT14    			(0x6C4)
#define HCINT14     			(0x6C8)
#define HCINTMSK14  			(0x6CC)
#define HCTSIZ14    			(0x6D0)
#define HCDMA14     			(0x6D4)
#define HCCHAR15    			(0x6E0)
#define HCSPLT15    			(0x6E4)
#define HCINT15     			(0x6E8)
#define HCINTMSK15  			(0x6EC)
#define HCTSIZ15    			(0x6F0)
#define HCDMA15     			(0x6F4)

/*** DEVICE MODE REGISTERS ***/
/* Device Global Registers */
#define DCFG        			(0x800)
#define DCTL        			(0x804)
#define DSTS        			(0x808)
#define DIEPMSK     			(0x810)
#define DOEPMSK     			(0x814)
#define DAINT       			(0x818)
#define DAINTMSK    			(0x81C)
#define DTKNQR1     			(0x820)
#define DTKNQR2     			(0x824)
#define DVBUSDIS    			(0x828)
#define DVBUSPULSE  			(0x82C)
#define DTHRCTL     			(0x830)

/* Device Logical IN Endpoint-Specific Registers */
#define DIEPCTL(x)  			(0x900 + 0x20 * (x))
#define DIEPINT(x)  			(0x908 + 0x20 * (x))
#define DIEPTSIZ(x) 			(0x910 + 0x20 * (x))
#define DIEPDMA(x)  			(0x914 + 0x20 * (x))
#define DTXFSTS(x)  			(0x918 + 0x20 * (x))

#define DIEPCTL0    			(0x900)
#define DIEPINT0    			(0x908)
#define DIEPTSIZ0   			(0x910)
#define DIEPDMA0    			(0x914)
#define DIEPCTL1    			(0x920)
#define DIEPINT1    			(0x928)
#define DIEPTSIZ1   			(0x930)
#define DIEPDMA1    			(0x934)
#define DIEPCTL2    			(0x940)
#define DIEPINT2    			(0x948)
#define DIEPTSIZ2  			(0x950)
#define DIEPDMA2    			(0x954)
#define DIEPCTL3    			(0x960)
#define DIEPINT3    			(0x968)
#define DIEPTSIZ3   			(0x970)
#define DIEPDMA3    			(0x974)
#define DIEPCTL4    			(0x980)
#define DIEPINT4    			(0x988)
#define DIEPTSIZ4   			(0x990)
#define DIEPDMA4    			(0x994)
#define DIEPCTL5    			(0x9A0)
#define DIEPINT5    			(0x9A8)
#define DIEPTSIZ5   			(0x9B0)
#define DIEPDMA5    			(0x9B4)
#define DIEPCTL6    			(0x9C0)
#define DIEPINT6    			(0x9C8)
#define DIEPTSIZ6   			(0x9D0)
#define DIEPDMA6    			(0x9D4)
#define DIEPCTL7    			(0x9E0)
#define DIEPINT7    			(0x9E8)
#define DIEPTSIZ7   			(0x9F0)
#define DIEPDMA7    			(0x9F4)
#define DIEPCTL8    			(0xA00)
#define DIEPINT8    			(0xA08)
#define DIEPTSIZ8   			(0xA10)
#define DIEPDMA8    			(0xA14)
#define DIEPCTL9    			(0xA20)
#define DIEPINT9    			(0xA28)
#define DIEPTSIZ9   			(0xA30)
#define DIEPDMA9    			(0xA34)
#define DIEPCTL10   			(0xA40)
#define DIEPINT10   			(0xA48)
#define DIEPTSIZ10  			(0xA50)
#define DIEPDMA10   			(0xA54)
#define DIEPCTL11   			(0xA60)
#define DIEPINT11   			(0xA68)
#define DIEPTSIZ11  			(0xA70)
#define DIEPDMA11   			(0xA74)
#define DIEPCTL12   			(0xA80)
#define DIEPINT12   			(0xA88)
#define DIEPTSIZ12  			(0xA90)
#define DIEPDMA12   			(0xA94)
#define DIEPCTL13   			(0xAA0)
#define DIEPINT13   			(0xAA8)
#define DIEPTSIZ13  			(0xAB0)
#define DIEPDMA13   			(0xAB4)
#define DIEPCTL14   			(0xAC0)
#define DIEPINT14   			(0xAC8)
#define DIEPTSIZ14  			(0xAD0)
#define DIEPDMA14   			(0xAD4)
#define DIEPCTL15   			(0xAE0)
#define DIEPINT15   			(0xAE8)
#define DIEPTSIZ15  			(0xAF0)
#define DIEPDMA15   			(0xAF4)

/* Device Logical OUT Endpoint-Specific Registers */
#define DOEPCTL(x)  			(0xB00 + 0x20 * (x))
#define DOEPINT(x)  			(0xB08 + 0x20 * (x))
#define DOEPTSIZ(x) 			(0xB10 + 0x20 * (x))
#define DOEPDMA(x)  			(0xB14 + 0x20 * (x))
#define DOEPCTL0    			(0xB00)
#define DOEPINT0    			(0xB08)
#define DOEPTSIZ0   			(0xB10)
#define DOEPDMA0    			(0xB14)
#define DOEPCTL1    			(0xB20)
#define DOEPINT1    			(0xB28)
#define DOEPTSIZ1   			(0xB30)
#define DOEPDMA1    			(0xB34)
#define DOEPCTL2    			(0xB40)
#define DOEPINT2    			(0xB48)
#define DOEPTSIZ2   			(0xB50)
#define DOEPDMA2    			(0xB54)
#define DOEPCTL3    			(0xB60)
#define DOEPINT3    			(0xB68)
#define DOEPTSIZ3   			(0xB70)
#define DOEPDMA3    			(0xB74)
#define DOEPCTL4    			(0xB80)
#define DOEPINT4    			(0xB88)
#define DOEPTSIZ4   			(0xB90)
#define DOEPDMA4    			(0xB94)
#define DOEPCTL5    			(0xBA0)
#define DOEPINT5    			(0xBA8)
#define DOEPTSIZ5   			(0xBB0)
#define DOEPDMA5    			(0xBB4)
#define DOEPCTL6    			(0xBC0)
#define DOEPINT6    			(0xBC8)
#define DOEPTSIZ6   			(0xBD0)
#define DOEPDMA6    			(0xBD4)
#define DOEPCTL7    			(0xBE0)
#define DOEPINT7    			(0xBE8)
#define DOEPTSIZ7   			(0xBF0)
#define DOEPDMA7    			(0xBF4)
#define DOEPCTL8    			(0xC00)
#define DOEPINT8    			(0xC08)
#define DOEPTSIZ8   			(0xC10)
#define DOEPDMA8    			(0xC14)
#define DOEPCTL9    			(0xC20)
#define DOEPINT9    			(0xC28)
#define DOEPTSIZ9   			(0xC30)
#define DOEPDMA9    			(0xC34)
#define DOEPCTL10   			(0xC40)
#define DOEPINT10   			(0xC48)
#define DOEPTSIZ10  			(0xC50)
#define DOEPDMA10   			(0xC54)
#define DOEPCTL11   			(0xC60)
#define DOEPINT11   			(0xC68)
#define DOEPTSIZ11  			(0xC70)
#define DOEPDMA11   			(0xC74)
#define DOEPCTL12   			(0xC80)
#define DOEPINT12   			(0xC88)
#define DOEPTSIZ12  			(0xC90)
#define DOEPDMA12   			(0xC94)
#define DOEPCTL13   			(0xCA0)
#define DOEPINT13   			(0xCA8)
#define DOEPTSIZ13  			(0xCB0)
#define DOEPDMA13   			(0xCB4)
#define DOEPCTL14   			(0xCC0)
#define DOEPINT14   			(0xCC8)
#define DOEPTSIZ14  			(0xCD0)
#define DOEPDMA14   			(0xCD4)
#define DOEPCTL15   			(0xCE0)
#define DOEPINT15   			(0xCE8)
#define DOEPTSIZ15  			(0xCF0)
#define DOEPDMA15   			(0xCF4)

/* Power and Clock Gating Register */
#define PCGCCTL				(0xE00)

#define EP0FIFO				(0x1000)

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
	UINT32 buf;
} dwc_otg_dev_dma_desc_t;

#endif //ifndef __DW_USB_DXE_H__
