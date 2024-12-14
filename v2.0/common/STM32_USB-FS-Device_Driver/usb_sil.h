/* @file    usb_sil.h
 * @author  MCD Application Team
 * @version V4.1.0
 * @date    26-May-2017
 * @brief   Simplified Interface Layer function prototypes.
 */
#ifndef __USB_SIL_H
#define __USB_SIL_H

uint32_t USB_SIL_Init(void);
uint32_t USB_SIL_Write(uint8_t bEpAddr, uint8_t *pBufferPointer, uint32_t wBufferSize);
uint32_t USB_SIL_Read(uint8_t bEpAddr, uint8_t *pBufferPointer);

#endif // __USB_SIL_H
