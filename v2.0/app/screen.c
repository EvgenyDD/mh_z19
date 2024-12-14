#include "screen.h"
#include "display.h"

#include <stddef.h>

const uint32_t time_screen[SCREEN_COUNT] = {
	4000, // SCREEN_CO2
	1500, // SCREEN_TEMP
	1500, // SCREEN_HUMIDITY
};

const uint32_t time_half_transition = 220; // ms

static uint32_t scr_timer = 0;
static uint32_t cur_screen = 0;
static uint32_t screens[SCREEN_COUNT][DIG_COUNT] = {0};

void screen_init(void)
{
	screens[SCREEN_TEMP][0] = PATTRN_CHR_t;
	screens[SCREEN_HUMIDITY][0] = PATTRN_CHR_h;
}

void screen_poll(uint32_t diff_ms)
{
	scr_timer += diff_ms;
	if(scr_timer >= time_screen[cur_screen])
	{
		if(++cur_screen >= SCREEN_COUNT) cur_screen = 0;
		scr_timer = 0;
	}

	if(scr_timer < time_half_transition)
	{
		for(uint32_t i = 0; i < DIG_COUNT; i++)
			display_set_pwm(i, (float)scr_timer / (float)time_half_transition);
	}
	else if(time_screen[cur_screen] - scr_timer < time_half_transition)
	{
		for(uint32_t i = 0; i < DIG_COUNT; i++)
			display_set_pwm(i, (float)(time_screen[cur_screen] - scr_timer) / (float)time_half_transition);
	}
	if(diff_ms)
	{
		for(uint32_t i = 0; i < DIG_COUNT; i++)
			display_set_symbol(i, screens[cur_screen][i]);
	}
}

void screen_reset(uint32_t screen, uint32_t digit) { screens[screen][digit] = 0; }
void screen_set_symbol(uint32_t screen, uint32_t digit, uint32_t symbol) { screens[screen][digit] = symbol; }

void screen_set_num(uint32_t screen, uint32_t digit, uint8_t num)
{
	if(num < 10) screens[screen][digit] = pattrn_dig[num];
}

void screen_set_dot(uint32_t screen, uint32_t digit, bool state)
{
	if(state)
		screens[screen][digit] |= (1UL << 8UL);
	else
		screens[screen][digit] &= ~(1UL << 8UL);
}

static void screen_set_num_value(uint32_t screen, uint32_t value, uint32_t start, uint32_t num_dig, uint32_t num_zeros)
{
	for(uint32_t i = 0; i < num_dig; i++)
	{
		if(i >= num_zeros)
		{
			if(value > 0)
				screen_set_num(screen, start + num_dig - 1 - i, value % 10);
			else
				screen_reset(screen, start + num_dig - 1 - i);
		}
		else
			screen_set_num(screen, start + num_dig - 1 - i, value % 10);
		value /= 10;
	}
}

void screen_upd_co2_fail(void)
{
	for(size_t i = 0; i < DIG_COUNT; i++)
		// screen_set_symbol(SCREEN_CO2, i, PATTRN_CHR_MINUS);
		screen_set_symbol(SCREEN_CO2, i, PATTRN_CHR_L);
}

void screen_upd_co2(uint32_t value)
{
	screen_set_num_value(SCREEN_CO2, value, 0, 4, 1);
}

void screen_upd_temperature(uint32_t value /* 0.1 Celsius quant */) // minus???
{
	screen_set_num_value(SCREEN_TEMP, value, 1, 3, 2);
	screen_set_dot(SCREEN_TEMP, 2, true);
}

void screen_upd_humidity(uint8_t value)
{
	screen_set_num_value(SCREEN_HUMIDITY, value, 2, 2, 1);
}