/* @file    usb_init.h
 * @author  MCD Application Team
 * @version V4.1.0
 * @date    26-May-2017
 * @brief   Initialization routines & global variables
 */
#ifndef __USB_INIT_H
#define __USB_INIT_H

#include "usb_core.h"

void USB_Init(void);

extern DEVICE_INFO *pInformation;
extern DEVICE_PROP *pProperty;
extern uint16_t wInterrupt_Mask;

#endif // __USB_INIT_H
