#include "display.h"
#include "platform.h"

void TIM1_TRG_COM_TIM17_IRQHandler(void);

// Note: pwm is done by switching anodes (common anode display)
// Note: Segments are catodes
static void _reset_dig_0(void) { PIN_SET(DIG_0); }
static void _reset_dig_1(void) { PIN_SET(DIG_1); }
static void _reset_dig_2(void) { PIN_SET(DIG_2); }
static void _reset_dig_3(void) { PIN_RST(DIG_3); }
static void _set_dig_0(void) { PIN_RST(DIG_0); }
static void _set_dig_1(void) { PIN_RST(DIG_1); }
static void _set_dig_2(void) { PIN_RST(DIG_2); }
static void _set_dig_3(void) { PIN_SET(DIG_3); }

static void (*const p_dig_set[])(void) = {_set_dig_0, _set_dig_1, _set_dig_2, _set_dig_3};
static void (*const p_dig_reset[])(void) = {_reset_dig_0, _reset_dig_1, _reset_dig_2, _reset_dig_3};

static volatile uint32_t seg_patt[DIG_COUNT] = {PATTRN_CHR_C, PATTRN_CHR_UD, PATTRN_CHR_UD, PATTRN_CHR_BRKT_R}; // 16 bit HI - PORTB, 16 bit LO - PORTA

static inline void seg_set(uint32_t pattern)
{
	// Reset segments
	GPIOA->BSRR = ((1 << 8) | (1 << 9) | (1 << 10) | (1 << 13) | (1 << 14));
	GPIOB->BSRR = ((1 << 3) | (1 << 4) | (1 << 7));

	// Set segments
	GPIOA->BSRR = (pattern & 0xFFFF) << 16;
	GPIOB->BSRR = pattern & 0xFFFF0000UL;
}

void TIM1_TRG_COM_TIM17_IRQHandler(void)
{
	static volatile uint32_t pwm_cnt = 0, dig_iter = 0;
	if(++pwm_cnt >= 50)
	{
		(*p_dig_reset[dig_iter])();				  // deactivate current anode
		if(++dig_iter >= DIG_COUNT) dig_iter = 0; // switch to next digit
		seg_set(seg_patt[dig_iter]);			  // set segment array
		(*p_dig_set[dig_iter])();
		pwm_cnt = 0;
	}
	if(TIM17->SR & TIM_FLAG_Update) TIM17->SR = ~(TIM_IT_Update);
}

void display_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM17, ENABLE);

	NVIC_SetPriority(TIM1_TRG_COM_TIM17_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
	NVIC_EnableIRQ(TIM1_TRG_COM_TIM17_IRQn);

	uint32_t tmpcr1 = TIM17->CR1;
	tmpcr1 &= ~TIM_CR1_CKD;
	tmpcr1 |= TIM_CKD_DIV1;
	TIM_ARRPreloadConfig(TIM17, ENABLE);
	TIM17->CR1 = tmpcr1;
	TIM17->ARR = 499;
	TIM17->PSC = 5;
	TIM17->RCR = 0;
	TIM17->EGR = TIM_EGR_UG;

	TIM17->DIER |= TIM_IT_Update;
	TIM17->CR1 |= TIM_CR1_CEN;
}
