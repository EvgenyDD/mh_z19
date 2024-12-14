/* @file    usb_sil.c
 * @author  MCD Application Team
 * @version V4.1.0
 * @date    26-May-2017
 * @brief   Simplified Interface Layer for Global Initialization and Endpoint
 *          Rea/Write operations.
 */
#include "usb_lib.h"

/**
 * Function Name  : USB_SIL_Init
 * Description    : Initialize the USB Device IP and the Endpoint 0.
 **/
uint32_t USB_SIL_Init(void)
{
	_SetISTR(0); // clear pending interrupts
	wInterrupt_Mask = IMR_MSK;
	_SetCNTR(wInterrupt_Mask);
	return 0;
}

/**
 * Function Name  : USB_SIL_Write
 * Description    : Write a buffer of data to a selected endpoint.
 * Input          : - bEpAddr: The address of the non control endpoint.
 *                  - pBufferPointer: The pointer to the buffer of data to be written
 *                    to the endpoint.
 *                  - wBufferSize: Number of data to be written (in bytes).
 * Return         : Status.
 **/
uint32_t USB_SIL_Write(uint8_t bEpAddr, uint8_t *pBufferPointer, uint32_t wBufferSize)
{
	UserToPMABufferCopy(pBufferPointer, GetEPTxAddr(bEpAddr & 0x7F), wBufferSize); // Use the memory interface function to write to the selected endpoint
	SetEPTxCount((bEpAddr & 0x7F), wBufferSize);								   // Update the data length in the control register
	return 0;
}

/**
 * Function Name  : USB_SIL_Read
 * Description    : Write a buffer of data to a selected endpoint.
 * Input          : - bEpAddr: The address of the non control endpoint.
 *                  - pBufferPointer: The pointer to which will be saved the
 *                     received data buffer.
 * Return         : Number of received data (in Bytes).
 **/
uint32_t USB_SIL_Read(uint8_t bEpAddr, uint8_t *pBufferPointer)
{
	uint32_t DataLength = GetEPRxCount(bEpAddr & 0x7F);							  // Get the number of received data on the selected Endpoint
	PMAToUserBufferCopy(pBufferPointer, GetEPRxAddr(bEpAddr & 0x7F), DataLength); // Use the memory interface function to write to the selected endpoint
	return DataLength;
}
