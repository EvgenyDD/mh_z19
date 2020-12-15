#include "stm32f3xx.h"

uint32_t SystemCoreClock = 8000000;

const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};

void SystemInit(void) { SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2)); /* set CP10 and CP11 Full Access */ }
