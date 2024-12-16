#include "ws2812.h"
#include "platform.h"

#include <string.h>

#define BITS_PER_SIG 15

#define LO BITS_PER_SIG / 4
#define HI BITS_PER_SIG * 3 / 4

static uint8_t tx_arr[BITS_PER_SIG * 8 * 3 + 5] = {0};

// static void set_pattern(uint32_t sig, bool set)
// {
// 	uint32_t start = sig * BITS_PER_SIG + 5;
// 	// uint32_t end = start + BITS_PER_SIG - 1;
// }

void ws2812_init(void)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_5);

	SPI_InitTypeDef SPI_InitStructure;
	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI2, &SPI_InitStructure);
	SPI_SSOutputCmd(SPI2, DISABLE);
	SPI_Cmd(SPI2, ENABLE);

	DMA_InitTypeDef DMA_InitStructure;
	DMA_StructInit(&DMA_InitStructure);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)tx_arr;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI2->DR));
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = sizeof(tx_arr);
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);

	for(uint32_t i = 0; i < BITS_PER_SIG * 8 * 3; i++)
	{
		uint32_t pos = i % BITS_PER_SIG;
		if(pos < 5)
			tx_arr[i / 8] |= (1 << (i % 8));
		else
			tx_arr[i / 8] &= ~(1 << (i % 8));
	}
	color_t c = {0};
	ws2812_set_color(&c);
}

void ws2812_poll(void)
{
	DMA_Cmd(DMA1_Channel5, DISABLE);
	DMA1_Channel5->CNDTR = sizeof(tx_arr);
	DMA1_Channel5->CMAR = (uint32_t)tx_arr;
	DMA_Cmd(DMA1_Channel5, ENABLE);
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
}

void ws2812_set_color(color_t *color)
{
	for(uint32_t i = BITS_PER_SIG * (8 * 0); i < BITS_PER_SIG * (8 * 1); i++) // g
	{
		uint32_t pos = i % BITS_PER_SIG;
		uint32_t bit_off = (color->g & (1 << (7 - i / BITS_PER_SIG))) ? HI : LO;
		if(pos < bit_off)
			tx_arr[i / 8] |= (1 << (7 - (i % 8)));
		else
			tx_arr[i / 8] &= ~(1 << (7 - (i % 8)));
	}
	for(uint32_t i = BITS_PER_SIG * (8 * 1); i < BITS_PER_SIG * (8 * 2); i++)
	{
		uint32_t pos = i % BITS_PER_SIG;
		uint32_t bit_off = (color->r & (1 << (7 - (i - BITS_PER_SIG * (8 * 1)) / BITS_PER_SIG))) ? HI : LO;
		if(pos < bit_off)
			tx_arr[i / 8] |= (1 << (7 - (i % 8)));
		else
			tx_arr[i / 8] &= ~(1 << (7 - (i % 8)));
	}
	for(uint32_t i = BITS_PER_SIG * (8 * 2); i < BITS_PER_SIG * (8 * 3); i++)
	{
		uint32_t pos = i % BITS_PER_SIG;
		uint32_t bit_off = (color->b & (1 << (7 - (i - BITS_PER_SIG * (8 * 2)) / BITS_PER_SIG))) ? HI : LO;
		if(pos < bit_off)
			tx_arr[i / 8] |= (1 << (7 - (i % 8)));
		else
			tx_arr[i / 8] &= ~(1 << (7 - (i % 8)));
	}

	ws2812_poll();
}

/**
 * h [0;360]
 * s [0;1]
 * v [0;255]
 */
color_t hsv2rgb(float h, float s, float v)
{
	float p, q, t, ff;
	color_t out;

	float hh = h;
	if(hh >= 360.0) hh = 0.0;
	hh /= 60.0;
	int i = (int)hh;
	ff = hh - i;
	p = v * (1.0 - s);
	q = v * (1.0 - (s * ff));
	t = v * (1.0 - (s * (1.0 - ff)));

	switch(i)
	{
	case 0:
		out.r = v;
		out.g = t;
		out.b = p;
		break;
	case 1:
		out.r = q;
		out.g = v;
		out.b = p;
		break;
	case 2:
		out.r = p;
		out.g = v;
		out.b = t;
		break;

	case 3:
		out.r = p;
		out.g = q;
		out.b = v;
		break;
	case 4:
		out.r = t;
		out.g = p;
		out.b = v;
		break;
	case 5:
	default:
		out.r = v;
		out.g = p;
		out.b = q;
		break;
	}
	return out;
}