#include "dht22.h"

#include "main.h"

#define WAIT_THRS 150

#define PIN_RESET(x) x##_GPIO_Port->BSRR = (uint32_t)x##_Pin << 16UL
#define PIN_SET(x) x##_GPIO_Port->BSRR = x##_Pin
#define PIN_READ(x) (x##_GPIO_Port->IDR & x##_Pin)

#define GPIO_TO_INT(GPIO) (((uint32_t)(&(GPIO->CRL)) - GPIOA_BASE) >> 10)

#define __PIN_TO_EXTI_IRQn(PIN) (PIN == 0) ? EXTI0_IRQn : (PIN == 1) ? EXTI1_IRQn   \
													  : (PIN == 2)	 ? EXTI2_IRQn   \
													  : (PIN == 3)	 ? EXTI3_IRQn   \
													  : (PIN == 4)	 ? EXTI4_IRQn   \
													  : (PIN <= 9)	 ? EXTI9_5_IRQn \
																	 : EXTI15_10_IRQn
#define _PIN_TO_EXTI_IRQn(PIN) __PIN_TO_EXTI_IRQn(PIN)
#define PIN_TO_EXTI_IRQn(PIN) _PIN_TO_EXTI_IRQn(PIN)

#define __PIN_TO_EXTI_HANDLER(PIN) EXTI##PIN##_IRQHandler
#define _PIN_TO_EXTI_HANDLER(PIN) __PIN_TO_EXTI_HANDLER(PIN)
#define PIN_TO_EXTI_HANDLER(PIN) _PIN_TO_EXTI_HANDLER(PIN)

#define EXTI_MODE_DISABLE 0x00
#define EXTI_MODE_RISE 0x01
#define EXTI_MODE_FALL 0x02
#define EXTI_MODE_BOTH 0x03

#define _EXTI_INIT(GPIO, PIN, EXTI_MODE, NVIC_PRIORITY)                                                                                            \
	do                                                                                                                                             \
	{                                                                                                                                              \
		/*// AFIO->EXTICR[PIN / 4] = (AFIO->EXTICR[PIN / 4] & ~((uint16_t)0x0F << ((PIN % 4) << 2))) | (GPIO_TO_INT(GPIO) << ((PIN % 4) << 2)); */ \
		BIT_BAND_PER(EXTI->FTSR, 1UL << PIN) = !!(EXTI_MODE & 0x02);                                                                               \
		BIT_BAND_PER(EXTI->RTSR, 1UL << PIN) = !!(EXTI_MODE & 0x01);                                                                               \
                                                                                                                                                   \
		(EXTI_MODE > 0) ? NVIC_EnableIRQ(PIN_TO_EXTI_IRQn(PIN)) : NVIC_DisableIRQ(PIN_TO_EXTI_IRQn(PIN));                                          \
		EXTI->PR = 1UL << PIN;                                                                                                                     \
		BIT_BAND_PER(EXTI->IMR, 1UL << PIN) = !!(EXTI_MODE);                                                                                       \
	} while(0)

#define EXTI_INIT(GPIO, PIN, EXTI_MODE, NVIC_PRIORITY) _EXTI_INIT(GPIO, PIN, EXTI_MODE, NVIC_PRIORITY)

//example: EXTI_INIT(GPIOA, 9, EXTI_MODE_BOTH, 15);

static int32_t ptr = -1;
static uint16_t delta[42 * 2];

void EXTI4_IRQHandler(void)
{
	if(EXTI->PR & 1 << 4)
	{
		EXTI->PR = 1 << 4;
	}
	// if(PIN_READ(DHT))
	if(ptr < (int32_t)sizeof(delta) && ptr >= 0)
	{
		delta[ptr++] = TIM16->CNT;
	}
	else
	{
		ptr = -1;
	}
}

void dht11_init(void)
{
	__HAL_RCC_SYSCFG_CLK_ENABLE();
	// EXTI_INIT(DHT_GPIO_Port, /*DHT_Pin*/ 4, EXTI_MODE_BOTH, 15);
	SYSCFG->EXTICR[1] |= (0 << 0);
	NVIC_SetPriority(EXTI4_IRQn, 15);
	EXTI->EMR |= (1 << 4);
	EXTI->IMR |= (1 << 4);
	EXTI->FTSR |= (1 << 4);
	EXTI->RTSR |= (1 << 4);

	__HAL_RCC_TIM16_CLK_ENABLE();
	TIM16->ARR = 65535;
	TIM16->PSC = 48 - 1;
	TIM16->CR1 |= TIM_CR1_CEN;
}

static uint32_t read_cycle(uint32_t cur_tics, bool neg_tic)
{
	uint32_t cnt_tics;

	if(cur_tics < MAX_TICS) cnt_tics = 0;
	if(neg_tic)
	{
		while(!PIN_READ(DHT) && (cnt_tics < MAX_TICS))
		{
			cnt_tics++;
		}
	}
	else
	{
		while(PIN_READ(DHT) && (cnt_tics < MAX_TICS))
		{
			cnt_tics++;
		}
	}
	return cnt_tics;
}

// 500 ms timeout
int dht11_poll(uint8_t *buf)
{
	ptr = -1;
	NVIC_DisableIRQ(EXTI4_IRQn);
	uint16_t dt[42];

	//reset DHT11
	// HAL_Delay(500);
	PIN_RESET(DHT);
	HAL_Delay(20);
	PIN_SET(DHT);

	EXTI->PR = 1 << 4;
	ptr = 0;
	NVIC_EnableIRQ(EXTI4_IRQn);

	TIM16->CNT = 0;

	// return 0;

	//start reading
	uint32_t cnt = 0;
	for(uint32_t i = 0; i < 83 && cnt < MAX_TICS; i++)
	{
		if(i & 1)
		{
			cnt = read_cycle(cnt, true);
		}
		else
		{
			cnt = read_cycle(cnt, false);
			dt[i / 2] = cnt;
		}
	}

	//release line
	NVIC_DisableIRQ(EXTI4_IRQn);
	PIN_SET(DHT);

	if(cnt >= MAX_TICS) return DHT11_NO_CONN;

	//convert data
	for(uint32_t i = 2; i < 42; i++)
	{
		(*buf) <<= 1;
		if(dt[i] > WAIT_THRS) (*buf)++;
		if(!((i - 1) % 8) && (i > 2)) buf++;
	}

	//calculate checksum
	buf -= 5;
	uint8_t check_sum = 0;
	for(uint32_t i = 0; i < 4; i++)
	{
		check_sum += *buf;
		buf++;
	}

	if(*buf != check_sum) return DHT11_CS_ERROR;

	return DHT11_OK;
}
