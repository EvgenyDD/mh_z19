#include "usb_core_dfu.h"
#include "usb_desc.h"
#include "usb_hw.h"
#include "usb_lib.h"

#ifdef USBD_CLASS_DFU

static void usbd_dfu_status_out(void) {}

static int usbd_dfu_get_iface_sett(uint8_t iface, uint8_t alt_sett)
{
	if(alt_sett > 0) return USB_UNSUPPORT; // 3
	if(iface > 1) return USB_UNSUPPORT;	   // 2
	return USB_SUCCESS;
}

void usb_poll(uint32_t diff_ms) { usbd_dfu_poll(diff_ms); }

DEVICE_PROP usb_prop = {
	usbd_dfu_init,
	usbd_dfu_reset,
	usbd_dfu_status_in,
	usbd_dfu_status_out,
	usbd_dfu_setup,
	usbd_dfu_get_iface_sett,
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