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
#include <ctype.h>
#include <debug.h>
#include <gpio.h>
#include <hi6220.h>
#include <mmio.h>
#include <partitions.h>
#include <platform_def.h>
#include <sp804_timer.h>
#include <string.h>
#include <usb.h>
#include "hikey_private.h"

#define NUM_ENDPOINTS			16

#define USB_BLOCK_HIGH_SPEED_SIZE	512

struct ep_type {
	unsigned char		active;
	unsigned char		busy;
	unsigned char		done;
	unsigned int		rc;
	unsigned int		size;
};

struct usb_endpoint {
	struct usb_endpoint	*next;
	unsigned int		maxpkt;
	struct usb_request	*req;
	unsigned char		num;
	unsigned char		in;
};

struct usb_config_bundle {
	struct usb_config_descriptor config;
	struct usb_interface_descriptor interface;
	struct usb_endpoint_descriptor ep1;
	struct usb_endpoint_descriptor ep2;
} __attribute__ ((packed));

static setup_packet ctrl_req[NUM_ENDPOINTS]
__attribute__ ((section("tzfw_coherent_mem")));
static unsigned char ctrl_resp[2]
__attribute__ ((section("tzfw_coherent_mem")));

static struct ep_type endpoints[NUM_ENDPOINTS]
__attribute__ ((section("tzfw_coherent_mem")));

dwc_otg_dev_dma_desc_t dma_desc
__attribute__ ((section("tzfw_coherent_mem")));
dwc_otg_dev_dma_desc_t dma_desc_ep0
__attribute__ ((section("tzfw_coherent_mem")));
dwc_otg_dev_dma_desc_t dma_desc_in
__attribute__ ((section("tzfw_coherent_mem")));
dwc_otg_dev_dma_desc_t dma_desc_addr
__attribute__ ((section("tzfw_coherent_mem")));

static struct usb_config_bundle config_bundle
__attribute__ ((section("tzfw_coherent_mem")));
static struct usb_device_descriptor device_descriptor
__attribute__ ((section("tzfw_coherent_mem")));

static struct usb_request rx_req
__attribute__ ((section("tzfw_coherent_mem")));
static struct usb_request tx_req
__attribute__ ((section("tzfw_coherent_mem")));

static struct usb_string_descriptor serial_string
__attribute__ ((section("tzfw_coherent_mem")));

static const struct usb_string_descriptor string_devicename = {
	24,
	USB_DT_STRING,
	{'A', 'n', 'd', 'r', 'o', 'i', 'd', ' ', '2', '.', '0'}
};

static const struct usb_string_descriptor serial_string_descriptor = {
	34,
	USB_DT_STRING,
	{'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'}
};

static const struct usb_string_descriptor lang_descriptor = {
	4,
	USB_DT_STRING,
	{0x0409}	/* en-US */
};

static void usb_rx_cmd_complete(unsigned actual, int stat);
static void usb_rx_data_complete(unsigned actual, int status);

static unsigned int rx_desc_bytes = 0;
static unsigned long rx_addr;
static unsigned long rx_length;
static unsigned int last_one = 0;
static char *cmdbuf;
static struct usb_endpoint ep1in, ep1out;
static int g_usb_enum_flag = 0;

int usb_need_reset = 0;

static int usb_drv_port_speed(void)
{
	/* 2'b00 High speed (PHY clock is at 30MHz or 60MHz) */
	return (mmio_read_32(DSTS) & 2) == 0 ? 1 : 0;
}

static void reset_endpoints(void)
{
	int i;
	unsigned int data;

	INFO("enter reset_endpoints.\n");
	for (i = 0; i < NUM_ENDPOINTS; i++) {
		endpoints[i].active = 0;
		endpoints[i].busy = 0;
		endpoints[i].rc = -1;
		endpoints[i].done = 1;
	}

	/* EP0 IN ACTIVE NEXT=1 */
	mmio_write_32(DIEPCTL0, 0x8800);

	/* EP0 OUT ACTIVE */
	mmio_write_32(DOEPCTL0, 0x8000);

	/* Clear any pending OTG Interrupts */
	mmio_write_32(GOTGINT, ~0);

	/* Clear any pending interrupts */
	mmio_write_32(GINTSTS, ~0);
	mmio_write_32(DIEPINT0, ~0);
	mmio_write_32(DOEPINT0, ~0);
	mmio_write_32(DIEPINT1, ~0);
	mmio_write_32(DOEPINT1, ~0);

	/* IN EP interrupt mask */
	mmio_write_32(DIEPMSK, 0x0D);
	/* OUT EP interrupt mask */
	mmio_write_32(DOEPMSK, 0x0D);
	/* Enable interrupts on Ep0 */
	mmio_write_32(DAINTMSK, 0x00010001);

	/* EP0 OUT Transfer Size:64 Bytes, 1 Packet, 3 Setup Packet, Read to receive setup packet*/
	data = DOEPTSIZ0_SUPCNT(3) | DOEPTSIZ0_PKTCNT |
		(64 << DOEPTSIZ0_XFERSIZE_SHIFT);
	mmio_write_32(DOEPTSIZ0, data);
	//notes that:the compulsive conversion is expectable.
	dma_desc_ep0.status.b.bs = 0x3;
	dma_desc_ep0.status.b.mtrf = 0;
	dma_desc_ep0.status.b.sr = 0;
	dma_desc_ep0.status.b.l = 1;
	dma_desc_ep0.status.b.ioc = 1;
	dma_desc_ep0.status.b.sp = 0;
	dma_desc_ep0.status.b.bytes = 64;
	dma_desc_ep0.buf = (unsigned long)&ctrl_req;
	dma_desc_ep0.status.b.sts = 0;
	dma_desc_ep0.status.b.bs = 0x0;
	mmio_write_32(DOEPDMA0, ((unsigned long)&(dma_desc_ep0)));
	VERBOSE("%s, &ctrl_req:%llx:%x, &dms_desc_ep0:%llx:%x\n",
		__func__, (unsigned long)&ctrl_req, (unsigned long)&ctrl_req,
		(unsigned long)&dma_desc_ep0, (unsigned long)&dma_desc_ep0);
	/* EP0 OUT ENABLE CLEARNAK */
	data = mmio_read_32(DOEPCTL0);
	mmio_write_32(DOEPCTL0, (data | 0x84000000));

	VERBOSE("exit reset_endpoints. \n");
}

static int usb_drv_request_endpoint(int type, int dir)
{
	int ep = 1;    /*FIXME*/
	unsigned int newbits, data;

	newbits = (type << 18) | 0x10000000;

	/*
	 * (type << 18):Endpoint Type (EPType)
	 * 0x10000000:Endpoint Enable (EPEna)
	 * 0x000C000:Endpoint Type (EPType);Hardcoded to 00 for control.
	 * (ep<<22):TxFIFO Number (TxFNum)
	 * 0x20000:NAK Status (NAKSts);The core is transmitting NAK handshakes on this endpoint.
	 */
	if (dir) {  // IN: to host
		data = mmio_read_32(DIEPCTL(ep));
		data &= ~0x000c0000;
		data |= newbits | (ep << 22) | 0x20000;
		mmio_write_32(DIEPCTL(ep), data);
	} else {    // OUT: to device
		data = mmio_read_32(DOEPCTL(ep));
		data &= ~0x000c0000;
		data |= newbits;
		mmio_write_32(DOEPCTL(ep), data);
	}
	endpoints[ep].active = 1;	// true

    return ep | dir;
}

void usb_drv_release_endpoint(int ep)
{
	ep = ep % NUM_ENDPOINTS;
	if (ep < 1 || ep > NUM_ENDPOINTS)
		return;

	endpoints[ep].active = 0;
}

void usb_config(void)
{
	unsigned int data;

	INFO("enter usb_config\n");

	mmio_write_32(GDFIFOCFG, DATA_FIFO_CONFIG);
	mmio_write_32(GRXFSIZ, RX_SIZE);
	mmio_write_32(GNPTXFSIZ, ENDPOINT_TX_SIZE);

	mmio_write_32(DIEPTXF1, DATA_IN_ENDPOINT_TX_FIFO1);
	mmio_write_32(DIEPTXF2, DATA_IN_ENDPOINT_TX_FIFO2);
	mmio_write_32(DIEPTXF3, DATA_IN_ENDPOINT_TX_FIFO3);
	mmio_write_32(DIEPTXF4, DATA_IN_ENDPOINT_TX_FIFO4);
	mmio_write_32(DIEPTXF5, DATA_IN_ENDPOINT_TX_FIFO5);
	mmio_write_32(DIEPTXF6, DATA_IN_ENDPOINT_TX_FIFO6);
	mmio_write_32(DIEPTXF7, DATA_IN_ENDPOINT_TX_FIFO7);
	mmio_write_32(DIEPTXF8, DATA_IN_ENDPOINT_TX_FIFO8);
	mmio_write_32(DIEPTXF9, DATA_IN_ENDPOINT_TX_FIFO9);
	mmio_write_32(DIEPTXF10, DATA_IN_ENDPOINT_TX_FIFO10);
	mmio_write_32(DIEPTXF11, DATA_IN_ENDPOINT_TX_FIFO11);
	mmio_write_32(DIEPTXF12, DATA_IN_ENDPOINT_TX_FIFO12);
	mmio_write_32(DIEPTXF13, DATA_IN_ENDPOINT_TX_FIFO13);
	mmio_write_32(DIEPTXF14, DATA_IN_ENDPOINT_TX_FIFO14);
	mmio_write_32(DIEPTXF15, DATA_IN_ENDPOINT_TX_FIFO15);

	/*Init global csr register.*/

	/*
	 * set Periodic TxFIFO Empty Level,
	 * Non-Periodic TxFIFO Empty Level,
	 * Enable DMA, Unmask Global Intr
	 */
	INFO("USB: DMA mode.\n");
	mmio_write_32(GAHBCFG, GAHBCFG_CTRL_MASK);

	/*select 8bit UTMI+, ULPI Inerface*/
	INFO("USB ULPI PHY\n");
	mmio_write_32(GUSBCFG, 0x2400);

	/* Detect usb work mode,host or device? */
	do {
		data = mmio_read_32(GINTSTS);
	} while (data & GINTSTS_CURMODE_HOST);
	VERBOSE("Enter device mode\n");
	udelay(3);

	/*Init global and device mode csr register.*/
	/*set Non-Zero-Length status out handshake */
	data = (0x20 << DCFG_EPMISCNT_SHIFT) | DCFG_NZ_STS_OUT_HSHK;
	mmio_write_32(DCFG, data);

	/* Interrupt unmask: IN event, OUT event, bus reset */
	data = GINTSTS_OEPINT | GINTSTS_IEPINT | GINTSTS_ENUMDONE |
	       GINTSTS_USBRST | GINTSTS_USBSUSP | GINTSTS_ERLYSUSP |
	       GINTSTS_GOUTNAKEFF;
	mmio_write_32(GINTMSK, data);

	do {
		data = mmio_read_32(GINTSTS) & GINTSTS_ENUMDONE;
	} while (data);
	VERBOSE("USB Enum Done.\n");

	/* Clear any pending OTG Interrupts */
	mmio_write_32(GOTGINT, ~0);
	/* Clear any pending interrupts */
	mmio_write_32(GINTSTS, ~0);
	mmio_write_32(GINTMSK, ~0);
	data = mmio_read_32(GOTGINT);
	data &= ~0x3000;
	mmio_write_32(GOTGINT, data);
	/*endpoint settings cfg*/
	reset_endpoints();

	udelay(1);

	/*init finish. and ready to transfer data*/

	/* Soft Disconnect */
	mmio_write_32(DCTL, 0x802);
	udelay(10000);

	/* Soft Reconnect */
	mmio_write_32(DCTL, 0x800);
	VERBOSE("exit usb_config.\n");
}

void usb_drv_set_address(int address)
{
	unsigned int cfg;

	cfg = mmio_read_32(DCFG);
	cfg &= ~0x7F0;
	cfg |= address << 4;
	mmio_write_32(DCFG, cfg);	// 0x7F0: device address
}

static void ep_send(int ep, const void *ptr, int len)
{
	unsigned int data;

	endpoints[ep].busy = 1;		// true
	endpoints[ep].size = len;

	/* EPx OUT ACTIVE */
	data = mmio_read_32(DIEPCTL(ep)) | DXEPCTL_USBACTEP;
	mmio_write_32(DIEPCTL(ep), data);

	/* set DMA Address */
	if (!len) {
		/* send one empty packet */
		dma_desc_in.buf = 0;
	} else {
		dma_desc_in.buf = (unsigned long)ptr;
	}
	dma_desc_in.status.b.bs = 0x3;
	dma_desc_in.status.b.l = 1;
	dma_desc_in.status.b.ioc = 1;
	dma_desc_in.status.b.sp = 1;
	dma_desc_in.status.b.sts = 0;
	dma_desc_in.status.b.bs = 0x0;
	dma_desc_in.status.b.bytes = len;
	mmio_write_32(DIEPDMA(ep), (unsigned long)&dma_desc_in);

	data = mmio_read_32(DIEPCTL(ep));
	data |= DXEPCTL_EPENA | DXEPCTL_CNAK | DXEPCTL_NEXTEP(ep + 1);
	mmio_write_32(DIEPCTL(ep), data);
}

void usb_drv_stall(int endpoint, char stall, char in)
{
	unsigned int data;

	/*
	 * STALL Handshake (Stall)
	 */

	data = mmio_read_32(DIEPCTL(endpoint));
	if (in) {
		if (stall)
			mmio_write_32(DIEPCTL(endpoint), data | 0x00200000);
		else
			mmio_write_32(DIEPCTL(endpoint), data & ~0x00200000);
	} else {
		if (stall)
			mmio_write_32(DOEPCTL(endpoint), data | 0x00200000);
		else
			mmio_write_32(DOEPCTL(endpoint), data & ~0x00200000);
	}
}

int usb_drv_send_nonblocking(int endpoint, const void *ptr, int len)
{
	VERBOSE("%s, endpoint = %d, ptr = 0x%x, Len=%d.\n",
		__func__, endpoint, ptr, len);
	ep_send(endpoint % NUM_ENDPOINTS, ptr, len);
	return 0;
}

void usb_drv_cancel_all_transfers(void)
{
	reset_endpoints();
}

int hiusb_epx_tx(unsigned ep, void *buf, unsigned len)
{
	int blocksize,packets;
	unsigned int epints;
	unsigned int cycle = 0;
	unsigned int data;

	endpoints[ep].busy = 1;		//true
	endpoints[ep].size = len;

	while (mmio_read_32(GINTSTS) & 0x40) {
		data = mmio_read_32(DCTL);
		data |= 0x100;
		mmio_write_32(DCTL, data);
	}

	data = mmio_read_32(DIEPCTL(ep));
	data |= 0x08000000;
	mmio_write_32(DIEPCTL(ep), data);

	/* EPx OUT ACTIVE */
	mmio_write_32(DIEPCTL(ep), data | 0x8000);
	if (!ep) {
		blocksize = 64;
	} else {
		blocksize = usb_drv_port_speed() ? USB_BLOCK_HIGH_SPEED_SIZE : 64;
	}
	packets = (len + blocksize - 1) / blocksize;

	if (!len) {
		/* one empty packet */
		mmio_write_32(DIEPTSIZ(ep), 1 << 19);
		/* NULL */
		dma_desc_in.status.b.bs = 0x3;
		dma_desc_in.status.b.l = 1;
		dma_desc_in.status.b.ioc = 1;
		dma_desc_in.status.b.sp = last_one;
		dma_desc_in.status.b.bytes = 0;
		dma_desc_in.buf = 0;
		dma_desc_in.status.b.sts = 0;
		dma_desc_in.status.b.bs = 0x0;
		mmio_write_32(DIEPDMA(ep), (unsigned long)&dma_desc_in);
	} else {
		mmio_write_32(DIEPTSIZ(ep), len | (packets << 19));
		dma_desc_in.status.b.bs = 0x3;
		dma_desc_in.status.b.l = 1;
		dma_desc_in.status.b.ioc = 1;
		dma_desc_in.status.b.sp = last_one;
		dma_desc_in.status.b.bytes = len;
		dma_desc_in.buf = (unsigned long)buf;
		dma_desc_in.status.b.sts = 0;
		dma_desc_in.status.b.bs = 0x0;
		mmio_write_32(DIEPDMA(ep), (unsigned long)&dma_desc_in);
	}

	cycle = 0;
	while(1){
		data = mmio_read_32(DIEPINT(ep));
		if ((data & 0x2000) || (cycle > 10000)) {
			if (cycle > 10000) {
				NOTICE("Phase 2:ep(%d) status, DIEPCTL(%d) is [0x%x],"
				       "DTXFSTS(%d) is [0x%x], DIEPINT(%d) is [0x%x],"
				       "DIEPTSIZ(%d) is [0x%x] GINTSTS is [0x%x]\n",
					ep, ep, data,
					ep, mmio_read_32(DTXFSTS(ep)),
					ep, mmio_read_32(DIEPINT(ep)),
					ep, mmio_read_32(DIEPTSIZ(ep)),
					mmio_read_32(GINTSTS));
			}
			break;
		}

		cycle++;
		udelay(10);
	}
	VERBOSE("ep(%d) enable, DIEPCTL(%d) is [0x%x], DTXFSTS(%d) is [0x%x],"
		"DIEPINT(%d) is [0x%x], DIEPTSIZ(%d) is [0x%x] \n",
		ep, ep, mmio_read_32(DIEPCTL(ep)),
		ep, mmio_read_32(DTXFSTS(ep)),
		ep, mmio_read_32(DIEPINT(ep)),
		ep, mmio_read_32(DIEPTSIZ(ep)));

	__asm__ volatile("dsb	sy\n"
			 "isb	sy\n");
	data = mmio_read_32(DIEPCTL(ep));
	data |= 0x84000000;
	/* epena & cnak*/
	mmio_write_32(DIEPCTL(ep), data);
	__asm__ volatile("dsb	sy\n"
			 "isb	sy\n");

	cycle = 0;
	while (1) {
		epints = mmio_read_32(DIEPINT(ep)) & 1;
		if ((mmio_read_32(GINTSTS) & 0x40000) && epints) {
			VERBOSE("Tx succ:ep(%d), DTXFSTS(%d) is [0x%x] \n",
				ep, ep, mmio_read_32(DTXFSTS(ep)));
			mmio_write_32(DIEPINT(ep), epints);
			if (endpoints[ep].busy) {
				endpoints[ep].busy = 0;//false
				endpoints[ep].rc = 0;
				endpoints[ep].done = 1;//true
			}
			break;
		}
		cycle++;
		udelay(10);
		VERBOSE("loop for intr: ep(%d), DIEPCTL(%d) is [0x%x], ",
			"DTXFSTS(%d) is [0x%x], DIEPINT(%d) is [0x%x] \n",
			ep, ep, mmio_read_32(DIEPCTL(ep)),
			ep, mmio_read_32(DTXFSTS(ep)),
			ep, mmio_read_32(DIEPINT(ep)));

		if (cycle > 1000000) {
			WARN("Wait IOC intr over 10s! USB will reset\n");
			usb_need_reset = 1;
			return 1;
		}
	}

	cycle = 0;
	while (1) {
		if ((mmio_read_32(DIEPINT(ep)) & 0x2000) || (cycle > 100000)) {
			if (cycle > 100000){
				WARN("all wait cycle is [%d]\n",cycle);
			}
			break;
		}

		cycle++;
		udelay(10);
	}

	return 0;
}

int hiusb_epx_rx(unsigned ep, void *buf, unsigned len)
{
	unsigned int blocksize = 0, data;
	int packets;

	VERBOSE("ep%d rx, len = 0x%x, buf = 0x%x.\n", ep, len, buf);

	endpoints[ep].busy = 1;//true
	/* EPx UNSTALL */
	data = mmio_read_32(DOEPCTL(ep)) & ~0x00200000;
	mmio_write_32(DOEPCTL(ep), data);
	/* EPx OUT ACTIVE */
	data = mmio_read_32(DOEPCTL(ep)) | 0x8000;
	mmio_write_32(DOEPCTL(ep), data);

	blocksize = usb_drv_port_speed() ? USB_BLOCK_HIGH_SPEED_SIZE : 64;
	packets = (len + blocksize - 1) / blocksize;

#define MAX_RX_PACKET 0x3FF

	/*Max recv packets is 1023*/
	if (packets > MAX_RX_PACKET) {
		endpoints[ep].size = MAX_RX_PACKET * blocksize;
		len = MAX_RX_PACKET * blocksize;
	} else {
		endpoints[ep].size = len;
	}

	if (!len) {
		/* one empty packet */
		mmio_write_32(DOEPTSIZ(ep), 1 << 19);
		//NULL  /* dummy address */
		dma_desc.status.b.bs = 0x3;
		dma_desc.status.b.mtrf = 0;
		dma_desc.status.b.sr = 0;
		dma_desc.status.b.l = 1;
		dma_desc.status.b.ioc = 1;
		dma_desc.status.b.sp = 0;
		dma_desc.status.b.bytes = 0;
		dma_desc.buf = 0;
		dma_desc.status.b.sts = 0;
		dma_desc.status.b.bs = 0x0;

		mmio_write_32(DOEPDMA(ep), (unsigned long)&dma_desc);
	} else {
		if (len >= blocksize * 64) {
			rx_desc_bytes = blocksize*64;
		} else {
			rx_desc_bytes = len;
		}
		VERBOSE("rx len %d, rx_desc_bytes %d \n",len,rx_desc_bytes);
		dma_desc.status.b.bs = 0x3;
		dma_desc.status.b.mtrf = 0;
		dma_desc.status.b.sr = 0;
		dma_desc.status.b.l = 1;
		dma_desc.status.b.ioc = 1;
		dma_desc.status.b.sp = 0;
		dma_desc.status.b.bytes = rx_desc_bytes;
		dma_desc.buf = (unsigned long)buf;
		dma_desc.status.b.sts = 0;
		dma_desc.status.b.bs = 0x0;

		mmio_write_32(DOEPDMA(ep), (unsigned long)&dma_desc);
	}
	/* EPx OUT ENABLE CLEARNAK */
	data = mmio_read_32(DOEPCTL(ep));
	data |= 0x84000000;
	mmio_write_32(DOEPCTL(ep), data);
	return 0;
}

int usb_queue_req(struct usb_endpoint *ept, struct usb_request *req)
{
	if (ept->in)
		hiusb_epx_tx(ept->num, req->buf, req->length);
	else
		hiusb_epx_rx(ept->num, req->buf, req->length);

	return 0;
}

static void rx_cmd(void)
{
	struct usb_request *req = &rx_req;
	req->buf = cmdbuf;
	req->length = RX_REQ_LEN;
	req->complete = usb_rx_cmd_complete;
	usb_queue_req(&ep1out, req);
}

static void rx_data(void)
{
	struct usb_request *req = &rx_req;

	req->buf = (void *)((unsigned long) rx_addr);
	req->length = rx_length;
	req->complete = usb_rx_data_complete;
	usb_queue_req(&ep1out, req);
}

void tx_status(const char *status)
{
	struct usb_request *req = &tx_req;
	int len = strlen(status);

	memcpy(req->buf, status, (unsigned int)len);
	req->length = (unsigned int)len;
	req->complete = 0;
	usb_queue_req(&ep1in, req);
}

void fastboot_tx_status(const char *status)
{
	tx_status(status);
	rx_cmd();
}

void tx_dump_page(const char *ptr, int len)
{
	struct usb_request *req = &tx_req;

	memcpy(req->buf, ptr, (unsigned int)len);
	req->length = (unsigned int)len;
	req->complete = 0;
	usb_queue_req(&ep1in, req);
}


static void usb_rx_data_complete(unsigned actual, int status)
{

	if(status != 0)
		return;

	if(actual > rx_length) {
		actual = rx_length;
	}

	rx_addr += actual;
	rx_length -= actual;

	if(rx_length > 0) {
		rx_data();
	} else {
		tx_status("OKAY");
		rx_cmd();
	}
}

static void usb_status(unsigned online, unsigned highspeed)
{
	if (online) {
		INFO("usb: online (%s)\n", highspeed ? "highspeed" : "fullspeed");
		rx_cmd();
	}
}

void usb_handle_control_request(setup_packet* req)
{
	const void* addr = NULL;
	int size = -1;
	int i;
	int maxpacket;
	unsigned int data;
	char *serialno;
	struct usb_endpoint_descriptor epx;
	struct usb_config_bundle const_bundle = {
		.config = {
			.bLength	= sizeof(struct usb_config_descriptor),
			.bDescriptorType	= USB_DT_CONFIG,
			.wTotalLength	= sizeof(struct usb_config_descriptor) +
				sizeof(struct usb_interface_descriptor) +
				sizeof(struct usb_endpoint_descriptor) *
				USB_NUM_ENDPOINTS,
			.bNumInterfaces		= 1,
			.bConfigurationValue	= 1,
			.iConfiguration		= 0,
			.bmAttributes		= USB_CONFIG_ATT_ONE,
			.bMaxPower		= 0x80
		},
		.interface = {
			.bLength	= sizeof(struct usb_interface_descriptor),
			.bDescriptorType	= USB_DT_INTERFACE,
			.bInterfaceNumber	= 0,
			.bAlternateSetting	= 0,
			.bNumEndpoints		= USB_NUM_ENDPOINTS,
			.bInterfaceClass	= USB_CLASS_VENDOR_SPEC,
			.bInterfaceSubClass	= 0x42,
			.bInterfaceProtocol	= 0x03,
			.iInterface		= 0
		}
	};

	/* avoid to hang on accessing unaligned memory */
	struct usb_endpoint_descriptor const_ep1 = {
		.bLength	= sizeof(struct usb_endpoint_descriptor),
		.bDescriptorType	= USB_DT_ENDPOINT,
		.bEndpointAddress	= 0x81,
		.bmAttributes		= USB_ENDPOINT_XFER_BULK,
		.wMaxPacketSize		= 0,
		.bInterval		= 0
	};

	struct usb_endpoint_descriptor const_ep2 = {
		.bLength	= sizeof(struct usb_endpoint_descriptor),
		.bDescriptorType	= USB_DT_ENDPOINT,
		.bEndpointAddress	= 0x01,
		.bmAttributes		= USB_ENDPOINT_XFER_BULK,
		.wMaxPacketSize		= 0,
		.bInterval		= 1
	};

	struct usb_device_descriptor const_device = {
		.bLength		= sizeof(struct usb_device_descriptor),
		.bDescriptorType	= USB_DT_DEVICE,
		.bcdUSB			= 0x0200,
		.bDeviceClass		= 0,
		.bDeviceClass		= 0,
		.bDeviceProtocol	= 0,
		.bMaxPacketSize0	= 0x40,
		.idVendor		= 0x18d1,
		.idProduct		= 0xd00d,
		.bcdDevice		= 0x0100,
		.iManufacturer		= 1,
		.iProduct		= 2,
		.iSerialNumber		= 3,
		.bNumConfigurations	= 1
	};

	memcpy(&config_bundle, &const_bundle, sizeof(struct usb_config_bundle));
	memcpy(&config_bundle.ep1, &const_ep1, sizeof(struct usb_endpoint_descriptor));
	memcpy(&config_bundle.ep2, &const_ep2, sizeof(struct usb_endpoint_descriptor));
	memcpy(&device_descriptor, &const_device,
		sizeof(struct usb_device_descriptor));

	switch (req->request) {
	case USB_REQ_GET_STATUS:
		if (req->type == USB_DIR_IN)
			ctrl_resp[0] = 1;
		else
			ctrl_resp[0] = 0;
		ctrl_resp[1] = 0;
		addr = ctrl_resp;
		size = 2;
		break;

	case USB_REQ_CLEAR_FEATURE:
		if ((req->type == USB_RECIP_ENDPOINT) &&
		    (req->value == USB_ENDPOINT_HALT))
			usb_drv_stall(req->index & 0xf, 0, req->index >> 7);
		size = 0;
		break;

	case USB_REQ_SET_FEATURE:
		size = 0;
		break;

	case USB_REQ_SET_ADDRESS:
		size = 0;
		usb_drv_cancel_all_transfers();     // all endpoints reset
		usb_drv_set_address(req->value);   // set device address
		break;

	case USB_REQ_GET_DESCRIPTOR:
		VERBOSE("USB_REQ_GET_DESCRIPTOR: 0x%x\n", req->value >> 8);
		switch (req->value >> 8) {
		case USB_DT_DEVICE:
			addr = &device_descriptor;
			size = sizeof(device_descriptor);
			VERBOSE("Get device descriptor.\n");
			break;

		case USB_DT_OTHER_SPEED_CONFIG:
		case USB_DT_CONFIG:
			if ((req->value >> 8) == USB_DT_CONFIG) {
				maxpacket = usb_drv_port_speed() ? USB_BLOCK_HIGH_SPEED_SIZE : 64;
				config_bundle.config.bDescriptorType = USB_DT_CONFIG;
			} else {
				maxpacket = usb_drv_port_speed() ? 64 : USB_BLOCK_HIGH_SPEED_SIZE;
				config_bundle.config.bDescriptorType = USB_DT_OTHER_SPEED_CONFIG;
			}
			/* avoid hang when access unaligned structure */
			memcpy(&epx, &config_bundle.ep1, sizeof(struct usb_endpoint_descriptor));
			epx.wMaxPacketSize = maxpacket;
			memcpy(&config_bundle.ep1, &epx, sizeof(struct usb_endpoint_descriptor));
			memcpy(&epx, &config_bundle.ep2, sizeof(struct usb_endpoint_descriptor));
			epx.wMaxPacketSize = maxpacket;
			memcpy(&config_bundle.ep2, &epx, sizeof(struct usb_endpoint_descriptor));
			addr = &config_bundle;
			size = sizeof(config_bundle);
			VERBOSE("Get config descriptor.\n");
			break;

		case USB_DT_STRING:
			switch (req->value & 0xff) {
			case 0:
				addr = &lang_descriptor;
				size = lang_descriptor.bLength;
				break;
			case 1:
				addr = &string_devicename;
				size = 14;
				break;
			case 2:
				addr = &string_devicename;
				size = string_devicename.bLength;
				break;
			case 3:
				serialno = load_serialno();
				if (serialno == NULL) {
					addr = &serial_string_descriptor;
					size = serial_string_descriptor.bLength;
				} else {
					i = 0;
					memcpy((void *)&serial_string,
					       (void *)&serial_string_descriptor,
					       sizeof(serial_string));
					while (1) {
						serial_string.wString[i] = serialno[i];
						if (serialno[i] == '\0')
							break;
						i++;
					}
					addr = &serial_string;
					size = serial_string.bLength;
				}
				break;
			default:
				break;
			}
			break;

		default:
			break;
		}
		break;

	case USB_REQ_GET_CONFIGURATION:
		ctrl_resp[0] = 1;
		addr = ctrl_resp;
		size = 1;
		break;

	case USB_REQ_SET_CONFIGURATION:
		usb_drv_cancel_all_transfers();     // call reset_endpoints  reset all EPs

		usb_drv_request_endpoint(USB_ENDPOINT_XFER_BULK, USB_DIR_OUT);
		usb_drv_request_endpoint(USB_ENDPOINT_XFER_BULK, USB_DIR_IN);
		/*
		 * 0x10088800:
		 * 1:EP enable; 8:EP type:BULK; 8:USB Active Endpoint; 8:Next Endpoint
		 */
		data = mmio_read_32(DIEPCTL1) | 0x10088800;
		mmio_write_32(DIEPCTL1, data);
		data = mmio_read_32(DIEPCTL(1)) | 0x08000000;
		mmio_write_32(DIEPCTL(1), data);

		/* Enable interrupts on all endpoints */
		mmio_write_32(DAINTMSK, 0xffffffff);

		usb_status(req->value? 1 : 0, usb_drv_port_speed() ? 1 : 0);
		size = 0;
		VERBOSE("Set config descriptor.\n");

		/* USB 枚举成功点,置上标识 */
		g_usb_enum_flag = 1;
		break;

	default:
		break;
	}

	if (!size) {
		usb_drv_send_nonblocking(0, 0, 0);  // send an empty packet
	} else if (size == -1) { // stall:Applies to non-control, non-isochronous IN and OUT endpoints only.
		usb_drv_stall(0, 1, 1);     // IN
		usb_drv_stall(0, 1, 0);     // OUT
	} else { // stall:Applies to control endpoints only.
		usb_drv_stall(0, 0, 1);     // IN
		usb_drv_stall(0, 0, 0);     // OUT

		usb_drv_send_nonblocking(0, addr, size > req->length ? req->length : size);
	}
}

/* IRQ handler */
static void usb_poll(void)
{
	uint32_t ints;
	uint32_t epints, data;

	ints = mmio_read_32(GINTSTS);		/* interrupt status */


	if ((ints & 0xc3010) == 0)
		return;
	/*
	 * bus reset
	 * The core sets this bit to indicate that a reset is detected on the USB.
	 */
	if (ints & GINTSTS_USBRST) {
		VERBOSE("bus reset intr\n");
		/*set Non-Zero-Length status out handshake */
		/*
		 * DCFG:This register configures the core in Device mode after power-on
		 * or after certain control commands or enumeration. Do not make changes
		 * to this register after initial programming.
		 * Send a STALL handshake on a nonzero-length status OUT transaction and
		 * do not send the received OUT packet to the application.
		 */
		mmio_write_32(DCFG, 0x800004);
		reset_endpoints();
	}
	/*
	 * enumeration done, we now know the speed
	 * The core sets this bit to indicate that speed enumeration is complete. The
	 * application must read the Device Status (DSTS) register to obtain the
	 * enumerated speed.
	 */
	if (ints & GINTSTS_ENUMDONE) {
		/* Set up the maximum packet sizes accordingly */
		uint32_t maxpacket = usb_drv_port_speed() ? USB_BLOCK_HIGH_SPEED_SIZE : 64;  // high speed maxpacket=512
		VERBOSE("enum done intr. Maxpacket:%d\n", maxpacket);
		//Set Maximum In Packet Size (MPS)
		data = mmio_read_32(DIEPCTL1) & ~0x000003ff;
		mmio_write_32(DIEPCTL1, data | maxpacket);
		//Set Maximum Out Packet Size (MPS)
		data = mmio_read_32(DOEPCTL1) & ~0x000003ff;
		mmio_write_32(DOEPCTL1, data | maxpacket);
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
	if (ints & GINTSTS_IEPINT) {
		epints = mmio_read_32(DIEPINT0);
		mmio_write_32(DIEPINT0, epints);

		//VERBOSE("IN EP event,ints:0x%x, DIEPINT0:%x, DAINT:%x, DAINTMSK:%x.\n",
		//	ints, epints, mmio_read_32(DAINT), mmio_read_32(DAINTMSK));
		if (epints & 0x1) { /* Transfer Completed Interrupt (XferCompl) */
			VERBOSE("TX completed.DIEPTSIZ(0) = 0x%x.\n", mmio_read_32(DIEPTSIZ0));
			/*FIXME,Maybe you can use bytes*/
			/*int bytes = endpoints[0].size - (DIEPTSIZ(0) & 0x3FFFF);*/ //actual transfer
			if (endpoints[0].busy) {
				endpoints[0].busy = 0;//false
				endpoints[0].rc = 0;
				endpoints[0].done = 1;//true
			}
		}
		if (epints & 0x4) { /* AHB error */
			WARN("AHB error on IN EP0.\n");
		}

		if (epints & 0x8) { /* Timeout */
			WARN("Timeout on IN EP0.\n");
			if (endpoints[0].busy) {
				endpoints[0].busy = 1;//false
				endpoints[0].rc = 1;
				endpoints[0].done = 1;//true
			}
		}
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
	if (ints & GINTSTS_OEPINT) {
		/* indicates the status of an endpoint
		 * with respect to USB- and AHB-related events. */
		epints = mmio_read_32(DOEPINT(0));
		//VERBOSE("OUT EP event,ints:0x%x, DOEPINT0:%x, DAINT:%x, DAINTMSK:%x.\n",
		//	ints, epints, mmio_read_32(DAINT), mmio_read_32(DAINTMSK));
		if (epints) {
			mmio_write_32(DOEPINT(0), epints);
			/* Transfer completed */
			if (epints & DXEPINT_XFERCOMPL) {
				/*FIXME,need use bytes*/
				VERBOSE("EP0 RX completed. DOEPTSIZ(0) = 0x%x.\n",
					mmio_read_32(DOEPTSIZ(0)));
				if (endpoints[0].busy) {
					endpoints[0].busy = 0;
					endpoints[0].rc = 0;
					endpoints[0].done = 1;
				}
			}
			if (epints & DXEPINT_AHBERR) { /* AHB error */
				WARN("AHB error on OUT EP0.\n");
			}

			/*
			 * IN Token Received When TxFIFO is Empty (INTknTXFEmp)
			 * Indicates that an IN token was received when the associated TxFIFO (periodic/nonperiodic)
			 * was empty. This interrupt is asserted on the endpoint for which the IN token
			 * was received.
			 */
			if (epints & DXEPINT_SETUP) { /* SETUP phase done */
				VERBOSE("Setup phase \n");
				data = mmio_read_32(DIEPCTL(0)) | DXEPCTL_SNAK;
				mmio_write_32(DIEPCTL(0), data);
				data = mmio_read_32(DOEPCTL(0)) | DXEPCTL_SNAK;
				mmio_write_32(DOEPCTL(0), data);
				/*clear IN EP intr*/
				mmio_write_32(DIEPINT(0), ~0);
				usb_handle_control_request((setup_packet *)&ctrl_req);
			}

			/* Make sure EP0 OUT is set up to accept the next request */
			/* memset(p_ctrlreq, 0, NUM_ENDPOINTS*8); */
			data = DOEPTSIZ0_SUPCNT(3) | DOEPTSIZ0_PKTCNT |
				(64 << DOEPTSIZ0_XFERSIZE_SHIFT);
			mmio_write_32(DOEPTSIZ0, data);
			/*
			 * IN Token Received When TxFIFO is Empty (INTknTXFEmp)
			 * Indicates that an IN token was received when the associated TxFIFO (periodic/nonperiodic)
			 * was empty. This interrupt is asserted on the endpoint for which the IN token
			 * was received.
			 */
			// notes that:the compulsive conversion is expectable.
			// Holds the start address of the external memory for storing or fetching endpoint data.
			dma_desc_ep0.status.b.bs = 0x3;
			dma_desc_ep0.status.b.mtrf = 0;
			dma_desc_ep0.status.b.sr = 0;
			dma_desc_ep0.status.b.l = 1;
			dma_desc_ep0.status.b.ioc = 1;
			dma_desc_ep0.status.b.sp = 0;
			dma_desc_ep0.status.b.bytes = 64;
			dma_desc_ep0.buf = (uintptr_t)&ctrl_req;
			dma_desc_ep0.status.b.sts = 0;
			dma_desc_ep0.status.b.bs = 0x0;
			mmio_write_32(DOEPDMA0, (uintptr_t)&dma_desc_ep0);
			// endpoint enable; clear NAK
			mmio_write_32(DOEPCTL0, 0x84000000);
		}

		epints = mmio_read_32(DOEPINT1);
		if(epints) {
			mmio_write_32(DOEPINT1, epints);
			VERBOSE("OUT EP1: epints :0x%x,DOEPTSIZ1 :0x%x.\n",epints, mmio_read_32(DOEPTSIZ1));
			/* Transfer Completed Interrupt (XferCompl);Transfer completed */
			if (epints & DXEPINT_XFERCOMPL) {
				/* ((readl(DOEPTSIZ(1))) & 0x7FFFF is Transfer Size (XferSize) */
				/*int bytes = (p_endpoints + 1)->size - ((readl(DOEPTSIZ(1))) & 0x7FFFF);*/
				int bytes = rx_desc_bytes - dma_desc.status.b.bytes;
				VERBOSE("OUT EP1: recv %d bytes \n",bytes);
				if (endpoints[1].busy) {
					endpoints[1].busy = 0;
					endpoints[1].rc = 0;
					endpoints[1].done = 1;
					rx_req.complete(bytes, 0);
				}
			}

			if (epints & DXEPINT_AHBERR) { /* AHB error */
				WARN("AHB error on OUT EP1.\n");
			}
			if (epints & DXEPINT_SETUP) { /* SETUP phase done */
				WARN("SETUP phase done  on OUT EP1.\n");
			}
		}
	}
	/* write to clear interrupts */
	mmio_write_32(GINTSTS, ints);
}

#define EYE_PATTERN	0x70533483

/*
* pico phy exit siddq, nano phy enter siddq,
* and open the clock of pico phy and dvc,
*/
static void dvc_and_picophy_init_chip(void)
{
	unsigned int data;

	/* enable USB clock */
	mmio_write_32(PERI_SC_PERIPH_CLKEN0, PERI_CLK0_USBOTG);
	do {
		data = mmio_read_32(PERI_SC_PERIPH_CLKSTAT0);
	} while ((data & PERI_CLK0_USBOTG) == 0);


	/* out of reset */
	mmio_write_32(PERI_SC_PERIPH_RSTDIS0,
		      PERI_RST0_USBOTG_BUS | PERI_RST0_POR_PICOPHY |
		      PERI_RST0_USBOTG | PERI_RST0_USBOTG_32K);
	do {
		data = mmio_read_32(PERI_SC_PERIPH_RSTSTAT0);
		data &= PERI_RST0_USBOTG_BUS | PERI_RST0_POR_PICOPHY |
			PERI_RST0_USBOTG | PERI_RST0_USBOTG_32K;
	} while (data);

	mmio_write_32(PERI_SC_PERIPH_CTRL8, EYE_PATTERN);

	/* configure USB PHY */
	data = mmio_read_32(PERI_SC_PERIPH_CTRL4);
	/* make PHY out of low power mode */
	data &= ~PERI_CTRL4_PICO_SIDDQ;
	/* detect VBUS by external circuit, switch D+ to 1.5KOhm pullup */
	data |= PERI_CTRL4_PICO_VBUSVLDEXTSEL | PERI_CTRL4_PICO_VBUSVLDEXT;
	data &= ~PERI_CTRL4_FPGA_EXT_PHY_SEL;
	/* select PHY */
	data &= ~PERI_CTRL4_OTG_PHY_SEL;
	mmio_write_32(PERI_SC_PERIPH_CTRL4, data);

	udelay(1000);

	data = mmio_read_32(PERI_SC_PERIPH_CTRL5);
	data &= ~PERI_CTRL5_PICOPHY_BC_MODE;
	mmio_write_32(PERI_SC_PERIPH_CTRL5, data);
    
	udelay(20000);
}

int init_usb(void)
{
	static int init_flag = 0;
	uint32_t	data;

	if (init_flag == 0) {
		memset(&ctrl_req, 0, sizeof(setup_packet));
		memset(&ctrl_resp, 0, 2);
		memset(&endpoints, 0, sizeof(struct ep_type) * NUM_ENDPOINTS);
		memset(&dma_desc, 0, sizeof(struct dwc_otg_dev_dma_desc));
		memset(&dma_desc_ep0, 0, sizeof(struct dwc_otg_dev_dma_desc));
		memset(&dma_desc_in, 0, sizeof(struct dwc_otg_dev_dma_desc));
	}

	VERBOSE("Pico PHY and DVC init start.\n");

	dvc_and_picophy_init_chip();
	VERBOSE("Pico PHY and DVC init done.\n");

	/* wait for OTG AHB master idle */
	do {
		data = mmio_read_32(GRSTCTL) & GRSTCTL_AHBIDLE;
	} while (data == 0);
	VERBOSE("Reset usb controller\n");

	/* OTG: Assert software reset */
	mmio_write_32(GRSTCTL, GRSTCTL_CSFTRST);

	/* wait for OTG to ack reset */
	while (mmio_read_32(GRSTCTL) & GRSTCTL_CSFTRST);

	/* wait for OTG AHB master idle */
	while ((mmio_read_32(GRSTCTL) & GRSTCTL_AHBIDLE) == 0);

	VERBOSE("Reset usb controller done\n");

	usb_config();
	VERBOSE("exit usb_init()\n");
	return 0;
}

#define LOCK_STATE_LOCKED		0
#define LOCK_STATE_UNLOCKED		1
#define LOCK_STATE_RELOCKED		2

#define FB_MAX_FILE_SIZE		(256 * 1024 * 1024)

static struct ptentry *flash_ptn = NULL;

static void fb_getvar(char *cmdbuf)
{
	char response[64];
	char part_name[32];
	int bytes;
	struct ptentry *ptn = 0;

	if (!strncmp(cmdbuf + 7, "max-download-size", 17)) {
		bytes = sprintf(response, "OKAY0x%08x",
				FB_MAX_FILE_SIZE);
		response[bytes] = '\0';
		tx_status(response);
		rx_cmd();
	} else if (!strncmp(cmdbuf + 7, "partition-type:", 15)) {
		bytes = sprintf(part_name, "%s", cmdbuf + 22);
		ptn = find_ptn(part_name);
		if (ptn == NULL) {
			bytes = sprintf(response, "FAIL%s",
					"invalid partition");
			response[bytes] = '\0';
			flash_ptn = NULL;
		} else {
			bytes = sprintf(response, "OKAY");
			response[bytes] = '\0';
			flash_ptn = ptn;
		}
		tx_status(response);
		rx_cmd();
	} else if (!strncmp(cmdbuf + 7, "serialno", 8)) {
		bytes = sprintf(response, "OKAY%s",
				load_serialno());
		response[bytes] = '\0';
		tx_status(response);
		rx_cmd();
	} else {
		bytes = sprintf(response, "FAIL%s",
					"unknown var");
		response[bytes] = '\0';
		tx_status(response);
		rx_cmd();
	}
}

/* FIXME: do not support endptr yet */
static unsigned long strtoul(const char *nptr, char **endptr, int base)
{
	unsigned long step, data;
	int i;

	if (base == 0)
		step = 10;
	else if ((base < 2) || (base > 36)) {
		VERBOSE("%s: invalid base %d\n", __func__, base);
		return 0;
	} else
		step = base;

	for (i = 0, data = 0; ; i++) {
		if (nptr[i] == '\0')
			break;
		else if (!isalpha(nptr[i]) && !isdigit(nptr[i])) {
			VERBOSE("%s: invalid string %s at %d [%x]\n",
				__func__, nptr, i, nptr[i]);
			return 0;
		} else {
			data *= step;
			if (isupper(nptr[i]))
				data += nptr[i] - 'A' + 10;
			else if (islower(nptr[i]))
				data += nptr[i] - 'a' + 10;
			else if (isdigit(nptr[i]))
				data += nptr[i] - '0';
		}
	}
	return data;
}

static void fb_serialno(char *cmdbuf)
{
	struct random_serial_num random;

	generate_serialno(&random);
	flush_random_serialno((unsigned long)&random, sizeof(random));
}

static int fb_assigned_sn(char *cmdbuf)
{
	struct random_serial_num random;
	int ret;

	ret = assign_serialno(cmdbuf, &random);
	if (ret < 0)
		return ret;
	flush_random_serialno((unsigned long)&random, sizeof(random));
	return 0;
}

#define FB_DOWNLOAD_BASE	0x20000000

static unsigned long fb_download_base, fb_download_size;

static void fb_download(char *cmdbuf)
{
	char response[64];
	int bytes;

	if (!flash_ptn) {
		bytes = sprintf(response, "FAIL%s",
				"invalid partition");
		response[bytes] = '\0';
		tx_status(response);
		rx_cmd();
	} else {
		rx_addr = FB_DOWNLOAD_BASE;
		rx_length = strtoul(cmdbuf + 9, NULL, 16);
		fb_download_base = rx_addr;
		fb_download_size = rx_length;
		if (rx_length > FB_MAX_FILE_SIZE) {
			bytes = sprintf(response, "FAIL%s",
					"file is too large");
			response[bytes] = '\0';
			tx_status(response);
			rx_cmd();
		} else {
			bytes = sprintf(response, "DATA%08x",
					rx_length);
			VERBOSE("start:0x%x, length:0x%x, res:%s\n",
				rx_addr, rx_length, response);
			response[bytes] = '\0';
			tx_status(response);
			rx_data();
		}
	}
}

static void fb_flash(char *cmdbuf)
{
	flush_user_images(cmdbuf + 6, fb_download_base, fb_download_size);
	tx_status("OKAY");
	rx_cmd();
}

static void fb_reboot(char *cmdbuf)
{
	/* Send the system reset request */
	mmio_write_32(AO_SC_SYS_STAT0, 0x48698284);

	wfi();
	panic();
}

static void usb_rx_cmd_complete(unsigned actual, int stat)
{
	if(stat != 0) return;

	if(actual > 4095)
		actual = 4095;
	cmdbuf[actual] = 0;

	INFO("cmd :%s\n",cmdbuf);

	if(memcmp(cmdbuf, (void *)"reboot", 6) == 0) {
		tx_status("OKAY");
		fb_reboot(cmdbuf);
		return;
	} else if (!memcmp(cmdbuf, (void *)"getvar:", 7)) {
		fb_getvar(cmdbuf);
		return;
	} else if (!memcmp(cmdbuf, (void *)"download:", 9)) {
		fb_download(cmdbuf);
		return;
	} else if(memcmp(cmdbuf, (void *)"erase:", 6) == 0) {
	} else if(memcmp(cmdbuf, (void *)"flash:", 6) == 0) {
		INFO("recog updatefile\n");
		fb_flash(cmdbuf);
		return;
	} else if(memcmp(cmdbuf, (void *)"boot", 4) == 0) {
		INFO(" - OKAY\n");

		return;
	} else if (memcmp(cmdbuf, (void *)"oem serialno", 12) == 0) {
		if (*(cmdbuf + 12) == '\0') {
			fb_serialno(cmdbuf);
			tx_status("OKAY");
			rx_cmd();
			return;
		} else if (memcmp(cmdbuf + 12, (void *)" set", 4) == 0) {
			if (fb_assigned_sn(cmdbuf + 16) == 0) {
				tx_status("OKAY");
				rx_cmd();
				return;
			}
		}
	} else if (memcmp(cmdbuf, (void *)"oem led", 7) == 0) {
		if ((*(cmdbuf + 7) >= '1') && (*(cmdbuf + 7) <= '4')) {
			int led;
			led = *(cmdbuf + 7) - '0';
			if (memcmp(cmdbuf + 8, (void *)" on", 3) == 0) {
				gpio_set_value(31 + led, 1);
				tx_status("OKAY");
				rx_cmd();
				return;
			} else if (memcmp(cmdbuf + 8, (void *)" off", 4) == 0) {
				gpio_set_value(31 + led, 0);
				tx_status("OKAY");
				rx_cmd();
				return;
			}
		}
	}

	tx_status("FAILinvalid command");
	rx_cmd();
}

static void usbloader_init(void)
{
	VERBOSE("enter usbloader_init\n");

	/*usb sw and hw init*/
	init_usb();

	/*alloc and init sth for transfer*/
	ep1in.num = BULK_IN_EP;
	ep1in.in = 1;
	ep1in.req = NULL;
	ep1in.maxpkt = MAX_PACKET_LEN;
	ep1in.next = &ep1in;
	ep1out.num = BULK_OUT_EP;
	ep1out.in = 0;
	ep1out.req = NULL;
	ep1out.maxpkt = MAX_PACKET_LEN;
	ep1out.next = &ep1out;
	cmdbuf = (char *)(rx_req.buf);

	VERBOSE("exit usbloader_init\n");
}

void usb_reinit()
{
	if (usb_need_reset)
	{
		usb_need_reset = 0;
		init_usb();
	}
}

void usb_download(void)
{
	usbloader_init();
	INFO("Enter downloading mode. Please run fastboot command on Host.\n");
	for (;;) {
		usb_poll();
		usb_reinit();
	}
}
