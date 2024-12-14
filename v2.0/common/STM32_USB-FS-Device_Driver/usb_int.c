/* @file    usb_int.c
 * @author  MCD Application Team
 * @version V4.1.0
 * @date    26-May-2017
 * @brief   Endpoint CTR (Low and High) interrupt's service routines
 */
#include "usb_lib.h"

__IO uint16_t SaveRState;
__IO uint16_t SaveTState;

extern void (*pEpInt_IN[7])(void);
extern void (*pEpInt_OUT[7])(void);

static uint8_t EPindex;

void CTR_LP(void) // Low priority Endpoint Correct Transfer interrupt's service routine
{
	__IO uint16_t wEPVal = 0;
	while(((wIstr = _GetISTR()) & ISTR_CTR) != 0) // stay in loop while pending interrupts
	{
		EPindex = (uint8_t)(wIstr & ISTR_EP_ID); // extract highest priority endpoint number
		if(EPindex == 0)
		{
			// save RX & TX status and set both to NAK
			SaveRState = _GetENDPOINT(ENDP0);
			SaveTState = SaveRState & EPTX_STAT;
			SaveRState &= EPRX_STAT;
			_SetEPRxTxStatus(ENDP0, EP_RX_NAK, EP_TX_NAK);

			if((wIstr & ISTR_DIR) == 0) // DIR bit = origin of the interrupt
			{
				// DIR = 0 implies that (EP_CTR_TX = 1) always
				_ClearEP_CTR_TX(ENDP0);
				EP0_in();
				_SetEPRxTxStatus(ENDP0, SaveRState, SaveTState); // before terminate set Tx & Rx status
				return;
			}
			else
			{
				/* DIR = 1 & CTR_RX       => SETUP or OUT int */
				/* DIR = 1 & (CTR_TX | CTR_RX) => 2 int pending */
				wEPVal = _GetENDPOINT(ENDP0);
				if((wEPVal & EP_SETUP) != 0)
				{
					_ClearEP_CTR_RX(ENDP0); // SETUP bit kept frozen while CTR_RX = 1
					EP0_setup();
					_SetEPRxTxStatus(ENDP0, SaveRState, SaveTState); // before terminate set Tx & Rx status
					return;
				}
				else if((wEPVal & EP_CTR_RX) != 0)
				{
					_ClearEP_CTR_RX(ENDP0);
					EP0_out();
					_SetEPRxTxStatus(ENDP0, SaveRState, SaveTState); // before terminate set Tx & Rx status
					return;
				}
			}
		}
		else
		{
			wEPVal = _GetENDPOINT(EPindex);
			if((wEPVal & EP_CTR_RX) != 0)
			{
				_ClearEP_CTR_RX(EPindex); // clear int flag
				(*pEpInt_OUT[EPindex - 1])();
			}
			if((wEPVal & EP_CTR_TX) != 0)
			{
				_ClearEP_CTR_TX(EPindex); // clear int flag
				(*pEpInt_IN[EPindex - 1])();
			}
		}
	}
}

void CTR_HP(void) // High Priority Endpoint Correct Transfer interrupt's service routine
{
	uint32_t wEPVal = 0;
	while(((wIstr = _GetISTR()) & ISTR_CTR) != 0)
	{
		_SetISTR((uint16_t)CLR_CTR);			 // clear CTR flag
		EPindex = (uint8_t)(wIstr & ISTR_EP_ID); // extract highest priority endpoint number
		wEPVal = _GetENDPOINT(EPindex);
		if((wEPVal & EP_CTR_RX) != 0)
		{
			_ClearEP_CTR_RX(EPindex); // clear int flag
			(*pEpInt_OUT[EPindex - 1])();
		}
		else if((wEPVal & EP_CTR_TX) != 0)
		{
			_ClearEP_CTR_TX(EPindex); // clear int flag
			(*pEpInt_IN[EPindex - 1])();
		}
	}
}
