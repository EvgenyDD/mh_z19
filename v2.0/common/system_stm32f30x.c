#include "stm32f30x.h"

uint32_t SystemCoreClock = 72000000;
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};

void SystemInit(void);
void NMI_Handler(void);

static void clock_reinit(void)
{
	RCC_DeInit();
	RCC_HSEConfig(RCC_HSE_ON);
	ErrorStatus errorStatus = RCC_WaitForHSEStartUp();

	if(errorStatus == SUCCESS)
	{
		FLASH->ACR = FLASH_ACR_PRFTBE | (uint32_t)FLASH_ACR_LATENCY_1;
		RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;	// HCLK
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1; // PCLK2
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV2; // PCLK1
		RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);

		RCC->CFGR &= (uint32_t)((uint32_t) ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
		RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_PREDIV1 | RCC_CFGR_PLLXTPRE_PREDIV1 | RCC_CFGR_PLLMULL6);

		RCC->CR |= RCC_CR_PLLON;
		while((RCC->CR & RCC_CR_PLLRDY) == 0)
		{
		}

		RCC->CFGR &= (uint32_t)((uint32_t) ~(RCC_CFGR_SW));
		RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;

		while((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_PLL)
		{
		}

		RCC->CR |= RCC_CR_CSSON;
	}
}

void NMI_Handler(void)
{
	while(1)
		;
}

void SystemInit(void)
{
	SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2)); /* set CP10 and CP11 Full Access */
	clock_reinit();
}
