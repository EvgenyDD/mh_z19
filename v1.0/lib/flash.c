#include "flash.h"
#include "stm32f3xx_hal.h"

void hal_flash_unlock(void)
{
	if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0)
	{
		FLASH->KEYR = FLASH_KEY1;
		FLASH->KEYR = FLASH_KEY2;
	}
}

void hal_flash_wait_last_op(void)
{
	while(__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY))
		asm("nop");

	if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_EOP)) __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP); /* Clear FLASH End of Operation pending bit */

	if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_WRPERR) || __HAL_FLASH_GET_FLAG(FLASH_FLAG_PGERR))
	{
		__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);
		return; // error
	}
	while(__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY))
		asm("nop");

	if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_EOP)) __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP); /* Clear FLASH End of Operation pending bit */

	if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_WRPERR) || __HAL_FLASH_GET_FLAG(FLASH_FLAG_PGERR))
	{
		__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);
		return; // error
	}
}

void hal_flash_page_erase(uint32_t page_addr)
{
	FLASH->CR |= FLASH_CR_PER;
	FLASH->AR = page_addr;
	FLASH->CR |= FLASH_CR_STRT;

	hal_flash_wait_last_op();

	CLEAR_BIT(FLASH->CR, FLASH_CR_PER);
}

void hal_flash_program_u16(uint32_t addr, uint16_t data)
{
	FLASH->CR |= FLASH_CR_PG;
	*(__IO uint16_t *)addr = data;

	hal_flash_wait_last_op();

	FLASH->CR &= (~FLASH_CR_PG);
}
