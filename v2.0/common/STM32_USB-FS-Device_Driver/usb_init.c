/* @file    usb_init.c
 * @author  MCD Application Team
 * @version V4.1.0
 * @date    26-May-2017
 * @brief   Initialization routines & global variables
 */
#include "usb_lib.h"

DEVICE_PROP *pProperty;
DEVICE_INFO *pInformation, usb_dev;

void USB_Init(void)
{
	pInformation = &usb_dev;
	pInformation->state = 2;
	pProperty = &usb_prop;
	pProperty->init();
	pProperty->bDeviceState = UNCONNECTED;
}
