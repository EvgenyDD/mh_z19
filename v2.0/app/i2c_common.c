#include "i2c_common.h"
#include "stm32f30x.h"

#define TIMEOUT 0xFFF
#define TIMEOUT_SHORT 100

static uint32_t time = 0;

#define CHK_EVT_RET(x, c)                      \
	for(time = TIMEOUT; time > 0 && x; time--) \
		;                                      \
	if(time == 0) return (c)

#define CHK_EVT_RET_SHORT(x, c)                      \
	for(time = TIMEOUT_SHORT; time > 0 && x; time--) \
		;                                            \
	if(time == 0) return (c)

void i2c_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	I2C_InitTypeDef I2C_InitStruct;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_4);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_4);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	I2C_DeInit(I2C1);

	I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStruct.I2C_AnalogFilter = I2C_AnalogFilter_Enable;
	I2C_InitStruct.I2C_OwnAddress1 = 0xFE;
	I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStruct.I2C_Timing = 0x0010020A; // 400kHz, 100ns rise/fall time
	I2C_Init(I2C1, &I2C_InitStruct);

	I2C_Cmd(I2C1, ENABLE);
}

int i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t size)
{
	CHK_EVT_RET(I2C_GetFlagStatus(I2C1, I2C_ISR_BUSY), -1);

	I2C_TransferHandling(I2C1, addr, 1, I2C_SoftEnd_Mode, I2C_Generate_Start_Write);
	CHK_EVT_RET(!I2C_GetFlagStatus(I2C1, I2C_ISR_TXIS), -2);

	I2C_SendData(I2C1, reg);
	CHK_EVT_RET(!I2C_GetFlagStatus(I2C1, I2C_ISR_TC), -3);

	I2C_TransferHandling(I2C1, addr, size, I2C_AutoEnd_Mode, I2C_Generate_Start_Read);
	while(size)
	{
		CHK_EVT_RET(!I2C_GetFlagStatus(I2C1, I2C_ISR_RXNE), -4);
		*data = I2C_ReceiveData(I2C1);
		data++;
		size--;
	}
	CHK_EVT_RET(!I2C_GetFlagStatus(I2C1, I2C_ISR_STOPF), -5);
	I2C_ClearFlag(I2C1, I2C_ICR_STOPCF);
	return 0;
}

int i2c_write(uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t size)
{
	CHK_EVT_RET(I2C_GetFlagStatus(I2C1, I2C_ISR_BUSY), -1);

	I2C_TransferHandling(I2C1, addr, 1, I2C_Reload_Mode, I2C_Generate_Start_Write);
	CHK_EVT_RET(!I2C_GetFlagStatus(I2C1, I2C_ISR_TXIS), -2);

	I2C_SendData(I2C1, reg);
	CHK_EVT_RET(!I2C_GetFlagStatus(I2C1, I2C_ISR_TCR), -3);

	I2C_TransferHandling(I2C1, addr, size, I2C_AutoEnd_Mode, I2C_No_StartStop);
	while(size)
	{
		CHK_EVT_RET(!I2C_GetFlagStatus(I2C1, I2C_ISR_TXIS), -4);
		I2C_SendData(I2C1, *data);
		data++;
		size--;
	}
	CHK_EVT_RET(!I2C_GetFlagStatus(I2C1, I2C_ISR_TXIS), -5);

	CHK_EVT_RET(!I2C_GetFlagStatus(I2C1, I2C_ISR_STOPF), -6);
	I2C_ClearFlag(I2C1, I2C_ICR_STOPCF);
	return 0;
}
