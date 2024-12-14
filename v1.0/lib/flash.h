#ifndef FLASH_H
#define FLASH_H

#include "stm32f3xx_hal.h"

void hal_flash_unlock(void);
static inline void hal_flash_lock(void) { FLASH->CR |= FLASH_CR_LOCK; }

void hal_flash_wait_last_op(void);
void hal_flash_page_erase(uint32_t page_addr);
void hal_flash_program_u16(uint32_t addr, uint16_t data);

#endif // FLASH_H