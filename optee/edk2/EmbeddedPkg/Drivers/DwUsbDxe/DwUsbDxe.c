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

#include <IndustryStandard/Usb.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UncachedMemoryAllocationLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Protocol/UsbDevice.h>
#include <Guid/ArmGlobalVariableHob.h>

#include "DwUsbDxe.h"

STATIC dwc_otg_dev_dma_desc_t *g_dma_desc,*g_dma_desc_ep0,*g_dma_desc_in;
STATIC USB_DEVICE_REQUEST *p_ctrlreq;
STATIC VOID *rx_buf;
STATIC UINT32 rx_desc_bytes = 0;
STATIC UINTN mNumDataBytes;

#define USB_BLOCK_HIGH_SPEED_SIZE    512
#define DATA_SIZE 32768
#define CMD_SIZE 512
#define MATCH_CMD_LITERAL(Cmd, Buf) !AsciiStrnCmp (Cmd, Buf, sizeof (Cmd) - 1)

STATIC USB_DEVICE_DESCRIPTOR    *mDeviceDescriptor;

// The config descriptor, interface descriptor, and endpoint descriptors in a
// buffer (in that order)
STATIC VOID                     *mDescriptors;
// Convenience pointers to those descriptors inside the buffer:
STATIC USB_INTERFACE_DESCRIPTOR *mInterfaceDescriptor;
STATIC USB_CONFIG_DESCRIPTOR    *mConfigDescriptor;
STATIC USB_ENDPOINT_DESCRIPTOR  *mEndpointDescriptors;

STATIC USB_DEVICE_RX_CALLBACK   mDataReceivedCallback;
STATIC USB_DEVICE_TX_CALLBACK   mDataSentCallback;

STATIC EFI_USB_STRING_DESCRIPTOR mLangStringDescriptor = {
  4,
  USB_DESC_TYPE_STRING,
  {0x409}
};

STATIC EFI_USB_STRING_DESCRIPTOR mManufacturerStringDescriptor = {
  18,
  USB_DESC_TYPE_STRING,
  {'9', '6', 'B', 'o', 'a', 'r', 'd', 's'}
};

STATIC EFI_USB_STRING_DESCRIPTOR mProductStringDescriptor = {
  12,
  USB_DESC_TYPE_STRING,
  {'H', 'i', 'K', 'e', 'y'}
};

STATIC EFI_USB_STRING_DESCRIPTOR mSerialStringDescriptor = {
  34,
  USB_DESC_TYPE_STRING,
  {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'}
};

// The time between interrupt polls, in units of 100 nanoseconds
// 10 Microseconds
#define DW_INTERRUPT_POLL_PERIOD 10000
STATIC int usb_drv_port_speed(void) /*To detect which mode was run, high speed or full speed*/
{
    /*
    * 2'b00: High speed (PHY clock is running at 30 or 60 MHz)
    */
    UINT32 val = READ_REG32(DSTS) & 2;
    return (!val);
}

STATIC VOID reset_endpoints(void)
{
  /* EP0 IN ACTIVE NEXT=1 */
  WRITE_REG32(DIEPCTL0, 0x8800);

  /* EP0 OUT ACTIVE */
  WRITE_REG32(DOEPCTL0, 0x8000);

  /* Clear any pending OTG Interrupts */
  WRITE_REG32(GOTGINT, 0xFFFFFFFF);

  /* Clear any pending interrupts */
  WRITE_REG32(GINTSTS, 0xFFFFFFFF);
  WRITE_REG32(DIEPINT0, 0xFFFFFFFF);
  WRITE_REG32(DOEPINT0, 0xFFFFFFFF);
  WRITE_REG32(DIEPINT1, 0xFFFFFFFF);
  WRITE_REG32(DOEPINT1, 0xFFFFFFFF);

  /* IN EP interrupt mask */
  WRITE_REG32(DIEPMSK, 0x0D);
  /* OUT EP interrupt mask */
  WRITE_REG32(DOEPMSK, 0x0D);
  /* Enable interrupts on Ep0 */
  WRITE_REG32(DAINTMSK, 0x00010001);

  /* EP0 OUT Transfer Size:64 Bytes, 1 Packet, 3 Setup Packet, Read to receive setup packet*/
  WRITE_REG32(DOEPTSIZ0, 0x60080040);

  //notes that:the compulsive conversion is expectable.
  g_dma_desc_ep0->status.b.bs = 0x3;
  g_dma_desc_ep0->status.b.mtrf = 0;
  g_dma_desc_ep0->status.b.sr = 0;
  g_dma_desc_ep0->status.b.l = 1;
  g_dma_desc_ep0->status.b.ioc = 1;
  g_dma_desc_ep0->status.b.sp = 0;
  g_dma_desc_ep0->status.b.bytes = 64;
  g_dma_desc_ep0->buf = (UINT32)(UINTN)(p_ctrlreq);
  g_dma_desc_ep0->status.b.sts = 0;
  g_dma_desc_ep0->status.b.bs = 0x0;
  WRITE_REG32(DOEPDMA0, (unsigned long)(g_dma_desc_ep0));
  /* EP0 OUT ENABLE CLEARNAK */
  WRITE_REG32(DOEPCTL0, (READ_REG32(DOEPCTL0) | 0x84000000));
}

STATIC VOID ep_tx(IN UINT8 ep, CONST VOID *ptr, UINT32 len)
{
    UINT32 blocksize;
    UINT32 packets;

    /* EPx OUT ACTIVE */
    WRITE_REG32(DIEPCTL(ep), (READ_REG32(DIEPCTL(ep))) | 0x8000);
    if(!ep) {
        blocksize = 64;
    } else {
        blocksize = usb_drv_port_speed() ? USB_BLOCK_HIGH_SPEED_SIZE : 64;
    }
    packets = (len + blocksize - 1) / blocksize;

    if (!len) { //send a null packet
        /* one empty packet */
        g_dma_desc_in->status.b.bs = 0x3;
        g_dma_desc_in->status.b.l = 1;
        g_dma_desc_in->status.b.ioc = 1;
        g_dma_desc_in->status.b.sp = 1;
        g_dma_desc_in->status.b.bytes = 0;
        g_dma_desc_in->buf = 0;
        g_dma_desc_in->status.b.sts = 0;
        g_dma_desc_in->status.b.bs = 0x0;

        WRITE_REG32(DIEPDMA(ep), (unsigned long)(g_dma_desc_in));             // DMA Address (DMAAddr) is zero
    } else { //prepare to send a packet
        /*WRITE_REG32((len | (packets << 19)), DIEPTSIZ(ep));*/  // packets+transfer size
	WRITE_REG32(DIEPTSIZ(ep), len | (packets << 19));

	//flush cache
	WriteBackDataCacheRange ((void*)ptr, len);

        g_dma_desc_in->status.b.bs = 0x3;
        g_dma_desc_in->status.b.l = 1;
        g_dma_desc_in->status.b.ioc = 1;
        g_dma_desc_in->status.b.sp = 1;
        g_dma_desc_in->status.b.bytes = len;
        g_dma_desc_in->buf = (UINT32)((UINTN)ptr);
        g_dma_desc_in->status.b.sts = 0;
        g_dma_desc_in->status.b.bs = 0x0;
        WRITE_REG32(DIEPDMA(ep), (unsigned long)(g_dma_desc_in));         // ptr is DMA address
    }
    asm("dsb  sy");
    asm("isb  sy");
    /* epena & cnak*/
    WRITE_REG32(DIEPCTL(ep), READ_REG32(DIEPCTL(ep)) | 0x84000800);
    return;
}

STATIC VOID ep_rx(unsigned ep, UINT32 len)
{
    /* EPx UNSTALL */
    WRITE_REG32(DOEPCTL(ep), ((READ_REG32(DOEPCTL(ep))) & (~0x00200000)));
    /* EPx OUT ACTIVE */
    WRITE_REG32(DOEPCTL(ep), (READ_REG32(DOEPCTL(ep)) | 0x8000));

    if (len >= DATA_SIZE)
	    rx_desc_bytes = DATA_SIZE;
    else
	    rx_desc_bytes = len;

    rx_buf = AllocatePool (DATA_SIZE);
    ASSERT (rx_buf != NULL);

    InvalidateDataCacheRange (rx_buf, len);

    g_dma_desc->status.b.bs = 0x3;
    g_dma_desc->status.b.mtrf = 0;
    g_dma_desc->status.b.sr = 0;
    g_dma_desc->status.b.l = 1;
    g_dma_desc->status.b.ioc = 1;
    g_dma_desc->status.b.sp = 0;
    g_dma_desc->status.b.bytes = rx_desc_bytes;
    g_dma_desc->buf = (UINT32)((UINTN)rx_buf);
    g_dma_desc->status.b.sts = 0;
    g_dma_desc->status.b.bs = 0x0;

    asm("dsb  sy");
    asm("isb  sy");
    WRITE_REG32(DOEPDMA(ep), (UINT32)((UINTN)g_dma_desc));
    /* EPx OUT ENABLE CLEARNAK */
    WRITE_REG32(DOEPCTL(ep), (READ_REG32(DOEPCTL(ep)) | 0x84000000));
}

STATIC
EFI_STATUS
HandleGetDescriptor (
  IN USB_DEVICE_REQUEST  *Request
  )
{
  UINT8       DescriptorType;
  UINTN       ResponseSize;
  VOID       *ResponseData;
  CHAR16      SerialNo[16];
  UINTN       SerialNoLen;
  EFI_STATUS  Status;

  ResponseSize = 0;
  ResponseData = NULL;

  // Pretty confused if bmRequestType is anything but this:
  ASSERT (Request->RequestType == USB_DEV_GET_DESCRIPTOR_REQ_TYPE);

  // Choose the response
  DescriptorType = Request->Value >> 8;
  switch (DescriptorType) {
  case USB_DESC_TYPE_DEVICE:
    DEBUG ((EFI_D_INFO, "USB: Got a request for device descriptor\n"));
    ResponseSize = sizeof (USB_DEVICE_DESCRIPTOR);
    ResponseData = mDeviceDescriptor;
    break;
  case USB_DESC_TYPE_CONFIG:
    DEBUG ((EFI_D_INFO, "USB: Got a request for config descriptor\n"));
    ResponseSize = mConfigDescriptor->TotalLength;
    ResponseData = mDescriptors;
    break;
  case USB_DESC_TYPE_STRING:
    DEBUG ((EFI_D_INFO, "USB: Got a request for String descriptor %d\n", Request->Value & 0xFF));
    switch (Request->Value & 0xff) {
    case 0:
      ResponseSize = mLangStringDescriptor.Length;
      ResponseData = &mLangStringDescriptor;
      break;
    case 1:
      ResponseSize = mManufacturerStringDescriptor.Length;
      ResponseData = &mManufacturerStringDescriptor;
      break;
    case 2:
      ResponseSize = mProductStringDescriptor.Length;
      ResponseData = &mProductStringDescriptor;
      break;
    case 3:
      Status = gRT->GetVariable (
                      (CHAR16*)L"SerialNo",
                      &gArmGlobalVariableGuid,
                      NULL,
                      &SerialNoLen,
                      SerialNo
                      );
      if (EFI_ERROR (Status) == 0) {
        CopyMem (mSerialStringDescriptor.String, SerialNo, SerialNoLen);
      }
      ResponseSize = mSerialStringDescriptor.Length;
      ResponseData = &mSerialStringDescriptor;
      break;
    }
    break;
  default:
    DEBUG ((EFI_D_INFO, "USB: Didn't understand request for descriptor 0x%04x\n", Request->Value));
    break;
  }

  // Send the response
  if (ResponseData) {
    ASSERT (ResponseSize != 0);

    if (Request->Length < ResponseSize) {
      // Truncate response
      ResponseSize = Request->Length;
    } else if (Request->Length > ResponseSize) {
      DEBUG ((EFI_D_INFO, "USB: Info: ResponseSize < wLength\n"));
    }

    ep_tx(0, ResponseData, ResponseSize);
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
HandleSetAddress (
  IN USB_DEVICE_REQUEST  *Request
  )
{
  // Pretty confused if bmRequestType is anything but this:
  ASSERT (Request->RequestType == USB_DEV_SET_ADDRESS_REQ_TYPE);
  DEBUG ((EFI_D_INFO, "USB: Setting address to %d\n", Request->Value));
  reset_endpoints();

  WRITE_REG32(DCFG, (READ_REG32(DCFG) & ~0x7F0) | (Request->Value << 4));
  ep_tx(0, 0, 0);

  return EFI_SUCCESS;
}

int usb_drv_request_endpoint(unsigned int type, int dir)
{
  unsigned int ep = 1;    /*FIXME*/
  int ret;
  unsigned long newbits;
  
  ret = (int)ep | dir;
  newbits = (type << 18) | 0x10000000;
  
  /*
   * (type << 18):Endpoint Type (EPType)
   * 0x10000000:Endpoint Enable (EPEna)
   * 0x000C000:Endpoint Type (EPType);Hardcoded to 00 for control.
   * (ep<<22):TxFIFO Number (TxFNum)
   * 0x20000:NAK Status (NAKSts);The core is transmitting NAK handshakes on this endpoint.
   */
  if (dir) {  // IN: to host
  	WRITE_REG32(DIEPCTL(ep), ((READ_REG32(DIEPCTL(ep)))& ~0x000C0000) | newbits | (ep<<22)|0x20000);
  } else {    // OUT: to device
  	WRITE_REG32(DOEPCTL(ep), ((READ_REG32(DOEPCTL(ep))) & ~0x000C0000) | newbits);
  }
  
  return ret;
}
STATIC
EFI_STATUS
HandleSetConfiguration (
  IN USB_DEVICE_REQUEST  *Request
  )
{
  ASSERT (Request->RequestType == USB_DEV_SET_CONFIGURATION_REQ_TYPE);

  // Cancel all transfers
  reset_endpoints();

  usb_drv_request_endpoint(2, 0);
  usb_drv_request_endpoint(2, 0x80);

  WRITE_REG32(DIEPCTL1, (READ_REG32(DIEPCTL1)) | 0x10088800);

  /* Enable interrupts on all endpoints */
  WRITE_REG32(DAINTMSK, 0xFFFFFFFF);

  ep_rx(1, CMD_SIZE);
  ep_tx(0, 0, 0);
  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
HandleDeviceRequest (
  IN USB_DEVICE_REQUEST  *Request
  )
{
  EFI_STATUS  Status;

  switch (Request->Request) {
  case USB_DEV_GET_DESCRIPTOR:
    Status = HandleGetDescriptor (Request);
    break;
  case USB_DEV_SET_ADDRESS:
    Status = HandleSetAddress (Request);
    break;
  case USB_DEV_SET_CONFIGURATION:
    Status = HandleSetConfiguration (Request);
    break;
  default:
    DEBUG ((EFI_D_ERROR,
      "Didn't understand RequestType 0x%x Request 0x%x\n",
      Request->RequestType, Request->Request));
      Status = EFI_INVALID_PARAMETER;
    break;
  }

  return Status;
}


// Instead of actually registering interrupt handlers, we poll the controller's
//  interrupt source register in this function.
STATIC
VOID
CheckInterrupts (
  IN EFI_EVENT  Event,
  IN VOID      *Context
  )
{
  UINT32 ints = READ_REG32(GINTSTS);    // interrupt register
  UINT32 epints;

  /*
   * bus reset
   * The core sets this bit to indicate that a reset is detected on the USB.
   */
  if (ints & 0x1000) {
	  WRITE_REG32(DCFG, 0x800004);
	  reset_endpoints();
  }

  /*
   * enumeration done, we now know the speed
   * The core sets this bit to indicate that speed enumeration is complete. The
   * application must read the Device Status (DSTS) register to obtain the
   * enumerated speed.
   */
  if (ints & 0x2000) {
	  /* Set up the maximum packet sizes accordingly */
	  unsigned long maxpacket = usb_drv_port_speed() ? USB_BLOCK_HIGH_SPEED_SIZE : 64;
	  //Set Maximum In Packet Size (MPS)
	  WRITE_REG32(DIEPCTL1, ((READ_REG32(DIEPCTL1)) & ~0x000003FF) | maxpacket);
	  //Set Maximum Out Packet Size (MPS)
	  WRITE_REG32(DOEPCTL1, ((READ_REG32(DOEPCTL1)) & ~0x000003FF) | maxpacket);
  }

  /*
   * IN EP event
   * The core sets this bit to indicate that an interrupt is pending on one of the IN
   * endpoints of the core (in Device mode). The application must read the
   * Device All Endpoints Interrupt (DAINT) register to determine the exact
   * number of the IN endpoint on which the interrupt occurred, and then read
   * the corresponding Device IN Endpoint-n Interrupt (DIEPINTn) register to
   * determine the exact cause of the interrupt. The application must clear the
   * appropriate status bit in the corresponding DIEPINTn register to clear this bit.
   */
  if (ints & 0x40000) {
	  epints = READ_REG32(DIEPINT0);
	  WRITE_REG32(DIEPINT0, epints);
	  if (epints & 0x1) /* Transfer Completed Interrupt (XferCompl) */
		  DEBUG ((EFI_D_INFO, "INT: IN TX completed.DIEPTSIZ(0) = 0x%x.\n", READ_REG32(DIEPTSIZ0)));

	  epints = READ_REG32(DIEPINT1);
	  WRITE_REG32(DIEPINT1, epints);
	  if (epints & 0x1)
		  DEBUG ((EFI_D_INFO, "ep1: IN TX completed\n"));
  }

  /*
   * OUT EP event
   * The core sets this bit to indicate that an interrupt is pending on one of the
   * OUT endpoints of the core (in Device mode). The application must read the
   * Device All Endpoints Interrupt (DAINT) register to determine the exact
   * number of the OUT endpoint on which the interrupt occurred, and then read
   * the corresponding Device OUT Endpoint-n Interrupt (DOEPINTn) register
   * to determine the exact cause of the interrupt. The application must clear the
   * appropriate status bit in the corresponding DOEPINTn register to clear this bit.
   */
  if (ints & 0x80000) {
	  /* indicates the status of an endpoint
	   * with respect to USB- and AHB-related events. */
	  epints = READ_REG32(DOEPINT0);
	  if(epints) {
		  WRITE_REG32(DOEPINT0, epints);
		  if (epints & 0x1)
			  DEBUG ((EFI_D_INFO,"INT: EP0 RX completed. DOEPTSIZ(0) = 0x%x.\n", READ_REG32(DOEPTSIZ0)));
		  /*
		   *
		   IN Token Received When TxFIFO is Empty (INTknTXFEmp)
		   * Indicates that an IN token was received when the associated TxFIFO (periodic/nonperiodic)
		   * was empty. This interrupt is asserted on the endpoint for which the IN token
		   * was received.
		   */
		  if (epints & 0x8) { /* SETUP phase done */
			  // PRINT_DEBUG("Setup phase \n");
			  WRITE_REG32(DIEPCTL0, READ_REG32(DIEPCTL0) | 0x08000000);
			  WRITE_REG32(DOEPCTL0, READ_REG32(DOEPCTL0) | 0x08000000);
			  /*clear IN EP intr*/
			  WRITE_REG32(DIEPINT0, 0xffffffff);
			  HandleDeviceRequest((USB_DEVICE_REQUEST *)p_ctrlreq);
		  }

		  /* Make sure EP0 OUT is set up to accept the next request */
		  /* memset(p_ctrlreq, 0, NUM_ENDPOINTS*8); */
		  WRITE_REG32(DOEPTSIZ0, 0x60080040);
		  /*
		   * IN Token Received When TxFIFO is Empty (INTknTXFEmp)
		   * Indicates that an IN token was received when the associated TxFIFO (periodic/nonperiodic)
		   * was empty. This interrupt is asserted on the endpoint for which the IN token
		   * was received.
		   */
		  g_dma_desc_ep0->status.b.bs = 0x3;
		  g_dma_desc_ep0->status.b.mtrf = 0;
		  g_dma_desc_ep0->status.b.sr = 0;
		  g_dma_desc_ep0->status.b.l = 1;
		  g_dma_desc_ep0->status.b.ioc = 1;
		  g_dma_desc_ep0->status.b.sp = 0;
		  g_dma_desc_ep0->status.b.bytes = 64;
		  g_dma_desc_ep0->buf = (UINT32)(UINTN)(p_ctrlreq);
		  g_dma_desc_ep0->status.b.sts = 0;
		  g_dma_desc_ep0->status.b.bs = 0x0;
		  WRITE_REG32(DOEPDMA0, (unsigned long)(g_dma_desc_ep0));
		  // endpoint enable; clear NAK
		  WRITE_REG32(DOEPCTL0, 0x84000000);
	  }

	  epints = (READ_REG32(DOEPINT1));
	  if(epints) {
		  WRITE_REG32(DOEPINT1, epints);
		  /* Transfer Completed Interrupt (XferCompl);Transfer completed */
		  if (epints & 0x1) {
			  asm("dsb  sy");
			  asm("isb  sy");

			  UINT32 bytes = rx_desc_bytes - g_dma_desc->status.b.bytes;
			  UINT32 len = 0;

			  if (MATCH_CMD_LITERAL ("download", rx_buf)) {
				  mNumDataBytes = AsciiStrHexToUint64 (rx_buf + sizeof ("download"));
			  } else {
				if (mNumDataBytes != 0)
					mNumDataBytes -= bytes;
			  }

			  mDataReceivedCallback (bytes, rx_buf);

			  if (mNumDataBytes == 0)
				  len = CMD_SIZE;
			  else if (mNumDataBytes > DATA_SIZE)
				  len = DATA_SIZE;
			  else
				  len = mNumDataBytes;

			  ep_rx(1, len);
		  }
	  }
  }

  //WRITE_REG32 clear ints
  WRITE_REG32(GINTSTS, ints);
}

EFI_STATUS
DwUsbSend (
  IN        UINT8  EndpointIndex,
  IN        UINTN  Size,
  IN  CONST VOID  *Buffer
  )
{
    ep_tx(EndpointIndex, Buffer, Size);
    return 0;
}

STATIC VOID phy_init()
{
  UINT32 val;

  //setup clock
  val = PHY_READ_REG32(0x200);
  val |= BIT4;
  PHY_WRITE_REG32(0x200, val);

  //setup phy

  val = PHY_READ_REG32(SC_PERIPH_RSTDIS0);
  val |= RST0_USBOTG_BUS | RST0_POR_PICOPHY |
	  RST0_USBOTG | RST0_USBOTG_32K;
  PHY_WRITE_REG32(SC_PERIPH_RSTDIS0, val);

  val = PHY_READ_REG32(SC_PERIPH_CTRL5);
  val &= ~CTRL5_PICOPHY_BC_MODE;
  val |= CTRL5_USBOTG_RES_SEL | CTRL5_PICOPHY_ACAENB;
  PHY_WRITE_REG32(SC_PERIPH_CTRL5, val);

  val = PHY_READ_REG32(SC_PERIPH_CTRL4);
  val &= ~(CTRL4_PICO_SIDDQ | CTRL4_PICO_OGDISABLE);
  val |=  CTRL4_PICO_VBUSVLDEXT | CTRL4_PICO_VBUSVLDEXTSEL |
	  CTRL4_OTG_PHY_SEL;
  PHY_WRITE_REG32(SC_PERIPH_CTRL4, val);

  PHY_WRITE_REG32(SC_PERIPH_CTRL8, EYE_PATTERN_PARA);
}


STATIC VOID usb_init()
{
  VOID* buf;

  buf = UncachedAllocatePages (1);
  g_dma_desc = buf;
  g_dma_desc_ep0 = g_dma_desc + sizeof(struct dwc_otg_dev_dma_desc);
  g_dma_desc_in = g_dma_desc_ep0 + sizeof(struct dwc_otg_dev_dma_desc);
  p_ctrlreq = (USB_DEVICE_REQUEST *)g_dma_desc_in + sizeof(struct dwc_otg_dev_dma_desc);

  SetMem(g_dma_desc, sizeof(struct dwc_otg_dev_dma_desc), 0);
  SetMem(g_dma_desc_ep0, sizeof(struct dwc_otg_dev_dma_desc), 0);
  SetMem(g_dma_desc_in, sizeof(struct dwc_otg_dev_dma_desc), 0);

  /*Reset usb controller.*/
  /* Wait for OTG AHB master idle */
  while (!((READ_REG32(GRSTCTL)) & 0x80000000));

  /* OTG: Assert Software Reset */
  WRITE_REG32(GRSTCTL, 1);

  /* Wait for OTG to ack reset */
  while ((READ_REG32(GRSTCTL)) & 1);

  /* Wait for OTG AHB master idle */
  while (!((READ_REG32(GRSTCTL)) & 0x80000000));

  WRITE_REG32(GDFIFOCFG, DATA_FIFO_CONFIG);
  WRITE_REG32(GRXFSIZ, RX_SIZE);
  WRITE_REG32(GNPTXFSIZ, ENDPOINT_TX_SIZE);
  WRITE_REG32(DIEPTXF1, DATA_IN_ENDPOINT_TX_FIFO1);

  /*
   * set Periodic TxFIFO Empty Level,
   * Non-Periodic TxFIFO Empty Level,
   * Enable DMA, Unmask Global Intr
   */
  WRITE_REG32(GAHBCFG, 0x1a1);

  /*select 8bit UTMI+, ULPI Inerface*/
  WRITE_REG32(GUSBCFG, 0x2400);

  /* Detect usb work mode,host or device? */
  while ((READ_REG32(GINTSTS)) & 1);
  MicroSecondDelay(1);

  /*Init global and device mode csr register.*/
  /*set Non-Zero-Length status out handshake */
  WRITE_REG32(DCFG, 0x800004);

  /* Interrupt unmask: IN event, OUT event, bus reset */
  WRITE_REG32(GINTMSK, 0xC3C08);

  while ((READ_REG32(GINTSTS)) & 0x2000);

  /* Clear any pending OTG Interrupts */
  WRITE_REG32(GOTGINT, 0xFFFFFFFF);
  /* Clear any pending interrupts */
  WRITE_REG32(GINTSTS, 0xFFFFFFFF);
  WRITE_REG32(GINTMSK, 0xFFFFFFFF);
  WRITE_REG32(GOTGINT, READ_REG32(GOTGINT) & (~0x3000));
  /*endpoint settings cfg*/
  reset_endpoints();

  /*init finish. and ready to transfer data*/

  /* Soft Disconnect */
  WRITE_REG32(DCTL, 0x802);
  MicroSecondDelay(1);

  /* Soft Reconnect */
  WRITE_REG32(DCTL, 0x800);
}

EFI_STATUS
EFIAPI
DwUsbStart (
  IN USB_DEVICE_DESCRIPTOR   *DeviceDescriptor,
  IN VOID                   **Descriptors,
  IN USB_DEVICE_RX_CALLBACK   RxCallback,
  IN USB_DEVICE_TX_CALLBACK   TxCallback
  )
{
  UINT8                    *Ptr;
  EFI_STATUS                Status;
  EFI_EVENT                 TimerEvent;

  ASSERT (DeviceDescriptor != NULL);
  ASSERT (Descriptors[0] != NULL);
  ASSERT (RxCallback != NULL);
  ASSERT (TxCallback != NULL);

  usb_init();

  mDeviceDescriptor = DeviceDescriptor;
  mDescriptors = Descriptors[0];

  // Right now we just support one configuration
  ASSERT (mDeviceDescriptor->NumConfigurations == 1);
  // ... and one interface
  mConfigDescriptor = (USB_CONFIG_DESCRIPTOR *)mDescriptors;
  ASSERT (mConfigDescriptor->NumInterfaces == 1);

  Ptr = ((UINT8 *) mDescriptors) + sizeof (USB_CONFIG_DESCRIPTOR);
  mInterfaceDescriptor = (USB_INTERFACE_DESCRIPTOR *) Ptr;
  Ptr += sizeof (USB_INTERFACE_DESCRIPTOR);

  mEndpointDescriptors = (USB_ENDPOINT_DESCRIPTOR *) Ptr;

  mDataReceivedCallback = RxCallback;
  mDataSentCallback = TxCallback;

  // Register a timer event so CheckInterupts gets called periodically
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  CheckInterrupts,
                  NULL,
                  &TimerEvent
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->SetTimer (
                  TimerEvent,
                  TimerPeriodic,
                  DW_INTERRUPT_POLL_PERIOD
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

USB_DEVICE_PROTOCOL mUsbDevice = {
  DwUsbStart,
  DwUsbSend
};


EFI_STATUS
EFIAPI
DwUsbEntryPoint (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_HANDLE  Handle;

  phy_init();
  Handle = NULL;
  return gBS->InstallProtocolInterface (
		  &Handle,
		  &gUsbDeviceProtocolGuid,
		  EFI_NATIVE_INTERFACE,
		  &mUsbDevice
		  );
}
