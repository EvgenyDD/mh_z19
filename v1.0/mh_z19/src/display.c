#include "display.h"
#include "main.h"

const uint32_t pattrn_dig[10] = {
    PATTRN_CHR_0, PATTRN_CHR_1, PATTRN_CHR_2, PATTRN_CHR_3, PATTRN_CHR_4,
    PATTRN_CHR_5, PATTRN_CHR_6, PATTRN_CHR_7, PATTRN_CHR_8, PATTRN_CHR_9};

// note: DIG_PWM_QUANT=50 ang gamma=2.8 have led_pwm_lvl[led] = 1 to brightness from 0.001 to 0.28 !
#define DIG_PWM_QUANT 50

// Note: pwm is done by switching anodes (common anode display)
// Note: Segments are catodes

static void _reset_dig_0(void) { PIN_SET(A1); }
static void _reset_dig_1(void) { PIN_SET(A2); }
static void _reset_dig_2(void) { PIN_SET(A3); }
static void _reset_dig_3(void) { PIN_RESET(A4); }
static void _set_dig_0(void) { PIN_RESET(A1); }
static void _set_dig_1(void) { PIN_RESET(A2); }
static void _set_dig_2(void) { PIN_RESET(A3); }
static void _set_dig_3(void) { PIN_SET(A4); }

static const uint8_t led_gamma[DIG_PWM_QUANT + 1] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 2, 2, 3, 3, 4,
    4, 5, 5, 6, 7, 8, 8, 9, 10, 12,
    13, 14, 15, 17, 18, 19, 21, 23, 25, 26,
    28, 30, 32, 35, 37, 39, 42, 44, 47, 50, 50};

static void (*const p_dig_set[])(void) = {_set_dig_0, _set_dig_1, _set_dig_2, _set_dig_3};
static void (*const p_dig_reset[])(void) = {_reset_dig_0, _reset_dig_1, _reset_dig_2, _reset_dig_3};

static volatile uint32_t seg_patt[DIG_COUNT] = {0}; // 16 bit HI - PORTB, 16 bit LO - PORTA
static volatile uint32_t dig_pwm_cnt[DIG_COUNT] = {DIG_PWM_QUANT, DIG_PWM_QUANT, DIG_PWM_QUANT, DIG_PWM_QUANT};

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
    static volatile uint32_t pwm_cnt = 0, dig_iter = 0, cur_brght = 0, cur_patt = 0;

    if(++pwm_cnt >= DIG_PWM_QUANT)
    {
        (*p_dig_reset[dig_iter])();               // deactivate current anode
        if(++dig_iter >= DIG_COUNT) dig_iter = 0; // switch to next digit

        // capture setpoints
        cur_brght = led_gamma[dig_pwm_cnt[dig_iter]];
        cur_patt = seg_patt[dig_iter];

        seg_set(cur_patt); // set segment array

        if(dig_pwm_cnt[dig_iter] != 0) (*p_dig_set[dig_iter])(); // activate anode if PWM is not 0
        pwm_cnt = 0;
    }
    if(pwm_cnt == cur_brght) (*p_dig_reset[dig_iter])(); // deactivate anode (for PWM)

    if(TIM17->SR & TIM_FLAG_UPDATE) TIM17->SR = ~(TIM_IT_UPDATE);
}

void display_init(void)
{
    __HAL_RCC_TIM17_CLK_ENABLE();

    NVIC_SetPriority(TIM1_TRG_COM_TIM17_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(TIM1_TRG_COM_TIM17_IRQn);

    uint32_t tmpcr1 = TIM17->CR1;
    tmpcr1 &= ~TIM_CR1_CKD;
    tmpcr1 |= TIM_CLOCKDIVISION_DIV1;
    MODIFY_REG(tmpcr1, TIM_CR1_ARPE, TIM_AUTORELOAD_PRELOAD_ENABLE);
    TIM17->CR1 = tmpcr1;
    TIM17->ARR = 499;
    TIM17->PSC = 5;
    TIM17->RCR = 0;
    TIM17->EGR = TIM_EGR_UG;

    TIM17->DIER |= TIM_IT_UPDATE;
    TIM17->CR1 |= TIM_CR1_CEN;
}

void display_set_pwm(uint32_t digit, float level) { dig_pwm_cnt[digit] = (unsigned int)(level * DIG_PWM_QUANT) % 50; }
void display_reset(uint32_t digit) { seg_patt[digit] = 0; }
void display_set_symbol(uint32_t digit, uint32_t symbol) { seg_patt[digit] = symbol; }

void display_set_num(uint32_t digit, uint8_t num)
{
    if(num < 10)
    {
        seg_patt[digit] = pattrn_dig[num];
    }
}

void display_set_dot(uint32_t digit, bool state)
{
    if(state)
        seg_patt[digit] |= (1UL << 8UL);
    else
        seg_patt[digit] &= ~(1UL << 8UL);
}
