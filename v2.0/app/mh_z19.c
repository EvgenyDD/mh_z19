#include "mh_z19.h"
#include "platform.h"

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

void USART2_IRQHandler(void);

void mh_z19_init(void)
{
	// A2 tx A3 rx
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_7);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_7);

	USART_InitTypeDef USART_InitStructure = {0};
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART2, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	NVIC_EnableIRQ(USART2_IRQn);

	DMA_InitTypeDef DMA_InitStructure;
	DMA_StructInit(&DMA_InitStructure);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)request;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART2->TDR;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = sizeof(request);
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_Init(DMA1_Channel7, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel7, ENABLE);
	USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
	USART_Cmd(USART2, ENABLE);
}

// static void uart_tx(const uint8_t *data, uint32_t len)
// {
// 	for(uint32_t i = 0; i < len; i++)
// 	{
// 		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
// 			;
// 		USART_SendData(USART2, data[i]);
// 	}
// }

void mh_z19_poll(uint32_t diff_ms)
{
	tim_upd += diff_ms;
	if(tim_upd >= INTERVAL_MS)
	{
		rx_flag = true;
		tim_upd = 0;
		rx_cnt = 0;

		DMA_Cmd(DMA1_Channel7, DISABLE);
		DMA1_Channel7->CNDTR = sizeof(request);
		DMA1_Channel7->CMAR = (uint32_t)request;
		DMA_Cmd(DMA1_Channel7, ENABLE);
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
}

uint16_t mx_z19_get_co2(void) { return co2_now; }
uint16_t mx_z19_get_temp(void) { return temp - 40; }
