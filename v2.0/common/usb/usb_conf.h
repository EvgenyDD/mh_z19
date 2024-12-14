#ifndef USB_CONF_H__
#define USB_CONF_H__

#include "stm32f30x.h"
#include <stddef.h>

#define BTABLE_ADDRESS (0x00)

#define USB_MAX_EP0_SIZE 64

#ifdef USBD_CLASS_COMPOSITE_DFU_CDC

#define EP_NUM (4)

#define ENDP0_RXADDR (0x40)
#define ENDP0_TXADDR (0x80)

#define ENDP1_TXADDR (0xC0)
#define ENDP2_TXADDR (0x100)

#define ENDP3_RXADDR (0x110)

#define SOF_CALLBACK
#endif

#ifdef USBD_CLASS_DFU
#define EP_NUM (1)

#define ENDP0_RXADDR (0x10)
#define ENDP0_TXADDR (0x50)

#endif // USBD_CLASS_DFU

#ifdef USBD_CLASS_CDC
#define EP_NUM (4)

#define ENDP0_RXADDR (0x40)
#define ENDP0_TXADDR (0x80)

#define ENDP1_TXADDR (0xC0)
#define ENDP2_TXADDR (0x100)

#define ENDP3_RXADDR (0x110)

#define SOF_CALLBACK

#endif // USBD_CLASS_CDC

#define IMR_MSK (CNTR_CTRM | CNTR_WKUPM | CNTR_SUSPM | CNTR_ERRM | CNTR_SOFM | CNTR_ESOFM | CNTR_RESETM)

#define USBD_CFG_MAX_NUM (1)
#define USBD_ITF_MAX_NUM (1)
#define USB_MAX_STR_DESC_SIZ (200)

#define USBD_SELF_POWERED
#define XFERSIZE (64)

#define TRANSFER_SIZE_BYTES(sze) ((uint8_t)(sze)),	   /* XFERSIZEB0 */ \
								 ((uint8_t)(sze >> 8)) /* XFERSIZEB1 */

#endif // USB_CONF_H__
