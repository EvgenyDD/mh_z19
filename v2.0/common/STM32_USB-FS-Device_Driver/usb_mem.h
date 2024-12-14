/* @file    usb_mem.h
 * @author  MCD Application Team
 * @version V4.1.0
 * @date    26-May-2017
 * @brief   Utility prototypes functions for memory/PMA transfers
 */
#ifndef __USB_MEM_H
#define __USB_MEM_H

void UserToPMABufferCopy(uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes);
void PMAToUserBufferCopy(uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes);

#endif // __USB_MEM_H
