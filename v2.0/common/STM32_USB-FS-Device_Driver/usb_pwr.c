#include "usb_pwr.h"
#include "usb_core.h"
#include "usb_hw.h"
#include "usb_lib.h"

__IO bool fSuspendEnabled = true; /* true when suspend is possible */
static __IO uint32_t EP[8];

static struct
{
	__IO RESUME_STATE eState;
	__IO uint8_t bESOFcnt;
} ResumeS;

static __IO uint32_t remotewakeupon = 0;

int usb_power_on(void)
{
	usb_pullup(ENABLE);

	/*** CNTR_PWDN = 0 ***/
	uint16_t wRegVal = CNTR_FRES;
	_SetCNTR(wRegVal);

	/* The following sequence is recommended:
	  1- FRES = 0
	  2- Wait until RESET flag = 1 (polling)
	  3- clear ISTR register */

	/*** CNTR_FRES = 0 ***/
	wInterrupt_Mask = 0;

	_SetCNTR(wInterrupt_Mask);

	/* Wait until RESET flag = 1 (polling) */
	while((_GetISTR() & ISTR_RESET) == 0)
		;

	/*** Clear pending interrupts ***/
	SetISTR(0);

	/*** Set interrupt mask ***/
	wInterrupt_Mask = CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM;
	_SetCNTR(wInterrupt_Mask);

	return USB_SUCCESS;
}

int usb_power_off(void)
{
	_SetCNTR(CNTR_FRES);			 // disable all interrupts and force USB reset
	_SetISTR(0);					 // clear interrupt status register
	usb_pullup(DISABLE);			 // Disable the Pull-Up
	_SetCNTR(CNTR_FRES + CNTR_PDWN); // switch-off device
	/* sw variables reset */
	/* ... */
	return USB_SUCCESS;
}

void Suspend(void)
{
#ifdef USB_LOW_PWR_MGMT_SUPPORT
	uint32_t tmpreg = 0;
	__IO uint32_t savePWR_CR = 0;
#endif

	/* suspend preparation */
	/* ... */
	uint16_t wCNTR = _GetCNTR();

	/* This a sequence to apply a force RESET to handle a robustness case */
	/*Store endpoints registers status */
	for(uint32_t i = 0; i < 8; i++)
		EP[i] = _GetENDPOINT(i);

	/* unmask RESET flag */
	wCNTR |= CNTR_RESETM;
	_SetCNTR(wCNTR);

	/*apply FRES */
	wCNTR |= CNTR_FRES;
	_SetCNTR(wCNTR);

	/*clear FRES*/
	wCNTR &= ~CNTR_FRES;
	_SetCNTR(wCNTR);

	/*poll for RESET flag in ISTR*/
	while((_GetISTR() & ISTR_RESET) == 0)
		;

	/* clear RESET flag in ISTR */
	_SetISTR((uint16_t)CLR_RESET);

	/*restore Enpoints*/
	for(uint32_t i = 0; i < 8; i++)
		_SetENDPOINT(i, EP[i]);

	/* Now it is safe to enter macrocell in suspend mode */
	wCNTR |= CNTR_FSUSP;
	_SetCNTR(wCNTR);

	/* force low-power mode in the macrocell */
	wCNTR = _GetCNTR();
	wCNTR |= CNTR_LPMODE;
	_SetCNTR(wCNTR);

#ifdef USB_LOW_PWR_MGMT_SUPPORT
	/*prepare entry in low power mode (STOP mode)*/
	/* Select the regulator state in STOP mode*/
	savePWR_CR = PWR->CR;
	tmpreg = PWR->CR;
	/* Clear PDDS and LPDS bits */
	tmpreg &= ((uint32_t)0xFFFFFFFC);
	/* Set LPDS bit according to PWR_Regulator value */
	tmpreg |= PWR_Regulator_LowPower;
	/* Store the new value */
	PWR->CR = tmpreg;
	/* Set SLEEPDEEP bit of Cortex System Control Register */
#if defined(STM32F303xE) || defined(STM32F303xC) || defined(STM32F37X)
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
#else
	SCB->SCR |= SCB_SCR_SLEEPDEEP;
#endif

	/* enter system in STOP mode, only when wakeup flag in not set */
	if((_GetISTR() & ISTR_WKUP) == 0)
	{
		__WFI();
		/* Reset SLEEPDEEP bit of Cortex System Control Register */
#if defined(STM32F303xE) || defined(STM32F303xC) || defined(STM32F37X)
		SCB->SCR &= (uint32_t) ~((uint32_t)SCB_SCR_SLEEPDEEP_Msk);
#else
		SCB->SCR &= (uint32_t) ~((uint32_t)SCB_SCR_SLEEPDEEP);
#endif
	}
	else
	{
		/* Clear Wakeup flag */
		_SetISTR(CLR_WKUP);
		/* clear FSUSP to abort entry in suspend mode  */
		wCNTR = _GetCNTR();
		wCNTR &= ~CNTR_FSUSP;
		_SetCNTR(wCNTR);

		/*restore sleep mode configuration */
		/* restore Power regulator config in sleep mode*/
		PWR->CR = savePWR_CR;

		/* Reset SLEEPDEEP bit of Cortex System Control Register */
#if defined(STM32F303xE) || defined(STM32F303xC) || defined(STM32F37X)
		SCB->SCR &= (uint32_t) ~((uint32_t)SCB_SCR_SLEEPDEEP_Msk);
#else
		SCB->SCR &= (uint32_t) ~((uint32_t)SCB_SCR_SLEEPDEEP);
#endif
	}
#endif /* USB_LOW_PWR_MGMT_SUPPORT */
}

void Resume_Init(void)
{
	/* ------------------ ONLY WITH BUS-POWERED DEVICES ---------------------- */
	/* restart the clocks */
	/* ...  */

	/* CNTR_LPMODE = 0 */
	uint16_t wCNTR = _GetCNTR();
	wCNTR &= (~CNTR_LPMODE);
	_SetCNTR(wCNTR);

#ifdef USB_LOW_PWR_MGMT_SUPPORT
	/* restore full power */
	/* ... on connected devices */
	Leave_LowPowerMode();
#endif /* USB_LOW_PWR_MGMT_SUPPORT */

	/* reset FSUSP bit */
	_SetCNTR(IMR_MSK);

	/* reverse suspend preparation */
	/* ... */
}

/*******************************************************************************
 * Description    : This is the state machine handling resume operations and
 *                 timing sequence. The control is based on the Resume structure
 *                 variables and on the ESOF interrupt calling this subroutine
 *                 without changing machine state.
 * Input          : a state machine value (RESUME_STATE)
 *                  RESUME_ESOF doesn't change ResumeS.eState allowing
 *                  decrementing of the ESOF counter in different states.
 *******************************************************************************/
void Resume(RESUME_STATE eResumeSetVal)
{
	uint16_t wCNTR;

	if(eResumeSetVal != RESUME_ESOF)
		ResumeS.eState = eResumeSetVal;
	switch(ResumeS.eState)
	{
	case RESUME_EXTERNAL:
		if(remotewakeupon == 0)
		{
			Resume_Init();
			ResumeS.eState = RESUME_OFF;
		}
		else /* RESUME detected during the RemoteWAkeup signalling => keep RemoteWakeup handling*/
		{
			ResumeS.eState = RESUME_ON;
		}
		break;
	case RESUME_INTERNAL:
		Resume_Init();
		ResumeS.eState = RESUME_START;
		remotewakeupon = 1;
		break;
	case RESUME_LATER:
		ResumeS.bESOFcnt = 2;
		ResumeS.eState = RESUME_WAIT;
		break;
	case RESUME_WAIT:
		ResumeS.bESOFcnt--;
		if(ResumeS.bESOFcnt == 0)
			ResumeS.eState = RESUME_START;
		break;
	case RESUME_START:
		wCNTR = _GetCNTR();
		wCNTR |= CNTR_RESUME;
		_SetCNTR(wCNTR);
		ResumeS.eState = RESUME_ON;
		ResumeS.bESOFcnt = 10;
		break;
	case RESUME_ON:
		ResumeS.bESOFcnt--;
		if(ResumeS.bESOFcnt == 0)
		{
			wCNTR = _GetCNTR();
			wCNTR &= (~CNTR_RESUME);
			_SetCNTR(wCNTR);
			ResumeS.eState = RESUME_OFF;
			remotewakeupon = 0;
		}
		break;
	case RESUME_OFF:
	case RESUME_ESOF:
	default:
		ResumeS.eState = RESUME_OFF;
		break;
	}
}