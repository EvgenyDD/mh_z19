#ifndef USBD_CORE_CDC_H__
#define USBD_CORE_CDC_H__

#include "usb_core.h"
#include <stdint.h>

// CDC Requests
#define SEND_ENCAPSULATED_COMMAND 0x00
#define GET_ENCAPSULATED_RESPONSE 0x01
#define SET_COMM_FEATURE 0x02
#define GET_COMM_FEATURE 0x03
#define CLEAR_COMM_FEATURE 0x04
#define SET_LINE_CODING 0x20
#define GET_LINE_CODING 0x21
#define SET_CONTROL_LINE_STATE 0x22
#define SEND_BREAK 0x23
#define NO_CMD 0xFF

typedef struct
{
	uint32_t bitrate;
	uint8_t format;
	uint8_t paritytype;
	uint8_t datatype;
} LINE_CODING;

void usb_cdc_rst_state(void);
void usbd_cdc_init(void);
void usbd_cdc_reset(void);
int usbd_cdc_setup(void);
void usbd_cdc_status_in(void);

int usbd_cdc_push_data(const uint8_t *data, uint32_t size);
extern void usbd_cdc_rx(const uint8_t *data, uint32_t size);

void usbd_cdc_lock(void);
void usbd_cdc_unlock(void);

#endif // USBD_CORE_CDC_H__
