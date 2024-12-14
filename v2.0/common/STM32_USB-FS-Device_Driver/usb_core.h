/* @file    usb_core.h
 * @author  MCD Application Team
 * @version V4.1.0
 * @date    26-May-2017
 * @brief   Standard protocol processing functions prototypes
 */
#ifndef __USB_CORE_H
#define __USB_CORE_H

#include "stm32f30x.h"
#include <stdint.h>

typedef enum
{
	WAIT_SETUP = 0,
	SETTING_UP,
	IN_DATA,
	OUT_DATA,
	LAST_IN_DATA,
	LAST_OUT_DATA,
	WAIT_STATUS_IN,
	WAIT_STATUS_OUT,
	STALLED,
	PAUSE
} CONTROL_STATE; /* The state machine states of a control pipe */

// If the return value is not SUCCESS or NOT_READY, the software will STALL the correspond endpoint
typedef enum
{
	USB_SUCCESS = 0,
	USB_ERROR,
	USB_UNSUPPORT,
	USB_NOT_READY // The process has not been finished, endpoint will be NAK to further request
} RESULT;

typedef struct
{
	uint16_t wLength; // data remain to be sent
	uint16_t wOffset; // offset of original data
	uint16_t pkt_size;
	uint8_t *(*copy_data)(uint16_t);
} ENDPOINT_INFO;

typedef union
{
	uint16_t w;
	struct BW
	{
		uint8_t bb1;
		uint8_t bb0;
	} bw;
} uint16_t_uint8_t;

typedef struct
{
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t_uint8_t wValue;
	uint16_t_uint8_t wIndex;
	uint16_t_uint8_t wLength;

	uint8_t state; // of type CONTROL_STATE

	uint8_t curr_feat;
	uint8_t curr_cfg;	   // Selected configuration
	uint8_t curr_iface;	   // Selected interface of current configuration
	uint8_t curr_alt_sett; // Selected Alternate Setting of current interface

	ENDPOINT_INFO ep_info;
} DEVICE_INFO;

typedef struct
{
	void (*init)(void);
	void (*reset)(void);
	void (*status_in)(void);  // data downloaded host->device
	void (*status_out)(void); // data uploaded device->host
	int (*setup)(void);
	int (*get_iface_sett)(uint8_t iface, uint8_t alt_sett);
	uint8_t *(*get_dev_desc)(uint16_t len);
#ifdef LPM_ENABLED
	uint8_t *(*get_bos_desc)(uint16_t len);
#endif
	uint8_t *(*get_cfg_desc)(uint16_t len);
	uint8_t *(*get_str_desc)(uint16_t len);
	uint8_t *(*get_ext_prop_feat_desc)(uint16_t len);
	uint8_t *(*get_ext_compat_id_feat_desc)(uint16_t len);

	uint8_t max_pkt_size;

	__IO uint32_t bDeviceState;

	uint8_t ep_count;  /* Number of endpoints that are used */
	uint8_t cfg_count; /* Number of configuration available */
} DEVICE_PROP;

#define Type_Recipient (pInformation->bmRequestType & (REQUEST_TYPE | RECIPIENT))

#define USBwValue wValue.w
#define USBwValue0 wValue.bw.bb0
#define USBwValue1 wValue.bw.bb1

#define USBwIndex wIndex.w
#define USBwIndex0 wIndex.bw.bb0
#define USBwIndex1 wIndex.bw.bb1

#define USBwLength wLength.w
#define USBwLength0 wLength.bw.bb0
#define USBwLength1 wLength.bw.bb1

uint8_t EP0_setup(void);
uint8_t EP0_out(void);
uint8_t EP0_in(void);

uint8_t *Standard_GetConfiguration(uint16_t Length);
uint8_t *Standard_GetInterface(uint16_t Length);
uint8_t *Standard_GetDescriptorData(uint16_t Length, uint8_t *data, uint16_t len);
uint8_t *Standard_GetStatus(uint16_t Length);

void set_dev_addr(uint8_t);

extern DEVICE_PROP usb_prop;
extern DEVICE_INFO usb_dev;

extern __IO uint16_t SaveRState;
extern __IO uint16_t SaveTState;

#endif // __USB_CORE_H
