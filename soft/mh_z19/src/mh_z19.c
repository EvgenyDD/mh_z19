#include "mh_z19.h"
#include "main.h"

extern UART_HandleTypeDef huart2;

#define INTERVAL_MS 5000
#define RX_OFFSET_MS 30

static uint32_t tim_upd = INTERVAL_MS * 8 / 10;
static bool rx_flag = false;
static volatile uint8_t rx_cnt = 0;
static volatile uint16_t rx_value = 0;
static volatile uint16_t co2_now = MH_Z19_INVALID_VALUE;
static volatile uint16_t temp = 100;
static volatile uint16_t co2_irq = 0;

static uint8_t request[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
static uint8_t resp[32];

void mh_z19_init(void)
{
    // HAL_UART_IRQHandler(&huart2);
    
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    USART2->CR1 |= 1 << 5;
    // USART2->CR1 &= ~(uint32_t)(1 << 6);
    // USART2->CR1 &= ~(uint32_t)(1 << 8);

    // USART2->CR1 &= ~((uint32_t)((1 << 6) | (1 << 8)));
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}

void mh_z19_poll(uint32_t diff_ms)
{
    tim_upd += diff_ms;
    if(tim_upd >= INTERVAL_MS)
    {
        rx_flag = true;
        tim_upd = 0;
        rx_cnt = 0;
        HAL_UART_Transmit_DMA(&huart2, (int8_t *)request, sizeof(request));
        // HAL_UART_Receive_IT(&huart2, resp, 9);
    }
    if(tim_upd > RX_OFFSET_MS && rx_flag)
    {
        rx_flag = false;
        if(rx_cnt == 9)
        {
            co2_now = co2_irq;
        }
        else
        {
            co2_now = MH_Z19_INVALID_VALUE;
        }
    }
}

void USART2_IRQHandler(void)
{
    if(USART2->ISR & USART_ISR_RXNE)
    {
        uint8_t x = USART2->RDR;

        if(rx_cnt == 2)
        {
            rx_value = x << 8;
        }
        else if(rx_cnt == 3)
        {
            rx_value |= x;
            co2_irq = rx_value;
        }
        else if(rx_cnt == 4)
        {
            temp = x;
        }
        rx_cnt++;
    }
    if(USART2->ISR & USART_ISR_ORE)
    {
        USART2->ICR |= USART_ICR_ORECF;
        rx_cnt = 0;
    }
    if(USART2->ISR & USART_ISR_TC)
    {
        USART2->ICR |= USART_ICR_TCCF;
        huart2.gState = HAL_UART_STATE_READY;
    }
}

uint16_t mx_z19_get_co2(void) { return co2_now; }
uint16_t mx_z19_get_temp(void) { return temp - 40; }
