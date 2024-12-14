/* @file    usb_core.c
 * @author  MCD Application Team
 * @version V4.1.0
 * @date    26-May-2017
 * @brief   Standard protocol processing (USB v2.0)
 */
#include "usb_def.h"
#include "usb_lib.h"
#include <stdbool.h>
#include <stddef.h>

#define ValBit(v, bit) ((v) & (1 << (bit)))
#define SetBit(v, bit) ((v) |= (1 << (bit)))
#define ClrBit(v, bit) ((v) &= ((1 << (bit)) ^ 255))

#define StatusInfo0 StatusInfo.bw.bb1 /* Reverse bb0 & bb1 */

static uint16_t_uint8_t StatusInfo;
static bool Data_Mul_MaxPacketSize = false;

uint8_t *Standard_GetConfiguration(uint16_t Length)
{
	if(Length == 0)
	{
		pInformation->ep_info.wLength = sizeof(pInformation->curr_cfg);
		return 0;
	}
	return (uint8_t *)&pInformation->curr_cfg;
}

uint8_t *Standard_GetInterface(uint16_t Length)
{
	if(Length == 0)
	{
		pInformation->ep_info.wLength = sizeof(pInformation->curr_alt_sett);
		return 0;
	}
	return (uint8_t *)&pInformation->curr_alt_sett;
}

uint8_t *Standard_GetStatus(uint16_t Length)
{
	if(Length == 0)
	{
		pInformation->ep_info.wLength = 2;
		return 0;
	}
	StatusInfo.w = 0; // Reset Status Information
	if(Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT))
	{
		uint8_t Feature = pInformation->curr_feat;							  // Get Device Status
		ValBit(Feature, 5) ? SetBit(StatusInfo0, 1) : ClrBit(StatusInfo0, 1); // Remote Wakeup enabled
		ValBit(Feature, 6) ? SetBit(StatusInfo0, 0) : ClrBit(StatusInfo0, 0); // Bus/Self - powered
		return (uint8_t *)&StatusInfo;
	}
	else if(Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT)) // Interface Status
	{
		return (uint8_t *)&StatusInfo;
	}
	else if(Type_Recipient == (STANDARD_REQUEST | ENDPOINT_RECIPIENT)) // Get EndPoint Status
	{
		uint8_t wIndex0 = pInformation->USBwIndex0;
		uint8_t Related_Endpoint = (wIndex0 & 0x0f);
		if(ValBit(wIndex0, 7))
		{
			if(_GetTxStallStatus(Related_Endpoint)) SetBit(StatusInfo0, 0); /* IN Endpoint stalled */
		}
		else
		{
			if(_GetRxStallStatus(Related_Endpoint)) SetBit(StatusInfo0, 0); /* OUT Endpoint stalled */
		}
		return (uint8_t *)&StatusInfo;
	}
	return NULL;
}

uint8_t *Standard_GetDescriptorData(uint16_t Length, uint8_t *data, uint16_t len)
{
	uint32_t wOffset = pInformation->ep_info.wOffset;
	if(Length == 0)
	{
		pInformation->ep_info.wLength = len - wOffset;
		return NULL;
	}
	return data + wOffset;
}

static uint8_t post0_process(void) // Stall the Endpoint 0 in case of error
{
	SetEPRxCount(ENDP0, usb_prop.max_pkt_size);
	if(pInformation->state == STALLED)
	{
		SaveRState = EP_RX_STALL;
		SaveTState = EP_TX_STALL;
	}
	return (pInformation->state == PAUSE);
}

static void data_stage_out(void)
{
	ENDPOINT_INFO *pEPinfo = &pInformation->ep_info;
	uint32_t save_rLength = pEPinfo->wLength;

	if(pEPinfo->copy_data && save_rLength)
	{
		uint32_t Length = pEPinfo->pkt_size;
		if(Length > save_rLength) Length = save_rLength;
		uint8_t *Buffer = (*pEPinfo->copy_data)(Length);
		pEPinfo->wLength -= Length;
		pEPinfo->wOffset += Length;
		PMAToUserBufferCopy(Buffer, GetEPRxAddr(ENDP0), Length);
	}

	if(pEPinfo->wLength != 0)
	{
		SaveRState = EP_RX_VALID; /* re-enable for next data reception */
		SetEPTxCount(ENDP0, 0);
		SaveTState = EP_TX_VALID; /* Expect the host to abort the data OUT stage */
	}
	/* Set the next State*/
	if(pEPinfo->wLength >= pEPinfo->pkt_size)
	{
		pInformation->state = OUT_DATA;
	}
	else
	{
		if(pEPinfo->wLength > 0)
		{
			pInformation->state = LAST_OUT_DATA;
		}
		else if(pEPinfo->wLength == 0)
		{
			/* USB spec: section 8.5.3.1 Reporting Status Results */
			pInformation->state = WAIT_STATUS_IN;
			(*pProperty->status_in)();
			if(pInformation->state == STALLED)
			{
				/* command failed to complete: in this case we should return a STALL */
				SaveRState = EP_RX_STALL;
				SaveTState = EP_TX_STALL;
			}
			else
			{
				_SetEPTxCount(ENDP0, 0);
				SaveTState = EP_TX_VALID; /* command completed successfully: send a zero-length packet during status stage */
			}
		}
	}
}

static void data_stage_in(void)
{
	ENDPOINT_INFO *pEPinfo = &pInformation->ep_info;
	uint32_t save_wLength = pEPinfo->wLength;
	uint32_t state = pInformation->state;

	if((save_wLength == 0) && (state == LAST_IN_DATA))
	{
		if(Data_Mul_MaxPacketSize == true)
		{
			_SetEPTxCount(ENDP0, 0);
			SaveTState = EP_TX_VALID; /* No more data to send and empty packet */
			state = LAST_IN_DATA;
			Data_Mul_MaxPacketSize = false;
		}
		else
		{
			state = WAIT_STATUS_OUT; /* No more data to send so STALL the TX Status*/
			SaveTState = EP_TX_STALL;
		}
		goto Expect_Status_Out;
	}

	uint32_t Length = pEPinfo->pkt_size;
	state = (save_wLength <= Length) ? LAST_IN_DATA : IN_DATA;
	if(Length > save_wLength) Length = save_wLength;

	UserToPMABufferCopy((*pEPinfo->copy_data)(Length), GetEPTxAddr(ENDP0), Length);
	SetEPTxCount(ENDP0, Length);

	pEPinfo->wLength -= Length;
	pEPinfo->wOffset += Length;
	SaveTState = EP_TX_VALID;
	SaveRState = EP_RX_VALID; /* Expect the host to abort the data IN stage */

Expect_Status_Out:
	pInformation->state = state;
}

static void NoData_Setup0(void)
{
	RESULT Result = USB_UNSUPPORT;
	uint32_t state;

	if(Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT))
	{
		switch(pInformation->bRequest)
		{
		case SET_CONFIGURATION:
			if((pInformation->USBwValue0 <= usb_prop.cfg_count) &&
			   (pInformation->USBwValue1 == 0) &&
			   (pInformation->USBwIndex == 0)) /*call Back usb spec 2.0*/
			{
				pInformation->curr_cfg = pInformation->USBwValue0;
				if(pInformation->curr_cfg != 0) usb_prop.bDeviceState = CONFIGURED;
				Result = USB_SUCCESS;
			}
			else
			{
				Result = USB_UNSUPPORT;
			}
			break;
		case SET_ADDRESS:
			if((pInformation->USBwValue0 > 127) || (pInformation->USBwValue1 != 0) || (pInformation->USBwIndex != 0) || (pInformation->curr_cfg != 0))
			/* Device Address should be 127 or less*/
			{
				state = STALLED;
				goto exit_NoData_Setup0;
			}
			Result = USB_SUCCESS;
			break;
		case SET_FEATURE:
			if((pInformation->USBwValue0 == DEVICE_REMOTE_WAKEUP) && (pInformation->USBwIndex == 0))
			{
				SetBit(pInformation->curr_feat, 5);
				Result = USB_SUCCESS;
			}
			else
				Result = USB_UNSUPPORT;
			break;
		case CLEAR_FEATURE:
			if(pInformation->USBwValue0 == DEVICE_REMOTE_WAKEUP && pInformation->USBwIndex == 0 && ValBit(pInformation->curr_feat, 5))
			{
				ClrBit(pInformation->curr_feat, 5);
				Result = USB_SUCCESS;
			}
			else
				Result = USB_UNSUPPORT;
			break;
		default: break;
		}
	}
	else if(Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))
	{
		if(pInformation->bRequest == SET_INTERFACE)
		{
			/*Test if the specified Interface and Alternate Setting are supported by the application Firmware*/
			RESULT Re = (*pProperty->get_iface_sett)(pInformation->USBwIndex0, pInformation->USBwValue0);
			if(pInformation->curr_cfg != 0)
			{
				if((Re != USB_SUCCESS) || (pInformation->USBwIndex1 != 0) || (pInformation->USBwValue1 != 0))
				{
					Result = USB_UNSUPPORT;
				}
				else if(Re == USB_SUCCESS)
				{
					pInformation->curr_iface = pInformation->USBwIndex0;
					pInformation->curr_alt_sett = pInformation->USBwValue0;
					Result = USB_SUCCESS;
				}
			}
			else
				Result = USB_UNSUPPORT;
		}
	}
	else if(Type_Recipient == (STANDARD_REQUEST | ENDPOINT_RECIPIENT))
	{
		if(pInformation->bRequest == CLEAR_FEATURE)
		{
			if((pInformation->USBwValue != ENDPOINT_STALL) ||
			   (pInformation->USBwIndex1 != 0))
				Result = USB_UNSUPPORT;
			else
			{
				uint32_t rEP = pInformation->USBwIndex0 & ~0x80;
				uint32_t Related_Endpoint = ENDP0 + rEP;
				uint32_t Status = ValBit(pInformation->USBwIndex0, 7) ? _GetEPTxStatus(Related_Endpoint) : _GetEPRxStatus(Related_Endpoint);
				if((rEP >= usb_prop.ep_count) || (Status == 0) || (pInformation->curr_cfg == 0))
					Result = USB_UNSUPPORT;
				else
				{
					if(pInformation->USBwIndex0 & 0x80)
					{
						if(_GetTxStallStatus(Related_Endpoint)) // IN endpoint
						{
							ClearDTOG_TX(Related_Endpoint);
							SetEPTxStatus(Related_Endpoint, EP_TX_VALID);
						}
					}
					else
					{
						if(_GetRxStallStatus(Related_Endpoint)) // OUT endpoint
						{
							if(Related_Endpoint == ENDP0)
							{
								/* After clear the STALL, enable the default endpoint receiver */
								SetEPRxCount(Related_Endpoint, usb_prop.max_pkt_size);
								_SetEPRxStatus(Related_Endpoint, EP_RX_VALID);
							}
							else
							{
								ClearDTOG_RX(Related_Endpoint);
								_SetEPRxStatus(Related_Endpoint, EP_RX_VALID);
							}
						}
					}
					Result = USB_SUCCESS;
				}
			}
		}
		else if(pInformation->bRequest == SET_FEATURE)
		{
			uint32_t wIndex0 = pInformation->USBwIndex0;
			uint32_t rEP = wIndex0 & ~0x80;
			uint32_t Related_Endpoint = ENDP0 + rEP;
			uint32_t Status = ValBit(pInformation->USBwIndex0, 7) ? _GetEPTxStatus(Related_Endpoint) : _GetEPRxStatus(Related_Endpoint);
			if(Related_Endpoint >= usb_prop.ep_count || pInformation->USBwValue != 0 || Status == 0 || pInformation->curr_cfg == 0)
				Result = USB_UNSUPPORT;
			else
			{
				_SetEPTxStatus(Related_Endpoint, wIndex0 & 0x80 ? EP_TX_STALL : EP_RX_STALL); /* IN/OUT endpoint */
				Result = USB_SUCCESS;
			}
		}
	}
	else
	{
		Result = USB_UNSUPPORT;
	}

	if(Result != USB_SUCCESS)
	{
		Result = (*pProperty->setup)();
		if(Result == USB_NOT_READY)
		{
			state = PAUSE;
			goto exit_NoData_Setup0;
		}
	}

	if(Result != USB_SUCCESS)
	{
		state = STALLED;
		goto exit_NoData_Setup0;
	}

	state = WAIT_STATUS_IN; /* After no data stage SETUP */

	_SetEPTxCount(ENDP0, 0);
	SaveTState = EP_TX_VALID;

exit_NoData_Setup0:
	pInformation->state = state;
	return;
}

static void Data_Setup0(void)
{
	RESULT Result;
	uint8_t *(*copy_data)(uint16_t) = NULL;

	if((pInformation->bmRequestType & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_VENDOR)
	{
		switch(pInformation->USBwIndex0)
		{
		case USB_DESC_TYPE_OS_FEATURE_EXT_PROPERTIES: copy_data = pProperty->get_ext_prop_feat_desc; break;
		case USB_DESC_TYPE_OS_FEATURE_EXT_COMPAT_ID: copy_data = pProperty->get_ext_compat_id_feat_desc; break;
		default: break;
		}
	}
	else if(pInformation->bRequest == GET_DESCRIPTOR)
	{
		if(Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT))
		{
			switch(pInformation->USBwValue1)
			{
			case DEVICE_DESCRIPTOR: copy_data = pProperty->get_dev_desc; break;
			case CONFIG_DESCRIPTOR: copy_data = pProperty->get_cfg_desc; break;
			case STRING_DESCRIPTOR: copy_data = pProperty->get_str_desc; break;
#ifdef LPM_ENABLED
			case DEVICE_BOS_DESCRIPTOR: copy_data = pProperty->get_bos_desc; break;
#endif
			default: break;
			}
		}
	}
	else if((pInformation->bRequest == GET_STATUS) &&
			(pInformation->USBwValue == 0) &&
			(pInformation->USBwLength == 0x0002) &&
			(pInformation->USBwIndex1 == 0))
	{
		if((Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT)) && (pInformation->USBwIndex == 0)) // GET STATUS for Device
		{
			copy_data = Standard_GetStatus;
		}
		else if(Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT)) // GET STATUS for Interface
		{
			if(((*pProperty->get_iface_sett)(pInformation->USBwIndex0, 0) == USB_SUCCESS) && (pInformation->curr_cfg != 0))
			{
				copy_data = Standard_GetStatus;
			}
		}
		else if(Type_Recipient == (STANDARD_REQUEST | ENDPOINT_RECIPIENT)) // GET STATUS for EP
		{
			uint32_t Related_Endpoint = (pInformation->USBwIndex0 & 0x0f);
			uint32_t Reserved = pInformation->USBwIndex0 & 0x70;
			uint32_t Status = ValBit(pInformation->USBwIndex0, 7) ? _GetEPTxStatus(Related_Endpoint) : _GetEPRxStatus(Related_Endpoint);
			if((Related_Endpoint < usb_prop.ep_count) && (Reserved == 0) && (Status != 0)) copy_data = Standard_GetStatus;
		}
	}
	else if(pInformation->bRequest == GET_CONFIGURATION)
	{
		if(Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT)) copy_data = Standard_GetConfiguration;
	}
	else if(pInformation->bRequest == GET_INTERFACE)
	{
		if((Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT)) &&
		   (pInformation->curr_cfg != 0) &&
		   (pInformation->USBwValue == 0) &&
		   (pInformation->USBwIndex1 == 0) &&
		   (pInformation->USBwLength == 0x0001) &&
		   ((*pProperty->get_iface_sett)(pInformation->USBwIndex0, 0) == USB_SUCCESS)) copy_data = Standard_GetInterface;
	}

	if(copy_data)
	{
		pInformation->ep_info.wOffset = 0;
		pInformation->ep_info.copy_data = copy_data;
		(*copy_data)(0);
		Result = USB_SUCCESS;
	}
	else
	{
		Result = (*pProperty->setup)();
		if(Result == USB_NOT_READY)
		{
			pInformation->state = PAUSE;
			return;
		}
	}

	if(pInformation->ep_info.wLength == 0xFFFF)
	{
		pInformation->state = PAUSE; /* Data is not ready, wait it */
		return;
	}
	if(Result == USB_UNSUPPORT)
	{
		pInformation->state = STALLED; /* Unsupported request */
		return;
	}

	if(ValBit(pInformation->bmRequestType, 7))
	{
		__IO uint32_t wLength = pInformation->USBwLength; /* Device ==> Host */

		if(pInformation->ep_info.wLength > wLength)
		{
			pInformation->ep_info.wLength = wLength; // Restrict the data length to be the one host asks for
		}
		else if(pInformation->ep_info.wLength < pInformation->USBwLength)
		{
			if(pInformation->ep_info.wLength < pProperty->max_pkt_size)
				Data_Mul_MaxPacketSize = false;
			else if((pInformation->ep_info.wLength % pProperty->max_pkt_size) == 0)
				Data_Mul_MaxPacketSize = true;
		}

		pInformation->ep_info.pkt_size = pProperty->max_pkt_size;
		data_stage_in();
	}
	else
	{
		pInformation->state = OUT_DATA;
		SaveRState = EP_RX_VALID; /* enable for next data reception */
	}
}

uint8_t EP0_setup(void)
{
	union
	{
		uint8_t *b;
		uint16_t *w;
	} pBuf;
#if defined STM32F303xE || defined STM32F302x8
	uint16_t offset = 0;
	pBuf.b = (uint8_t *)(PMAAddr + _GetEPRxAddr(ENDP0));
#else
	uint16_t offset = 1;
	pBuf.b = PMAAddr + (uint8_t *)(_GetEPRxAddr(ENDP0) * 2); /* *2 for 32 bits addr */
#endif
	if(pInformation->state != PAUSE)
	{
		pInformation->bmRequestType = *pBuf.b++;	   /* bmRequestType */
		pInformation->bRequest = *pBuf.b++;			   /* bRequest */
		pBuf.w += offset;							   /* word not accessed because of 32 bits addressing */
		pInformation->USBwValue = ByteSwap(*pBuf.w++); /* wValue */
		pBuf.w += offset;							   /* word not accessed because of 32 bits addressing */
		pInformation->USBwIndex = ByteSwap(*pBuf.w++); /* wIndex */
		pBuf.w += offset;							   /* word not accessed because of 32 bits addressing */
		pInformation->USBwLength = *pBuf.w;			   /* wLength */
	}

	pInformation->state = SETTING_UP;
	if(pInformation->USBwLength == 0)
	{
		NoData_Setup0(); /* Setup with no data stage */
	}
	else
	{
		Data_Setup0(); /* Setup with data stage */
	}
	return post0_process();
}

/**
 * Function Name  : EP0_in
 * Description    : Process the IN token on all default endpoint.
 */
uint8_t EP0_in(void)
{
	uint32_t state = pInformation->state;

	if((state == IN_DATA) || (state == LAST_IN_DATA))
	{
		data_stage_in();
		state = pInformation->state; /* state may be changed outside the function */
	}
	else if(state == WAIT_STATUS_IN)
	{
		if((pInformation->bRequest == SET_ADDRESS) &&
		   (Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT)))
		{
			set_dev_addr(pInformation->USBwValue0);
			usb_prop.bDeviceState = ADDRESSED; // instead of pUser_Standard_Requests->User_SetDeviceAddress();
		}
		(*pProperty->status_in)();
		state = STALLED;
	}
	else
	{
		state = STALLED;
	}

	pInformation->state = state;

	return post0_process();
}

/**
 * Function Name  : EP0_out
 * Description    : Process the OUT token on all default endpoint.
 */
uint8_t EP0_out(void)
{
	uint32_t state = pInformation->state;
	if((state == IN_DATA) || (state == LAST_IN_DATA))
	{
		state = STALLED; /* host aborts the transfer before finish */
	}
	else if((state == OUT_DATA) || (state == LAST_OUT_DATA))
	{
		data_stage_out();
		state = pInformation->state; /* may be changed outside the function */
	}
	else if(state == WAIT_STATUS_OUT)
	{
		(*pProperty->status_out)();
		state = STALLED;
	}
	else /* Unexpect state, STALL the endpoint */
	{
		state = STALLED;
	}

	pInformation->state = state;
	return post0_process();
}

void set_dev_addr(uint8_t val) // Set the device and all the used Endpoints addresses
{
	uint32_t nEP = usb_prop.ep_count;
	for(uint32_t i = 0; i < nEP; i++) /* set address in every used endpoint */
	{
		_SetEPAddress((uint8_t)i, (uint8_t)i);
	}
	_SetDADDR(val | DADDR_EF); /* set device address and enable function */
}
