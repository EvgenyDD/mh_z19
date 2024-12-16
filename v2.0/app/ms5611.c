#include "ms5611.h"
#include "console.h"
#include "i2c_common.h"
#include "platform.h"
#include <stddef.h>

#define SAMPLERATE 256

#define WAIT_CONV_MS 10

#define MS5611_ADDR 0xEE

#define MS5611_REG_RESET 0x1E
#define MS5611_REG_D1_256 0x40
#define MS5611_REG_D1_512 0x42
#define MS5611_REG_D1_1024 0x44
#define MS5611_REG_D1_2048 0x46
#define MS5611_REG_D1_4096 0x48
#define MS5611_REG_D2_256 0x50
#define MS5611_REG_D2_512 0x52
#define MS5611_REG_D2_1024 0x54
#define MS5611_REG_D2_2048 0x56
#define MS5611_REG_D2_4096 0x58
#define MS5611_REG_ADC_READ 0x00
#define MS5611_REG_CAL0 0xA0

#define _CONCAT(x, y) x##y
#define CONCAT(x, y) _CONCAT(x, y)

ms5611_meas_t ms5611_meas = {0};

static struct
{
	uint16_t c[7];
} cal_data = {0};

enum
{
	STATE_DFLT = 0,
	STATE_WAIT_D1,
	STATE_WAIT_D2
} st = STATE_DFLT;

static uint32_t wait_cnt = 0;

static uint16_t swap_u16(uint16_t v) { return ((v & 0xFF) << 8) | (v >> 8); }

void ms5611_init(void)
{
	i2c_write(MS5611_ADDR, MS5611_REG_RESET, NULL, 0);
	delay_ms(4);

	for(uint32_t i = 0; i < 7; i++)
	{
		i2c_read(MS5611_ADDR, MS5611_REG_CAL0 + i * 2, (uint8_t *)&cal_data.c[i], 2);
		cal_data.c[i] = swap_u16(cal_data.c[i]);
	}
}

void ms5611_poll(uint32_t diff_ms)
{
	uint8_t buf[3];
	switch(st)
	{
	case STATE_WAIT_D1:
		wait_cnt += diff_ms;
		if(wait_cnt < WAIT_CONV_MS) break;
		i2c_read(MS5611_ADDR, MS5611_REG_ADC_READ, buf, 3);
		ms5611_meas.d1 = (buf[0] << 16) | (buf[1] << 8) | buf[2];
		i2c_write(MS5611_ADDR, CONCAT(MS5611_REG_D2_, SAMPLERATE), NULL, 0); // now get D2
		wait_cnt = 0;
		st = STATE_WAIT_D2;
		break;

	case STATE_WAIT_D2:
		wait_cnt += diff_ms;
		if(wait_cnt < WAIT_CONV_MS) break;
		wait_cnt = 0;
		i2c_read(MS5611_ADDR, MS5611_REG_ADC_READ, buf, 3);
		ms5611_meas.d2 = (buf[0] << 16) | (buf[1] << 8) | buf[2];

		ms5611_meas.dt = ms5611_meas.d2 - (cal_data.c[5] << 8);
		ms5611_meas.temp_0_01C = 2000 + ((ms5611_meas.dt * cal_data.c[6]) >> 23);
		ms5611_meas.off = ((int64_t)cal_data.c[2] << (int64_t)16) + (((int64_t)cal_data.c[4] * (int64_t)ms5611_meas.dt) >> (int64_t)7);
		ms5611_meas.sens = ((int64_t)cal_data.c[1] << (int64_t)15) + (((int64_t)cal_data.c[3] * (int64_t)ms5611_meas.dt) >> (int64_t)8);

		if(ms5611_meas.temp_0_01C < 2000)
		{
			int64_t t2 = ((int64_t)ms5611_meas.dt * (int64_t)ms5611_meas.dt) >> 31;
			int64_t off2 = (5 * (ms5611_meas.temp_0_01C - 2000) * (ms5611_meas.temp_0_01C - 2000)) >> 1;
			int64_t sens2 = (5 * (ms5611_meas.temp_0_01C - 2000) * (ms5611_meas.temp_0_01C - 2000)) >> 2;
			if(ms5611_meas.temp_0_01C < -1500)
			{
				off2 += 7 * (ms5611_meas.temp_0_01C + 1500) * (ms5611_meas.temp_0_01C + 1500);
				sens2 += (11 * (ms5611_meas.temp_0_01C + 1500) * (ms5611_meas.temp_0_01C + 1500)) >> 1;
			}
			ms5611_meas.temp_0_01C -= t2;
			ms5611_meas.off -= off2;
			ms5611_meas.sens -= sens2;
		}

		ms5611_meas.p_pa = ((((int64_t)ms5611_meas.d1 * ms5611_meas.sens) >> (int64_t)21) - ms5611_meas.off) >> (int64_t)15;
		ms5611_meas.p_0_01mmhg = ms5611_meas.p_pa * 2500 / 3333;

		__attribute__((fallthrough));

	default:
	case STATE_DFLT:
		i2c_write(MS5611_ADDR, CONCAT(MS5611_REG_D1_, SAMPLERATE), NULL, 0); //  get D1
		wait_cnt = 0;
		st = STATE_WAIT_D1;
		break;
	}
}