#ifndef USB_BSP_H__
#define USB_BSP_H__

#include <stdbool.h>
#include <stdint.h>

void usb_init(void);
void usb_disconnect(void);
void usb_poll(uint32_t diff_ms);

void usb_pullup(bool state);

#endif // USB_BSP_H__