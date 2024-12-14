/* @file    usb_lib.h
 * @author  MCD Application Team
 * @version V4.1.0
 * @date    26-May-2017
 * @brief   USB library include files
 */

#ifndef __USB_LIB_H
#define __USB_LIB_H

#include "usb_conf.h"
#include "usb_core.h"
#include "usb_def.h"
#include "usb_init.h"
#include "usb_int.h"
#include "usb_mem.h"
#include "usb_regs.h"
#include "usb_sil.h"

typedef enum _DEVICE_STATE
{
	UNCONNECTED = 0,
	ATTACHED,
	ADDRESSED,
	CONFIGURED,
} DEVICE_STATE;

#endif // __USB_LIB_H
