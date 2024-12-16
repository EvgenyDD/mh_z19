#include "usb_core_dfu.h"
#include "fw_header.h"
#include "platform.h"
#include "ret_mem.h"
#include "usb_desc.h"
#include "usb_istr.h"
#include "usb_lib.h"
#include "usb_pwr.h"
#include <stdbool.h>
#include <string.h>

#if defined(USBD_CLASS_DFU) || defined(USBD_CLASS_COMPOSITE_DFU_CDC)

#ifdef DFU_READS_CFG_SECTION
#include "config_system.h"
#endif

extern bool g_stay_in_boot;

#define USBD_BUF_SZ 1024
#define DISC_TO_MS 100

#if FW_TYPE == FW_LDR
#define FW_TARGET FW_APP
#define ADDR_ORIGIN ((uint32_t) & __app_start)
#define ADDR_END ((uint32_t) & __app_end)
#elif FW_TYPE == FW_APP
#define FW_TARGET FW_LDR
#define ADDR_ORIGIN ((uint32_t) & __ldr_start)
#define ADDR_END ((uint32_t) & __ldr_end)
#endif

static uint32_t cnt_till_reset = 0;
static uint8_t fw_sts[3], fw_type = FW_TYPE, fw_index_sel = 0;
static volatile bool dl_pending = false;
static bool dwnload_was = false;
static uint16_t tgt_upload_len = 0;
static uint32_t wlength = 0;
static uint8_t dfu_buffer[USBD_BUF_SZ];

static struct
{
	bool pending;
	bool offset_received;
	uint32_t offset;
	uint32_t size;
} upload = {0};

uint8_t usbd_dfu_cfg_desc[USB_DFU_CONFIG_DESC_SIZ] = {
	0x09,						 // bLength: Configuration Descriptor size
	USB_DESC_TYPE_CONFIGURATION, // bDescriptorType
	USB_DFU_CONFIG_DESC_SIZ,	 // wTotalLength
	0x00,						 //
	0x01,						 // bNumInterfaces: 1 interface
	0x01,						 // bConfigurationValue: Configuration value
	0x02,						 // iConfiguration: Index of string descriptor describing the configuration
	0xC0,						 // bmAttributes: bus powered and Supports Remote Wakeup
	(100 / 2),					 // max power 100 mA: this current is used for detecting Vbus

	// ******* Descriptor of DFU interface 0 Alternate setting 0
	USBD_DFU_IF_DESC(0), // This interface is mandatory for all devices

	// ******* DFU Functional Descriptor
	0x09,						   // blength = 9 Bytes
	USB_DESC_TYPE_DFU,			   // DFU Functional Descriptor
	0x0B,						   /* bmAttribute
										 bitCanDnload             = 1      (bit 0)
										 bitCanUpload             = 1      (bit 1)
										 bitManifestationTolerant = 0      (bit 2)
										 bitWillDetach            = 1      (bit 3)
										 Reserved                          (bit4-6)
										 bitAcceleratedST         = 0      (bit 7) */
	255,						   // detach timeout= 255 ms
	0x00,						   //
	TRANSFER_SIZE_BYTES(XFERSIZE), // WARNING: In DMA mode the multiple MPS packets feature is still not supported ==> when using DMA XFERSIZE should be 64
	0x1A,						   // bcdDFUVersion
	0x01,
};

void usbd_dfu_init(void)
{
	pInformation->curr_cfg = 0;
	usb_power_on();
	USB_SIL_Init();
}

void usbd_dfu_reset(void)
{
	usb_dev.curr_cfg = 0;
	pInformation->curr_feat = usb_cfg_desc[7];

	_SetBTABLE(BTABLE_ADDRESS);

	SetEPType(ENDP0, EP_CONTROL); // EP0
	SetEPTxStatus(ENDP0, EP_TX_NAK);
	SetEPRxAddr(ENDP0, ENDP0_RXADDR);
	SetEPRxCount(ENDP0, usb_prop.max_pkt_size);
	SetEPTxAddr(ENDP0, ENDP0_TXADDR);
	SetEPTxCount(ENDP0, usb_prop.max_pkt_size);
	Clear_Status_Out(ENDP0);
	SetEPRxValid(ENDP0);

	set_dev_addr(0);
}

void usbd_dfu_status_in(void) // data downloaded host->device
{
	switch(usb_dev.bRequest)
	{
	case DFU_UPLOAD:
		if(upload.pending)
		{
			memcpy(&upload.offset, &dfu_buffer[0], 4);
			memcpy(&upload.size, &dfu_buffer[4], 4);
			upload.offset_received = true;
		}
		break;

	case DFU_DNLOAD:
		if(dl_pending)
		{
			g_stay_in_boot = true;
			uint32_t addr_off, size_to_write = usb_dev.USBwLength - 4;
			memcpy(&addr_off, &dfu_buffer[0], 4);
			int sts;
#ifdef DFU_READS_CFG_SECTION
			if(fw_index_sel == FW_APP + 1)
			{
				if(addr_off > CFG_END - CFG_ORIGIN || CFG_END - CFG_ORIGIN - size_to_write < addr_off)
				{
					dl_pending = false;
					usb_dev.state = STALLED;
					return;
				}
				if(addr_off == 0) platform_flash_erase_flag_reset_sect_cfg();
				sts = platform_flash_write(CFG_ORIGIN + addr_off, &dfu_buffer[4], size_to_write);
			}
			else
#endif
			{
				if(addr_off > ADDR_END - ADDR_ORIGIN || ADDR_END - ADDR_ORIGIN - size_to_write < addr_off)
				{
					dl_pending = false;
					usb_dev.state = STALLED;
					return;
				}
				if(addr_off == 0) platform_flash_erase_flag_reset();
				sts = platform_flash_write(ADDR_ORIGIN + addr_off, &dfu_buffer[4], size_to_write);
			}
			dwnload_was = true;
			dl_pending = false;
			if(sts) usb_dev.state = STALLED;
		}
	default: break;
	}
}

static uint8_t *cb_upload(uint16_t Length)
{
	if(Length == 0) pInformation->ep_info.wLength = tgt_upload_len;
	return dfu_buffer + usb_dev.ep_info.wOffset;
}

static uint8_t *cb_download(uint16_t Length)
{
	// uint32_t wBlockNum = (uint16_t)usb_dev.wValue.bw.bb1 * 0x100 + (uint16_t)usb_dev.wValue.bw.bb0;
	wlength = (uint16_t)usb_dev.wLength.bw.bb0 * 0x100 + (uint16_t)usb_dev.wLength.bw.bb1;
	if(Length == 0) pInformation->ep_info.wLength = wlength;
	return dfu_buffer + usb_dev.ep_info.wOffset;
}

int usbd_dfu_setup(void)
{
	uint16_t wValue = (usb_dev.USBwValue1 << 8) | usb_dev.USBwValue0;
	uint8_t *(*copy_data)(uint16_t) = NULL;
	const bool is_upload = usb_dev.bmRequestType & 0x80;
	if(Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
	{
		switch(usb_dev.bRequest)
		{
		case DFU_DETACH: cnt_till_reset = DISC_TO_MS; break;

		case DFU_CLRSTATUS:
			g_stay_in_boot = true;
			if(usb_dev.USBwLength) return USB_UNSUPPORT;
			break;

		case DFU_GETSTATE:
			g_stay_in_boot = true;
			if(!usb_dev.USBwLength || !is_upload) return USB_UNSUPPORT;
			dfu_buffer[0] = fw_type;
			tgt_upload_len = sizeof(fw_type);
			copy_data = cb_upload;
			break;

		case DFU_GETSTATUS:
			g_stay_in_boot = true;
			if(!usb_dev.USBwLength || !is_upload) return USB_UNSUPPORT;
			fw_header_check_all();
			fw_sts[0] = g_fw_info[0].locked;
			fw_sts[1] = g_fw_info[1].locked;
			fw_sts[2] = g_fw_info[2].locked;
			memcpy(dfu_buffer, fw_sts, sizeof(fw_sts));
			tgt_upload_len = sizeof(fw_sts);
			copy_data = cb_upload;
			break;

		case DFU_UPLOAD:
			if(!usb_dev.USBwLength) return USB_UNSUPPORT;
			if(is_upload)
			{
				if(upload.pending &&
				   upload.offset_received &&
#ifdef DFU_READS_CFG_SECTION
				   wValue < FW_COUNT + 1 &&
#else
				   wValue < FW_COUNT &&
#endif
				   upload.size < USBD_BUF_SZ)
				{
					upload.pending = upload.offset_received = false;
#ifdef DFU_READS_CFG_SECTION
					if(wValue == FW_APP + 1)
					{
						if(config_validate() == CONFIG_STS_OK)
						{
							uint32_t size_to_send = config_get_size() - upload.offset;
							if(size_to_send > upload.size) size_to_send = upload.size;
							memcpy(dfu_buffer, (uint8_t *)CFG_ORIGIN + upload.offset, size_to_send);
							tgt_upload_len = size_to_send;
						}
						else
						{
							tgt_upload_len = 0;
						}
					}
					else
#endif // DFU_READS_CFG_SECTION
					{
						if(g_fw_info[wValue].locked ||
						   upload.offset >= g_fw_info[wValue].size)
						{
							tgt_upload_len = 0;
						}
						else
						{
							uint32_t size_to_send = g_fw_info[wValue].size - upload.offset;
							if(size_to_send > upload.size) size_to_send = upload.size;
							memcpy(dfu_buffer, (uint8_t *)g_fw_info[wValue].addr + upload.offset, size_to_send);
							tgt_upload_len = size_to_send;
						}
					}
				}
				else
				{
					return USB_UNSUPPORT;
				}
			}
			else // receive fw index, offset and size to read
			{
				if(wValue >= FW_COUNT + 1 || usb_dev.USBwLength != 4 + 4)
				{
					return USB_UNSUPPORT;
				}
				else
				{
					upload.pending = true;
					upload.offset_received = false;
				}
			}
			copy_data = is_upload ? cb_upload : cb_download;
			break;

		case DFU_DNLOAD:
			if(!usb_dev.USBwLength) return USB_UNSUPPORT;
			if(usb_dev.USBwLength > sizeof(dfu_buffer) || usb_dev.USBwLength < 1 + 4) return USB_UNSUPPORT;
			fw_index_sel = wValue;
			dl_pending = true;
			copy_data = cb_download;
			break;

		default: return USB_UNSUPPORT;
		}
	}
	else
	{
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

void usbd_dfu_poll(uint32_t diff_ms)
{
	if(cnt_till_reset)
	{
		if(cnt_till_reset <= diff_ms)
		{
			if(!dwnload_was) ret_mem_set_bl_stuck(true);
			platform_reset();
		}
		if(cnt_till_reset > diff_ms) cnt_till_reset -= diff_ms;
	}
}
#endif