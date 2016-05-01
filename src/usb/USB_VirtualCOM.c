/*
 * This file is part of eVic SDK.
 *
 * eVic SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * eVic SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eVic SDK.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 ReservedField
 */

/**
 * \file
 * This is an implementation of a USB CDC Virtual COM port.
 */

#include <string.h>
#include <stdlib.h>
#include <M451Series.h>
#include <USB_VirtualCOM.h>
#include <USB.h>

/* Endpoints */
#define USB_VCOM_CTRL_IN_EP  EP0
#define USB_VCOM_CTRL_OUT_EP EP1
#define USB_VCOM_BULK_IN_EP  EP2
#define USB_VCOM_BULK_OUT_EP EP3
#define USB_VCOM_INT_IN_EP   EP4

/* INTSTS masks */
#define USB_VCOM_CTRL_IN_INTSTS  USBD_INTSTS_EP0
#define USB_VCOM_CTRL_OUT_INTSTS USBD_INTSTS_EP1
#define USB_VCOM_BULK_IN_INTSTS  USBD_INTSTS_EP2
#define USB_VCOM_BULK_OUT_INTSTS USBD_INTSTS_EP3
#define USB_VCOM_INT_IN_INTSTS   USBD_INTSTS_EP4

/* Endpoint numbers */
#define USB_VCOM_CTRL_IN_EP_NUM  0x00
#define USB_VCOM_CTRL_OUT_EP_NUM 0x00
#define USB_VCOM_BULK_IN_EP_NUM  0x01
#define USB_VCOM_BULK_OUT_EP_NUM 0x02
#define USB_VCOM_INT_IN_EP_NUM   0x03

/* Maximum packet sizes */
#define USB_VCOM_CTRL_IN_MAX_PKT_SIZE  64
#define USB_VCOM_CTRL_OUT_MAX_PKT_SIZE 64
#define USB_VCOM_BULK_IN_MAX_PKT_SIZE  64
#define USB_VCOM_BULK_OUT_MAX_PKT_SIZE 64
#define USB_VCOM_INT_IN_MAX_PKT_SIZE   8

/* Buffers */
#define USB_VCOM_SETUP_BUF_BASE    0
#define USB_VCOM_SETUP_BUF_SIZE    8
#define USB_VCOM_CTRL_IN_BUF_BASE  (USB_VCOM_SETUP_BUF_BASE + USB_VCOM_SETUP_BUF_SIZE)
#define USB_VCOM_CTRL_IN_BUF_SIZE  USB_VCOM_CTRL_IN_MAX_PKT_SIZE
#define USB_VCOM_CTRL_OUT_BUF_BASE (USB_VCOM_CTRL_IN_BUF_BASE + USB_VCOM_CTRL_IN_BUF_SIZE)
#define USB_VCOM_CTRL_OUT_BUF_SIZE USB_VCOM_CTRL_OUT_MAX_PKT_SIZE
#define USB_VCOM_BULK_IN_BUF_BASE  (USB_VCOM_CTRL_OUT_BUF_BASE + USB_VCOM_CTRL_OUT_BUF_SIZE)
#define USB_VCOM_BULK_IN_BUF_SIZE  USB_VCOM_BULK_IN_MAX_PKT_SIZE
#define USB_VCOM_BULK_OUT_BUF_BASE (USB_VCOM_BULK_IN_BUF_BASE + USB_VCOM_BULK_IN_BUF_SIZE)
#define USB_VCOM_BULK_OUT_BUF_SIZE USB_VCOM_BULK_OUT_MAX_PKT_SIZE
#define USB_VCOM_INT_IN_BUF_BASE   (USB_VCOM_BULK_OUT_BUF_BASE + USB_VCOM_BULK_OUT_BUF_SIZE)
#define USB_VCOM_INT_IN_BUF_SIZE   USB_VCOM_INT_IN_MAX_PKT_SIZE

/* CDC class specific requests */
#define USB_VCOM_REQ_SET_LINE_CODE          0x20
#define USB_VCOM_REQ_GET_LINE_CODE          0x21
#define USB_VCOM_REQ_SET_CONTROL_LINE_STATE 0x22

/* USB VID and PID */
#define USB_VCOM_VID 0x0416
#define USB_VCOM_PID 0xB002

/* Virtual COM index */
#define USB_VCOM_INDEX 0

/* RX buffer size */
#define USB_VCOM_RX_BUF_SIZE 128

/* Mask for DTR bit in line state */
#define USB_VCOM_LINESTATE_MASK_DTR (1 << 0)

/**
 * USB device descriptor.
 */
static const uint8_t USB_VirtualCOM_DeviceDesc[LEN_DEVICE] = {
	LEN_DEVICE,  /* bLength */
	DESC_DEVICE, /* bDescriptorType */
	0x10, 0x01,  /* bcdUSB */
	0x02,        /* bDeviceClass */
	0x00,        /* bDeviceSubClass */
	0x00,        /* bDeviceProtocol */
	/* bMaxPacketSize0 */
	USB_VCOM_CTRL_IN_MAX_PKT_SIZE,
	/* idVendor */
	USB_VCOM_VID & 0x00FF,
	(USB_VCOM_VID & 0xFF00) >> 8,
	/* idProduct */
	USB_VCOM_PID & 0x00FF,
	(USB_VCOM_PID & 0xFF00) >> 8,
	0x00, 0x03,  /* bcdDevice */
	0x01,        /* iManufacture */
	0x02,        /* iProduct */
	0x03,        /* iSerialNumber */
	0x01         /* bNumConfigurations */
};

/**
 * USB configuration descriptor.
 */
static const uint8_t USB_VirtualCOM_ConfigDesc[0x43] = {
	LEN_CONFIG,     /* bLength */
	DESC_CONFIG,    /* bDescriptorType */
	0x43, 0x00,     /* wTotalLength    */
	0x02,           /* bNumInterfaces  */
	0x01,           /* bConfigurationValue */
	0x00,           /* iConfiguration */
	0xC0,           /* bmAttributes */
	0x32,           /* bMaxPower */

	/* INTERFACE descriptor */
	LEN_INTERFACE,  /* bLength */
	DESC_INTERFACE, /* bDescriptorType */
	0x00,           /* bInterfaceNumber */
	0x00,           /* bAlternateSetting */
	0x01,           /* bNumEndpoints */
	0x02,           /* bInterfaceClass */
	0x02,           /* bInterfaceSubClass */
	0x01,           /* bInterfaceProtocol */
	0x00,           /* iInterface */

	/* Communication Class Specified INTERFACE descriptor */
	0x05,           /* Size of the descriptor, in bytes */
	0x24,           /* CS_INTERFACE descriptor type */
	0x00,           /* Header functional descriptor subtype */
	0x10, 0x01,     /* Communication device compliant to the communication spec. ver. 1.10 */

	/* Communication Class Specified INTERFACE descriptor */
	0x05,           /* Size of the descriptor, in bytes */
	0x24,           /* CS_INTERFACE descriptor type */
	0x01,           /* Call management functional descriptor */
	0x00,           /* BIT0: Whether device handle call management itself. */
	/* BIT1: Whether device can send/receive call management information over a Data Class Interface 0 */
	0x01,           /* Interface number of data class interface optionally used for call management */

	/* Communication Class Specified INTERFACE descriptor */
	0x04,           /* Size of the descriptor, in bytes */
	0x24,           /* CS_INTERFACE descriptor type */
	0x02,           /* Abstract control management functional descriptor subtype */
	0x00,           /* bmCapabilities */

	/* Communication Class Specified INTERFACE descriptor */
	0x05,           /* bLength */
	0x24,           /* bDescriptorType: CS_INTERFACE descriptor type */
	0x06,           /* bDescriptorSubType */
	0x00,           /* bMasterInterface */
	0x01,           /* bSlaveInterface0 */

	/* ENDPOINT descriptor */
	LEN_ENDPOINT,                        /* bLength */
	DESC_ENDPOINT,                       /* bDescriptorType */
	(EP_INPUT | USB_VCOM_INT_IN_EP_NUM), /* bEndpointAddress */
	EP_INT,                              /* bmAttributes */
	USB_VCOM_INT_IN_MAX_PKT_SIZE, 0x00,  /* wMaxPacketSize */
	0x01,                                /* bInterval */

	/* INTERFACE descriptor */
	LEN_INTERFACE,  /* bLength */
	DESC_INTERFACE, /* bDescriptorType */
	0x01,           /* bInterfaceNumber */
	0x00,           /* bAlternateSetting */
	0x02,           /* bNumEndpoints  */
	0x0A,           /* bInterfaceClass */
	0x00,           /* bInterfaceSubClass */
	0x00,           /* bInterfaceProtocol */
	0x00,           /* iInterface */

	/* ENDPOINT descriptor */
	LEN_ENDPOINT,                         /* bLength */
	DESC_ENDPOINT,                        /* bDescriptorType */
	(EP_INPUT | USB_VCOM_BULK_IN_EP_NUM), /* bEndpointAddress */
	EP_BULK,                              /* bmAttributes */
	USB_VCOM_BULK_IN_MAX_PKT_SIZE, 0x00,  /* wMaxPacketSize */
	0x00,                                 /* bInterval */

	/* ENDPOINT descriptor */
	LEN_ENDPOINT,                           /* bLength */
	DESC_ENDPOINT,                          /* bDescriptorType */
	(EP_OUTPUT | USB_VCOM_BULK_OUT_EP_NUM), /* bEndpointAddress */
	EP_BULK,                                /* bmAttributes */
	USB_VCOM_BULK_OUT_MAX_PKT_SIZE, 0x00,   /* wMaxPacketSize */
	0x00,                                   /* bInterval */
};

/**
 * USB language string descriptor.
 */
static const uint8_t USB_VirtualCOM_StringLang[4] = {
	4,           /* bLength */
	DESC_STRING, /* bDescriptorType */
	0x09, 0x04
};

/**
 * USB vendor string descriptor.
 */
static const uint8_t USB_VirtualCOM_StringVendor[16] = {
	16,          /* bLength */
	DESC_STRING, /* bDescriptorType */
	'N', 0, 'u', 0, 'v', 0, 'o', 0, 't', 0, 'o', 0, 'n', 0
};

/**
 * USB product string descriptor.
 */
static const uint8_t USB_VirtualCOM_StringProduct[32] = {
	32,          /* bLength */
	DESC_STRING, /* bDescriptorType */
	'U', 0, 'S', 0, 'B', 0, ' ', 0, 'V', 0, 'i', 0, 'r', 0, 't', 0, 'u', 0, 'a', 0, 'l', 0, ' ', 0, 'C', 0, 'O', 0, 'M', 0
};

/**
 * USB serial string descriptor.
 */
static const uint8_t USB_VirtualCOM_StringSerial[26] = {
	26,          /* bLength */
	DESC_STRING, /* bDescriptorType */
	'A', 0, '0', 0, '2', 0, '0', 0, '1', 0, '4', 0, '0', 0, '9', 0, '0', 0, '3', 0, '0', 0, '5', 0
};

/**
 * USB string descriptor pointers.
 */
static const uint8_t *USB_VirtualCOM_StringDescs[4] = {
	USB_VirtualCOM_StringLang,
	USB_VirtualCOM_StringVendor,
	USB_VirtualCOM_StringProduct,
	USB_VirtualCOM_StringSerial
};

/**
 * USBD information structure.
 */
static const S_USBD_INFO_T USB_VirtualCOM_UsbdInfo = {
	USB_VirtualCOM_DeviceDesc,
	USB_VirtualCOM_ConfigDesc,
	USB_VirtualCOM_StringDescs,
	NULL,
	NULL,
	NULL
};

/**
 * Structure for USB CDC line coding data.
 */
typedef struct {
	/**< Data terminal rate, in bits per second. */
	uint32_t dwDTERate;
	/**< Stop bits. */
	uint8_t bCharFormat;
	/**< Parity. */
	uint8_t bParityType;
	/**< Data bits. */
	uint8_t bDataBits;
} USB_VirtualCOM_LineCoding_t;
#define USB_VirtualCOM_LineCoding_t_SIZE 7

/**
 * Structure for TX transfer queue element.
 */
typedef struct USB_VirtualCOM_TxTransfer {
	/**< Pointer to next transfer, or NULL if this is the last one. */
	struct USB_VirtualCOM_TxTransfer *next;
	/**< Data buffer size. */
	uint32_t size;
	/**< Number of bytes transferred so far. */
	uint32_t txSize;
	/**< Data buffer (variable length). */
	uint8_t buffer[];
} USB_VirtualCOM_TxTransfer_t;

/**
 * Structure for TX transfer queue.
 */
typedef struct {
	/**< First element (or NULL if list is empty). */
	USB_VirtualCOM_TxTransfer_t *head;
	/**< Last element (or NULL if list is empty). */
	USB_VirtualCOM_TxTransfer_t *tail;
} USB_VirtualCOM_TxQueue_t;

/**
 * Structure for RX ring buffer.
 */
typedef struct {
	/**< Current reading position. */
	uint16_t readIndex;
	/**< Current writing position. */
	uint16_t writeIndex;
	/**< Number of used bytes in the buffer. */
	uint16_t dataSize;
	/**< Ring buffer. */
	uint8_t buffer[USB_VCOM_RX_BUF_SIZE];
} USB_VirtualCOM_RxBuffer_t;

/**
 * Line coding data.
 * This is ignored, but stored for GET_LINE_CODE.
 * Default values: 115200 bauds, 1 stop bit, no parity, 8 data bits.
 */
static USB_VirtualCOM_LineCoding_t USB_VirtualCOM_lineCoding = {115200, 0, 0, 8};

/**
 * Line state set by host with SET_CONTROL_LINE_STATE.
 */
static uint16_t USB_VirtualCOM_lineState;

/**
 * TX transfer queue.
 */
static volatile USB_VirtualCOM_TxQueue_t USB_VirtualCOM_txQueue;

/**
 * True when the host is ready for a bulk in, but no data
 * was available last time the handler was called.
 */
static volatile uint8_t USB_VirtualCOM_bulkInWaiting;

/**
 * RX ring buffer.
 */
static volatile USB_VirtualCOM_RxBuffer_t USB_VirtualCOM_rxBuffer;

/**
 * RX callback function pointer.
 * NULL when not used.
 */
static volatile USB_VirtualCOM_RxCallback_t USB_VirtualCOM_rxCallbackPtr;

/**
 * True if send mode is asynchronous, false if synchronous.
 */
static uint8_t USB_VirtualCOM_isAsync;

/**
 * Write data to the RX ring buffer.
 * If the data size exceeds current buffer capacity, excess
 * data will be discarded.
 * This is an internal function.
 *
 * @param src  Source buffer.
 * @param size Data size.
 *
 * @return Number of bytes actually written.
 */
static uint16_t USB_VirtualCOM_RxBuffer_Write(uint8_t *src, uint16_t size) {
	uint16_t copySize1, copySize2;

	// Discard overflowing data
	size = Minimum(USB_VCOM_RX_BUF_SIZE - USB_VirtualCOM_rxBuffer.dataSize, size);

	// Copy data to write index up to buffer end
	copySize1 = Minimum(USB_VCOM_RX_BUF_SIZE - USB_VirtualCOM_rxBuffer.writeIndex, size);
	if(copySize1 == 0) {
		return 0;
	}
	USBD_MemCopy((uint8_t *) (USB_VirtualCOM_rxBuffer.buffer + USB_VirtualCOM_rxBuffer.writeIndex),
		src, copySize1);

	// Copy data (if any) to buffer start
	if(copySize1 != size) {
		copySize2 = size - copySize1;
		USBD_MemCopy((uint8_t *) USB_VirtualCOM_rxBuffer.buffer, src + copySize1, copySize2);
	}

	// Update index and used size
	USB_VirtualCOM_rxBuffer.writeIndex = (USB_VirtualCOM_rxBuffer.writeIndex + size) % USB_VCOM_RX_BUF_SIZE;
	USB_VirtualCOM_rxBuffer.dataSize += size;

	return size;
}

/**
 * Reads data from the RX ring buffer.
 * If the data size exceeds currently available bytes, it will
 * be reduced.
 * This is an internal function.
 *
 * @param dst  Destination buffer.
 * @param size Data size.
 *
 * @return Number of bytes actually read.
 */
static uint16_t USB_VirtualCOM_RxBuffer_Read(uint8_t *dst, uint16_t size) {
	uint16_t copySize1, copySize2;

	// Do not read more than is available
	size = Minimum(USB_VirtualCOM_rxBuffer.dataSize, size);

	// Copy data from read index up to buffer end
	copySize1 = Minimum(USB_VCOM_RX_BUF_SIZE - USB_VirtualCOM_rxBuffer.readIndex, size);
	if(copySize1 == 0) {
		return 0;
	}
	USBD_MemCopy(dst,
		(uint8_t *) (USB_VirtualCOM_rxBuffer.buffer + USB_VirtualCOM_rxBuffer.readIndex), copySize1);

	// Copy data (if any) from buffer start
	if(copySize1 != size) {
		copySize2 = size - copySize1;
		USBD_MemCopy(dst + copySize1, (uint8_t *) USB_VirtualCOM_rxBuffer.buffer, copySize2);
	}

	// Update index and used size
	USB_VirtualCOM_rxBuffer.readIndex = (USB_VirtualCOM_rxBuffer.readIndex + size) % USB_VCOM_RX_BUF_SIZE;
	USB_VirtualCOM_rxBuffer.dataSize -= size;

	return size;
}

/**
 * Copies a packet to the bulk IN buffer and sets the payload length.
 * USB_VirtualCOM_bulkInWaiting will be set to false.
 * This is an internal function.
 *
 * @param buffer Source buffer (can be NULL for zero-length packets).
 * @param size   Packet size. Must be <= USB_VCOM_BULK_IN_MAX_PKT_SIZE.
 */
static void USB_VirtualCOM_SendBulkInPayload(const uint8_t *buffer, uint32_t size) {
	USB_VirtualCOM_bulkInWaiting = 0;
	if(buffer != NULL && size != 0) {
		USBD_MemCopy((uint8_t *) (USBD_BUF_BASE + USB_VCOM_BULK_IN_BUF_BASE), (uint8_t *) buffer, size);
	}
	USBD_SET_PAYLOAD_LEN(USB_VCOM_BULK_IN_EP, size);
}

/**
 * Handler for bulk IN transfers.
 * This is an internal function.
 */
static void USB_VirtualCOM_HandleBulkIn() {
	USB_VirtualCOM_TxTransfer_t *transfer;
	uint32_t partialSize;

	// We are inside an interrupt, no need to worry about race conditions
	// This will be 0 only when a packet is actually transferred
	USB_VirtualCOM_bulkInWaiting = 1;

	transfer = USB_VirtualCOM_txQueue.head;
	if(transfer == NULL) {
		// Nothing to do
		return;
	}

	if(transfer->txSize == transfer->size) {
		if(transfer->size % USB_VCOM_BULK_IN_MAX_PKT_SIZE == 0) {
			// Buffer has been transferred, but size is a multiple of USB_VCOM_BULK_IN_MAX_PKT_SIZE
			// Send a zero packet
			USB_VirtualCOM_SendBulkInPayload(NULL, 0);
		}

		// Remove transfer from queue
		USB_VirtualCOM_txQueue.head = transfer->next;
		if(USB_VirtualCOM_txQueue.tail == transfer) {
			USB_VirtualCOM_txQueue.tail = NULL;
		}
		free(transfer);
	}
	else {
		// Transfer buffer, one packet at a time
		partialSize = Minimum(transfer->size - transfer->txSize, USB_VCOM_BULK_IN_MAX_PKT_SIZE);
		USB_VirtualCOM_SendBulkInPayload(transfer->buffer + transfer->txSize, partialSize);
		transfer->txSize += partialSize;
	}
}

/**
 * Handler for bulk OUT transfers.
 * This is an internal function.
 */
static void USB_VirtualCOM_HandleBulkOut() {
	uint16_t packetSize, writeSize;

	// Copy data from USB packet to RX ring buffer
	packetSize = USBD_GET_PAYLOAD_LEN(USB_VCOM_BULK_OUT_EP);
	writeSize = USB_VirtualCOM_RxBuffer_Write((uint8_t *) (USBD_BUF_BASE + USB_VCOM_BULK_OUT_BUF_BASE), packetSize);

	if(USB_VirtualCOM_rxCallbackPtr != NULL && writeSize != 0) {
		// Invoke callback
		USB_VirtualCOM_rxCallbackPtr();
	}

	// Ready for next bulk OUT
	USBD_SET_PAYLOAD_LEN(USB_VCOM_BULK_OUT_EP, USB_VCOM_BULK_OUT_MAX_PKT_SIZE);
}

/**
 * USB virtual COM interrupt handler.
 * This is an internal function.
 */
static void USB_VirtualCOM_IRQHandler() {
	uint32_t intSts, busState;

	intSts = USBD_GET_INT_FLAG();
	busState = USBD_GET_BUS_STATE();

	if(intSts & USBD_INTSTS_FLDET) {
		if(USBD_IS_ATTACHED()) {
			// USB plugged in
			USBD_ENABLE_USB();
		}
		else {
			// USB unplugged
			USBD_DISABLE_USB();
		}

		USB_VirtualCOM_lineState = 0;
	}
	else if(intSts & USBD_INTSTS_BUS) {
		if(busState & USBD_STATE_USBRST) {
			// Bus reset
			USBD_ENABLE_USB();
			USBD_SwReset();
		}
		else if(busState & USBD_STATE_SUSPEND) {
			// Suspend: USB enabled, PHY disabled
			USBD_DISABLE_PHY();
		}
		else if(busState & USBD_STATE_RESUME) {
			// Resume: USB enabled, PHY enabled
			USBD_ENABLE_USB();
		}

		USB_VirtualCOM_lineState = 0;
	}
	else if(intSts & USBD_INTSTS_USB) {
		if(intSts & USBD_INTSTS_SETUP) {
			// Clear IN/OUT data ready flag for control endpoints
			USBD_STOP_TRANSACTION(USB_VCOM_CTRL_IN_EP);
			USBD_STOP_TRANSACTION(USB_VCOM_CTRL_OUT_EP);

			// Handle setup packet
			USBD_ProcessSetupPacket();
		}
		else if(intSts & USB_VCOM_CTRL_IN_INTSTS) {
			// Handle control IN
			USBD_CtrlIn();
		}
		else if(intSts & USB_VCOM_CTRL_OUT_INTSTS) {
			// Handle control OUT
			USBD_CtrlOut();
		}
		else if(intSts & USB_VCOM_BULK_IN_INTSTS) {
			// Handle bulk IN
			USB_VirtualCOM_HandleBulkIn();
		}
		else if(intSts & USB_VCOM_BULK_OUT_INTSTS) {
			// Handle bulk OUT
			USB_VirtualCOM_HandleBulkOut();
		}
	}

	// Clear flag
	USBD_CLR_INT_FLAG(intSts);
}

/**
 * USB CDC class request handler.
 * This is an internal function.
 */
static void USB_VirtualCOM_HandleClassRequest() {
	USB_SetupPacket_t setupPacket;

	USBD_GetSetupPacket((uint8_t *) &setupPacket);

	if(setupPacket.bmRequestType & 0x80) {
		// Transfer direction: device to host
		switch(setupPacket.bRequest) {
			case USB_VCOM_REQ_GET_LINE_CODE:
				if(setupPacket.wIndex == USB_VCOM_INDEX) {
					// Copy line coding data to USB buffer
					USBD_MemCopy((uint8_t *) (USBD_BUF_BASE + USB_VCOM_CTRL_IN_BUF_BASE),
						(uint8_t *) &USB_VirtualCOM_lineCoding, USB_VirtualCOM_LineCoding_t_SIZE);
				}

				// Data stage
				USBD_SET_DATA1(USB_VCOM_CTRL_IN_EP);
				USBD_SET_PAYLOAD_LEN(USB_VCOM_CTRL_IN_EP, USB_VirtualCOM_LineCoding_t_SIZE);

				// Status stage
				USBD_PrepareCtrlOut(NULL, 0);
				break;
			default:
				// Setup error, stall the device
				USBD_SetStall(USB_VCOM_CTRL_IN_EP);
				USBD_SetStall(USB_VCOM_CTRL_OUT_EP);
				break;
		}
	}
	else {
		// Transfer direction: host to device
		switch(setupPacket.bRequest) {
			case USB_VCOM_REQ_SET_CONTROL_LINE_STATE:
				// Store new line state
				USB_VirtualCOM_lineState = setupPacket.wValue;

				// Status stage
				USBD_SET_DATA1(USB_VCOM_CTRL_IN_EP);
				USBD_SET_PAYLOAD_LEN(USB_VCOM_CTRL_IN_EP, 0);
				break;
			case USB_VCOM_REQ_SET_LINE_CODE:
				if(setupPacket.wIndex == USB_VCOM_INDEX) {
					// Prepare for line coding copy
					USBD_PrepareCtrlOut((uint8_t *) &USB_VirtualCOM_lineCoding, USB_VirtualCOM_LineCoding_t_SIZE);
				}

				// Status stage
				USBD_SET_DATA1(USB_VCOM_CTRL_IN_EP);
				USBD_SET_PAYLOAD_LEN(USB_VCOM_CTRL_IN_EP, 0);
				break;
			default:
				// Setup error, stall the device
				USBD_SetStall(USB_VCOM_CTRL_IN_EP);
				USBD_SetStall(USB_VCOM_CTRL_OUT_EP);
				break;
		}
	}
}

/**
 * Sends a data buffer synchronously over the USB virtual COM port.
 * This is an internal function.
 *
 * @param buf  Data buffer.
 * @param size Number of bytes to send.
 */
static void USB_VirtualCOM_SendSync(const uint8_t *buf, uint32_t size) {
	uint32_t partialSize;

	if(size == 0) {
		return;
	}

	while(size > 0) {
		// Wait for bulk in to be free
		while(!USB_VirtualCOM_bulkInWaiting);

		// Send one packet at a time
		partialSize = Minimum(size, USB_VCOM_BULK_IN_MAX_PKT_SIZE);
		USB_VirtualCOM_SendBulkInPayload(buf, partialSize);
		buf += partialSize;
		size -= partialSize;
	}

	if(partialSize == USB_VCOM_BULK_IN_MAX_PKT_SIZE) {
		// Size is a multiple of USB_VCOM_BULK_IN_MAX_PKT_SIZE
		// Send zero packet for termination
		while(!USB_VirtualCOM_bulkInWaiting);
		USB_VirtualCOM_SendBulkInPayload(NULL, 0);
	}

	while(!USB_VirtualCOM_bulkInWaiting);
}

/**
 * Sends a data buffer asynchronously over the USB virtual COM port.
 * This is an internal function.
 *
 * @param buf  Data buffer.
 * @param size Number of bytes to send.
 */
static void USB_VirtualCOM_SendAsync(const uint8_t *buf, uint32_t size) {
	USB_VirtualCOM_TxTransfer_t *transfer;
	uint32_t partialSize;

	if(size == 0) {
		return;
	}

	// Enter critical section
	__set_PRIMASK(1);

	partialSize = 0;
	if(USB_VirtualCOM_bulkInWaiting) {
		// Transfer first packet right away
		// If size is a multiple of USB_VCOM_BULK_IN_MAX_PKT_SIZE it will stay that way
		partialSize = Minimum(size, USB_VCOM_BULK_IN_MAX_PKT_SIZE);
		USB_VirtualCOM_SendBulkInPayload(buf, partialSize);
		buf += partialSize;
		size -= partialSize;
	}

	if(partialSize != 0 && partialSize < USB_VCOM_BULK_IN_MAX_PKT_SIZE) {
		// We already transferred the whole packet
		__set_PRIMASK(0);
		return;
	}

	// Allocate memory for transfer + buffer
	// If the buffer was exactly USB_VCOM_BULK_IN_MAX_PKT_SIZE bytes and
	// it has already been transferred, the transfer info still needs to be
	// allocated for the bulk IN handler to send the zero packet.
	transfer = (USB_VirtualCOM_TxTransfer_t *) malloc(sizeof(USB_VirtualCOM_TxTransfer_t) + size);
	if(transfer == NULL) {
		__set_PRIMASK(0);
		return;
	}

	// Fill in transfer data
	transfer->next = NULL;
	transfer->size = size;
	transfer->txSize = 0;
	if(size != 0) {
		memcpy(transfer->buffer, buf, size);
	}

	// Append transfer to queue
	if(USB_VirtualCOM_txQueue.head == NULL) {
		USB_VirtualCOM_txQueue.head = USB_VirtualCOM_txQueue.tail = transfer;
	}
	else {
		USB_VirtualCOM_txQueue.tail->next = transfer;
		USB_VirtualCOM_txQueue.tail = transfer;
	}

	// Exit critical section
	__set_PRIMASK(0);
}

void USB_VirtualCOM_Init() {
	// Initialize state
	USB_VirtualCOM_lineState = 0;
	USB_VirtualCOM_txQueue.head = USB_VirtualCOM_txQueue.tail = NULL;
	USB_VirtualCOM_bulkInWaiting = 1;
	USB_VirtualCOM_rxBuffer.readIndex = USB_VirtualCOM_rxBuffer.writeIndex = 0;
	USB_VirtualCOM_rxBuffer.dataSize = 0;
	USB_VirtualCOM_rxCallbackPtr = NULL;
	USB_VirtualCOM_isAsync = 0;

	// Open USB
	USBD_Open(&USB_VirtualCOM_UsbdInfo, USB_VirtualCOM_HandleClassRequest, NULL);

	// Initialize setup packet buffer
	USBD->STBUFSEG = USB_VCOM_SETUP_BUF_BASE;

	// Control IN endpoint
	USBD_CONFIG_EP(USB_VCOM_CTRL_IN_EP, USBD_CFG_CSTALL | USBD_CFG_EPMODE_IN | USB_VCOM_CTRL_IN_EP_NUM);
	USBD_SET_EP_BUF_ADDR(USB_VCOM_CTRL_IN_EP, USB_VCOM_CTRL_IN_BUF_BASE);
	// Control OUT endpoint
	USBD_CONFIG_EP(USB_VCOM_CTRL_OUT_EP, USBD_CFG_CSTALL | USBD_CFG_EPMODE_OUT | USB_VCOM_CTRL_OUT_EP_NUM);
	USBD_SET_EP_BUF_ADDR(USB_VCOM_CTRL_OUT_EP, USB_VCOM_CTRL_OUT_BUF_BASE);

	// Bulk IN endpoint
	USBD_CONFIG_EP(USB_VCOM_BULK_IN_EP, USBD_CFG_EPMODE_IN | USB_VCOM_BULK_IN_EP_NUM);
	USBD_SET_EP_BUF_ADDR(USB_VCOM_BULK_IN_EP, USB_VCOM_BULK_IN_BUF_BASE);
	// Bulk OUT endpoint
	USBD_CONFIG_EP(USB_VCOM_BULK_OUT_EP, USBD_CFG_EPMODE_OUT | USB_VCOM_BULK_OUT_EP_NUM);
	USBD_SET_EP_BUF_ADDR(USB_VCOM_BULK_OUT_EP, USB_VCOM_BULK_OUT_BUF_BASE);
	USBD_SET_PAYLOAD_LEN(USB_VCOM_BULK_OUT_EP, USB_VCOM_BULK_OUT_MAX_PKT_SIZE);

	// Interrupt IN endpoint
	USBD_CONFIG_EP(USB_VCOM_INT_IN_EP, USBD_CFG_EPMODE_IN | USB_VCOM_INT_IN_EP_NUM);
	USBD_SET_EP_BUF_ADDR(USB_VCOM_INT_IN_EP, USB_VCOM_INT_IN_BUF_BASE);

	// Start USB
	USBD_Start();

	// Enable USB interrupt
	NVIC_EnableIRQ(USBD_IRQn);
}

void USB_VirtualCOM_Send(const uint8_t *buf, uint32_t size) {
	if(!USBD_IS_ATTACHED() || !(USB_VirtualCOM_lineState & USB_VCOM_LINESTATE_MASK_DTR)) {
		// Don't send if no one is listening
		return;
	}

	if(USB_VirtualCOM_isAsync) {
		USB_VirtualCOM_SendAsync(buf, size);
	}
	else {
		USB_VirtualCOM_SendSync(buf, size);
	}
}

void USB_VirtualCOM_SendString(const char *str) {
	USB_VirtualCOM_Send((uint8_t *) str, strlen(str));
}

uint16_t USB_VirtualCOM_GetAvailableSize() {
	return USB_VirtualCOM_rxBuffer.dataSize;
}

uint16_t USB_VirtualCOM_Read(uint8_t *buf, uint16_t size) {
	uint16_t readSize;

	// Read in critical section
	__set_PRIMASK(1);
	readSize = USB_VirtualCOM_RxBuffer_Read(buf, size);
	__set_PRIMASK(0);

	return readSize;
}

void USB_VirtualCOM_SetRxCallback(USB_VirtualCOM_RxCallback_t callbackPtr) {
	// Atomic
	USB_VirtualCOM_rxCallbackPtr = callbackPtr;
}

void USB_VirtualCOM_SetAsyncMode(uint8_t isAsync) {
	USB_VirtualCOM_isAsync = isAsync;
}

USB_VirtualCOM_State_t USB_VirtualCOM_GetState() {
	if(!USBD_IS_ATTACHED()) {
		return DETACH;
	}

	if(USB_VirtualCOM_lineState & USB_VCOM_LINESTATE_MASK_DTR) {
		return READY;
	}

	return ATTACH;
}

/**
 * Global USB interrupt handler.
 * TODO: make this more flexible when more USB drivers are introduced.
 */
void USBD_IRQHandler() {
	// This handler should be fast.
	// Be careful with display debugging in here, as it may throw off USB timing.
	USB_VirtualCOM_IRQHandler();
}