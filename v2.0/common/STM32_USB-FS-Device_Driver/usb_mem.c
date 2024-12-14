/* @file    usb_mem.c
 * @author  MCD Application Team
 * @version V4.1.0
 * @date    26-May-2017
 * @brief   Utility functions for memory transfers to/from PMA
 */
#include "usb_lib.h"

/**
 * Function Name  : UserToPMABufferCopy
 * Description    : Copy a buffer from user memory area to packet memory area (PMA)
 * Input          : - pbUsrBuf: pointer to user memory area.
 *                  - wPMABufAddr: address into PMA.
 *                  - wNBytes: no. of bytes to be copied.
 **/
void UserToPMABufferCopy(uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes)
{
#if defined STM32F303xE || defined STM32F302x8
	uint32_t n = (wNBytes + 1) >> 1; /* n = (wNBytes + 1) / 2 */
	uint32_t i;
	uint16_t *pdwVal;
	pdwVal = (uint16_t *)(wPMABufAddr + PMAAddr);

	for(i = n; i != 0; i--)
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
		*pdwVal++ = *(uint16_t *)pbUsrBuf++;
#pragma GCC diagnostic pop
		pbUsrBuf++;
	}
#else
	uint32_t n = (wNBytes + 1) >> 1; /* n = (wNBytes + 1) / 2 */
	uint32_t i, temp1, temp2;
	uint16_t *pdwVal;
	pdwVal = (uint16_t *)(wPMABufAddr * 2 + PMAAddr);
	for(i = n; i != 0; i--)
	{
		temp1 = (uint16_t)*pbUsrBuf;
		pbUsrBuf++;
		temp2 = temp1 | (uint16_t)*pbUsrBuf << 8;
		*pdwVal++ = temp2;
		pdwVal++;
		pbUsrBuf++;
	}
#endif
}

/**
 * Function Name  : PMAToUserBufferCopy
 * Description    : Copy a buffer from user memory area to packet memory area (PMA)
 * Input          : - pbUsrBuf    = pointer to user memory area.
 *                  - wPMABufAddr = address into PMA.
 *                  - wNBytes     = no. of bytes to be copied.
 **/
void PMAToUserBufferCopy(uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes)
{
#if defined STM32F303xE || defined STM32F302x8
	uint32_t n = (wNBytes + 1) >> 1; /* /2*/
	uint32_t i;
	uint16_t *pdwVal;
	pdwVal = (uint16_t *)(wPMABufAddr + PMAAddr);
	for(i = n; i != 0; i--)
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
		*(uint16_t *)pbUsrBuf++ = *pdwVal++;
#pragma GCC diagnostic pop
		pbUsrBuf++;
	}
#else
	uint32_t n = (wNBytes + 1) >> 1; /* /2*/
	uint32_t i;
	uint32_t *pdwVal;
	pdwVal = (uint32_t *)(wPMABufAddr * 2 + PMAAddr);
	for(i = n; i != 0; i--)
	{
		*(uint16_t *)pbUsrBuf++ = *pdwVal++;
		pbUsrBuf++;
	}
#endif
}