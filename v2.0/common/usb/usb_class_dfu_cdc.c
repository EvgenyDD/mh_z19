#include "usb_core_cdc.h"
#include "usb_core_dfu.h"
#include "usb_desc.h"
#include "usb_hw.h"
#include "usb_lib.h"
#include "usb_pwr.h"

#ifdef USBD_CLASS_COMPOSITE_DFU_CDC

static void usbd_dfu_cdc_init(void)
{
	pInformation->curr_cfg = 0;
	usb_power_on();
	USB_SIL_Init();
}

static void usbd_dfu_cdc_reset(void)
{
	usb_cdc_rst_state();

	pInformation->curr_cfg = 0;
	pInformation->curr_iface = 0;
	pInformation->curr_feat = usb_cfg_desc[7];

	SetBTABLE(BTABLE_ADDRESS);

	SetEPType(ENDP0, EP_CONTROL); // EP0
	SetEPTxStatus(ENDP0, EP_TX_STALL);
	SetEPRxAddr(ENDP0, ENDP0_RXADDR);
	SetEPRxCount(ENDP0, usb_prop.max_pkt_size);
	SetEPTxAddr(ENDP0, ENDP0_TXADDR);
	SetEPRxCount(ENDP0, usb_prop.max_pkt_size);
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

static int usbd_dfu_cdc_setup(void)
{
	switch(usb_dev.bmRequestType & USB_REQ_RECIPIENT_MASK)
	{
	case USB_REQ_RECIPIENT_INTERFACE:
		if(usb_dev.USBwIndex0 == 0)
		{
			usbd_cdc_lock();
			return usbd_dfu_setup();
		}
		else
		{
			return usbd_cdc_setup();
		}

	case USB_REQ_RECIPIENT_ENDPOINT:
		return usbd_cdc_setup();

	default: break;
	}
	return USB_UNSUPPORT;
}

static void usbd_dfu_cdc_status_in(void)
{
	usbd_dfu_status_in();
	usbd_cdc_status_in();
}

static void usbd_dfu_cdc_status_out(void) {}

static int usbd_dfu_cdc_get_iface_sett(uint8_t iface, uint8_t alt_sett)
{
	if(alt_sett > 0) return USB_UNSUPPORT;
	if(iface > 1) return USB_UNSUPPORT;
	return USB_SUCCESS;
}

void usb_poll(uint32_t diff_ms) { usbd_dfu_poll(diff_ms); }

DEVICE_PROP usb_prop = {
	usbd_dfu_cdc_init,
	usbd_dfu_cdc_reset,
	usbd_dfu_cdc_status_in,
	usbd_dfu_cdc_status_out,
	usbd_dfu_cdc_setup,
	usbd_dfu_cdc_get_iface_sett,
	usbd_usr_device_desc,
	usbd_get_cfg_desc,
	usbd_get_str_desc,
	usbd_usr_ext_prop_feat_desc,
	usbd_usr_ext_compat_id_feat_desc,
	USB_MAX_EP0_SIZE,
	UNCONNECTED,
	EP_NUM,
	1,
};
#endif