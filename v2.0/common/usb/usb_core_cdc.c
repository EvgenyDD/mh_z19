
#include "usb_core_cdc.h"
#include "usb_desc.h"
#include "usb_istr.h"
#include "usb_lib.h"
#include "usb_pwr.h"
#include <stdbool.h>

#if defined(USBD_CLASS_CDC) || defined(USBD_CLASS_COMPOSITE_DFU_CDC)

#define CDC_DATA_IN_PACKET_SIZE CDC_DATA_MAX_PACKET_SIZE
#define CDC_DATA_OUT_PACKET_SIZE CDC_DATA_MAX_PACKET_SIZE

#define CDC_IN_FRAME_INTERVAL 5 /* Number of frames between IN transfers */
#define APP_RX_DATA_SIZE 2048	/* Total size of IN buffer: APP_RX_DATA_SIZE*8/MAX_BAUDARATE*1000 should be > CDC_IN_FRAME_INTERVAL */

enum
{
	USB_CDC_IDLE = 0,
	USB_CDC_BUSY,
	USB_CDC_ZLP
};

static uint8_t cdc_cmd_buf[CDC_CMD_PACKET_SIZE];
static uint8_t cdc_rx_buf[CDC_DATA_MAX_PACKET_SIZE]; // CDC ->app
static uint8_t cdc_tx_buf[APP_RX_DATA_SIZE];
static uint32_t cdc_tx_buf_len = 0;

static uint32_t tx_push_ptr = 0; // app -> CDC
static uint32_t tx_pop_ptr = 0;	 // app -> CDC

static uint8_t cdc_tx_state = USB_CDC_IDLE;

static bool lock_cdc_tx = false;

uint8_t Request = 0;

static LINE_CODING linecoding = {
	115200, // baud rate
	0x00,	// stop bits: 1
	0x00,	// parity: none
	0x08	// N of bits: 8
};

static uint8_t *line_coding_get(uint16_t Length)
{
	if(Length) return (uint8_t *)&linecoding;
	pInformation->ep_info.wLength = sizeof(linecoding);
	return NULL;
}

static uint8_t *line_coding_set(uint16_t Length)
{
	if(Length) return (uint8_t *)&linecoding;
	pInformation->ep_info.wLength = sizeof(linecoding);
	return NULL;
}

void usb_cdc_rst_state(void)
{
	cdc_tx_buf_len = 0;
	tx_push_ptr = 0;
	tx_pop_ptr = 0;
	cdc_tx_state = USB_CDC_IDLE;
	lock_cdc_tx = false;
	Request = 0;
}

void usbd_cdc_init(void)
{
	usb_cdc_rst_state();
	pInformation->curr_cfg = 0;
	usb_power_on();
	USB_SIL_Init();
}

void usbd_cdc_reset(void)
{
	pInformation->curr_cfg = 0;
	pInformation->curr_iface = 0;
	pInformation->curr_feat = usb_cfg_desc[7];

	SetBTABLE(BTABLE_ADDRESS);

	SetEPType(ENDP0, EP_CONTROL); // EP0
	SetEPTxStatus(ENDP0, EP_TX_STALL);
	SetEPRxAddr(ENDP0, ENDP0_RXADDR);
	SetEPRxCount(ENDP0, usb_prop.max_pkt_size);
	SetEPTxAddr(ENDP0, ENDP0_TXADDR);
	SetEPTxCount(ENDP0, usb_prop.max_pkt_size);
	Clear_Status_Out(ENDP0);
	SetEPRxValid(ENDP0);

	SetEPType(ENDP1, EP_BULK); // EP1
	SetEPTxAddr(ENDP1, ENDP1_TXADDR);
	SetEPRxStatus(ENDP1, EP_RX_DIS);
	SetEPTxStatus(ENDP1, EP_TX_NAK);

	SetEPType(ENDP2, EP_INTERRUPT); // EP2
	SetEPTxAddr(ENDP2, ENDP2_TXADDR);
	SetEPRxStatus(ENDP2, EP_RX_DIS);
	SetEPTxStatus(ENDP2, EP_TX_NAK);

	SetEPType(ENDP3, EP_BULK); // EP3
	SetEPRxAddr(ENDP3, ENDP3_RXADDR);
	SetEPRxCount(ENDP3, CDC_DATA_MAX_PACKET_SIZE);
	SetEPRxStatus(ENDP3, EP_RX_VALID);
	SetEPTxStatus(ENDP3, EP_TX_DIS);

	set_dev_addr(0);
}

int usbd_cdc_setup(void)
{
	uint8_t *(*copy_data)(uint16_t) = NULL;

	if(usb_dev.USBwLength)
	{
		if(usb_dev.bRequest == GET_LINE_CODING)
		{
			if(Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) copy_data = line_coding_get;
		}
		else if(usb_dev.bRequest == SET_LINE_CODING)
		{
			if(Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) copy_data = line_coding_set;
			Request = SET_LINE_CODING;
		}
	}
	else
	{
		if(Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
		{
			if(usb_dev.bRequest == SET_COMM_FEATURE) return USB_SUCCESS;
			if(usb_dev.bRequest == SET_CONTROL_LINE_STATE) return USB_SUCCESS;
		}
		return USB_UNSUPPORT;
	}

	if(usb_dev.USBwLength)
	{
		if(copy_data == NULL) return USB_UNSUPPORT;
		pInformation->ep_info.copy_data = copy_data;
		pInformation->ep_info.wOffset = 0;
		(*copy_data)(0);
	}
	return USB_SUCCESS;
}

void usbd_cdc_status_in(void)
{
	if(Request == SET_LINE_CODING) Request = 0;
}

int usbd_cdc_push_data(const uint8_t *data, uint32_t size)
{
	int count = 0;
	for(uint32_t i = 0; i < size; i++)
	{
		cdc_tx_buf[tx_push_ptr++] = data[i];
		count++;
		if(tx_push_ptr == APP_RX_DATA_SIZE)
		{
			tx_push_ptr = 0;
			count = 0;
		}
	}
	return count;
}

void EP1_IN_Callback(void)
{
	if(cdc_tx_state == USB_CDC_BUSY)
	{
		if(cdc_tx_buf_len == 0)
		{
			cdc_tx_state = USB_CDC_IDLE;
		}
		else
		{
			uint16_t tx_ptr;
			uint16_t tx_size;
			if(cdc_tx_buf_len > CDC_DATA_IN_PACKET_SIZE)
			{
				tx_ptr = tx_pop_ptr;
				tx_size = CDC_DATA_IN_PACKET_SIZE;

				tx_pop_ptr += CDC_DATA_IN_PACKET_SIZE;
				cdc_tx_buf_len -= CDC_DATA_IN_PACKET_SIZE;
			}
			else
			{
				tx_ptr = tx_pop_ptr;
				tx_size = cdc_tx_buf_len;

				tx_pop_ptr += cdc_tx_buf_len;
				cdc_tx_buf_len = 0;
				if(tx_size == CDC_DATA_IN_PACKET_SIZE) cdc_tx_state = USB_CDC_ZLP;
			}

			UserToPMABufferCopy(&cdc_tx_buf[tx_ptr], ENDP1_TXADDR, tx_size); // prepare the available data buffer to be sent on IN endpoint
			SetEPTxCount(ENDP1, tx_size);
			SetEPTxValid(ENDP1);
			return;
		}
	}

	if(cdc_tx_state == USB_CDC_ZLP) // avoid any asynchronous transfer during ZLP
	{
		SetEPTxCount(ENDP1, 0); // send ZLP to indicate the end of the current transfer
		SetEPTxValid(ENDP1);
		cdc_tx_state = USB_CDC_IDLE;
	}
}

void EP3_OUT_Callback(void)
{
	uint16_t cnt = USB_SIL_Read(EP3_OUT, cdc_rx_buf);
	usbd_cdc_rx(cdc_rx_buf, cnt);
	SetEPRxValid(ENDP3); /* Enable the receive of data on EP3 */
}

static void handle_usb_async_xfer(void)
{
	if(lock_cdc_tx == false && cdc_tx_state == USB_CDC_IDLE)
	{
		if(tx_pop_ptr == APP_RX_DATA_SIZE) tx_pop_ptr = 0;

		if(tx_pop_ptr == tx_push_ptr)
		{
			cdc_tx_state = USB_CDC_IDLE;
			return;
		}

		cdc_tx_buf_len = tx_pop_ptr > tx_push_ptr ? APP_RX_DATA_SIZE - tx_pop_ptr : tx_push_ptr - tx_pop_ptr;

		uint16_t tx_ptr;
		uint16_t tx_size;
		if(cdc_tx_buf_len > CDC_DATA_IN_PACKET_SIZE)
		{
			tx_ptr = tx_pop_ptr;
			tx_size = CDC_DATA_IN_PACKET_SIZE;
			tx_pop_ptr += CDC_DATA_IN_PACKET_SIZE;
			cdc_tx_buf_len -= CDC_DATA_IN_PACKET_SIZE;
			cdc_tx_state = USB_CDC_BUSY;
		}
		else
		{
			tx_ptr = tx_pop_ptr;
			tx_size = cdc_tx_buf_len;
			tx_pop_ptr += cdc_tx_buf_len;
			cdc_tx_buf_len = 0;
			cdc_tx_state = tx_size == CDC_DATA_IN_PACKET_SIZE ? USB_CDC_ZLP : USB_CDC_BUSY;
		}
		UserToPMABufferCopy(&cdc_tx_buf[tx_ptr], ENDP1_TXADDR, tx_size);
		SetEPTxCount(ENDP1, tx_size);
		SetEPTxValid(ENDP1);
	}
}

void SOF_Callback(void)
{
	static uint32_t frame_count = 0;
	if(usb_prop.bDeviceState == CONFIGURED)
	{
		if(frame_count++ == CDC_IN_FRAME_INTERVAL)
		{
			frame_count = 0;
			handle_usb_async_xfer();
		}
	}
}

void usbd_cdc_lock(void) { lock_cdc_tx = true; }
void usbd_cdc_unlock(void) { lock_cdc_tx = false; }

#endif