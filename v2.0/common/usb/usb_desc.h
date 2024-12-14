#ifndef __USB_DESC_H
#define __USB_DESC_H

#include <stdint.h>

#define CDC_IN_EP 0x81	/* EP1 for data IN */
#define CDC_OUT_EP 0x03 /* EP3 for data OUT */
#define CDC_CMD_EP 0x82 /* EP2 for CDC commands (IN) */

#define CDC_DATA_MAX_PACKET_SIZE 64 /* Endpoint IN & OUT Packet size */
#define CDC_CMD_PACKET_SIZE 8		/* Control Endpoint Packet size */

void usdb_desc_init(void);

uint8_t *usbd_usr_device_desc(uint16_t Length);
uint8_t *usbd_get_cfg_desc(uint16_t Length);
uint8_t *usbd_get_str_desc(uint16_t Length);
uint8_t *usbd_usr_ext_prop_feat_desc(uint16_t Length);
uint8_t *usbd_usr_ext_compat_id_feat_desc(uint16_t Length);

extern uint8_t usb_cfg_desc[];

#endif // __USB_DESC_H
