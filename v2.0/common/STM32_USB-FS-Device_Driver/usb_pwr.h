#ifndef __USB_PWR_H
#define __USB_PWR_H

#include "usb_core.h"
#include <stdbool.h>

typedef enum _RESUME_STATE
{
	RESUME_EXTERNAL = 0,
	RESUME_INTERNAL,
	RESUME_LATER,
	RESUME_WAIT,
	RESUME_START,
	RESUME_ON,
	RESUME_OFF,
	RESUME_ESOF
} RESUME_STATE;

void Suspend(void);
void Resume_Init(void);
void Resume(RESUME_STATE eResumeSetVal);
int usb_power_on(void);
int usb_power_off(void);

extern __IO bool fSuspendEnabled; /* true when suspend is possible */

#endif // __USB_PWR_H