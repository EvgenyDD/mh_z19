#ifndef SCREEN_H__
#define SCREEN_H__

#include <stdbool.h>
#include <stdint.h>

enum
{
	SCREEN_CO2,		 // prefix:
	SCREEN_TEMP,	 // prefix: t
	SCREEN_HUMIDITY, // prefix: h
	SCREEN_PRESSURE, // prefix: p

	SCREEN_COUNT,
};

void screen_init(void);

void screen_poll(uint32_t diff_ms);

void screen_reset(uint32_t screen, uint32_t digit);
void screen_set_symbol(uint32_t screen, uint32_t digit, uint32_t symbol);
void screen_set_num(uint32_t screen, uint32_t digit, uint8_t num);
void screen_set_dot(uint32_t screen, uint32_t digit, bool state);

void screen_upd_co2(uint32_t value);
void screen_upd_co2_fail(void);
void screen_upd_temperature(uint32_t value /* 0.1 Celsius quant */);
void screen_upd_humidity(uint8_t value);
void screen_upd_pressure(uint16_t value);

#endif // SCREEN_H__