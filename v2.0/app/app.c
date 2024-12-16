#include "config_system.h"
#include "console.h"
#include "crc.h"
#include "dht22.h"
#include "display.h"
#include "fw_header.h"
#include "i2c_common.h"
#include "mh_z19.h"
#include "ms5611.h"
#include "platform.h"
#include "prof.h"
#include "ret_mem.h"
#include "screen.h"
#include "usb_core.h"
#include "usb_core_cdc.h"
#include "usb_hw.h"
#include "usb_lib.h"
#include "ws2812.h"

#define SYSTICK_IN_MS (72000000 / 1000)

bool g_stay_in_boot = false;
volatile uint32_t system_time_ms = 0;
uint32_t g_uid[3];

uint8_t tmp = 0xAA;
config_entry_t g_device_config[] = {{"temp", sizeof(tmp), 0, &tmp}};
const uint32_t g_device_config_count = sizeof(g_device_config) / sizeof(g_device_config[0]);

static int32_t prev_systick = 0;
static struct
{
	uint8_t dht_t;
	uint8_t dht_h;
} dht_last = {0};

void delay_ms(volatile uint32_t delay_ms)
{
	volatile uint32_t start = 0;
	int32_t mark_prev = 0;
	prof_mark(&mark_prev);
	const uint32_t time_limit = delay_ms * SYSTICK_IN_MS;
	for(;;)
	{
		start += (uint32_t)prof_mark(&mark_prev);
		if(start >= time_limit) return;
	}
}

static int map_int(int x, int in_min, int in_max, int out_min, int out_max)
{
	if(x < in_min) x = in_min;
	if(x > in_max) x = in_max;
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void main(void)
{
	RCC->AHBENR |= RCC_AHBENR_CRCEN;

	platform_get_uid(g_uid);

	prof_init();
	platform_watchdog_init();

	platform_init();

	fw_header_check_all();

	ret_mem_init();
	ret_mem_set_load_src(LOAD_SRC_APP); // let preboot know it was booted from app

	if(config_validate() == CONFIG_STS_OK) config_read_storage();

	usb_init();

	display_init();
	screen_init();

	screen_upd_temperature(256);
	screen_upd_humidity(30);
	screen_upd_co2_fail();

	i2c_init();
	ms5611_init();
	ws2812_init();
	mh_z19_init();
	dht11_init();

	for(;;)
	{
		uint32_t time_diff_systick = (uint32_t)prof_mark(&prev_systick);

		static uint32_t /*remain_systick_us_prev = 0, */ remain_systick_ms_prev = 0;
		// uint32_t diff_us = (time_diff_systick + remain_systick_us_prev) / (SYSTICK_IN_US);
		// remain_systick_us_prev = (time_diff_systick + remain_systick_us_prev) % SYSTICK_IN_US;

		uint32_t diff_ms = (time_diff_systick + remain_systick_ms_prev) / (SYSTICK_IN_MS);
		remain_systick_ms_prev = (time_diff_systick + remain_systick_ms_prev) % SYSTICK_IN_MS;

		system_time_ms += diff_ms;
		platform_watchdog_reset();

		usb_poll(diff_ms);

		static uint32_t cnt = 0, cnt2 = 0, cnt3 = 0;
		cnt += diff_ms;
		if(cnt > 500)
		{
			cnt = 0;
			static uint8_t buf[5];
			int sts_dht = dht11_poll(buf);

			if(sts_dht == DHT11_OK)
			{
				screen_upd_humidity(buf[0]);
				// screen_upd_temperature(buf[2] * 10);
				dht_last.dht_h = buf[0];
				dht_last.dht_t = buf[2];
			}
			else if(sts_dht == DHT11_CS_ERROR)
			{
				screen_upd_humidity(1);
			}
			else if(sts_dht == DHT11_NO_CONN)
			{
				screen_upd_humidity(2);
			}

			screen_upd_pressure(ms5611_meas.p_0_01mmhg / 100);
			screen_upd_temperature(ms5611_meas.temp_0_01C / 10);
		}

		mh_z19_poll(diff_ms);
		ms5611_poll(diff_ms);

		cnt2 += diff_ms;
		if(cnt2 > 2)
		{
			cnt2 = 0;
			ws2812_set_color((color_t[]){hsv2rgb(map_int(mx_z19_get_co2(), 400, 900, 240, 360),
												 1,
												 map_int(mx_z19_get_co2(), 400, 900, 0, 60))});
		}

		screen_poll(diff_ms);

		if(mx_z19_get_co2() == MH_Z19_INVALID_VALUE)
		{
			screen_upd_co2_fail();
		}
		else
		{
			screen_upd_co2(mx_z19_get_co2());
		}

		if(usb_prop.bDeviceState == CONFIGURED)
		{
			cnt3 += diff_ms;
			if(cnt3 > 1200)
			{
				cnt3 = 0;
				console_print("CO2: %d ppm | t (MH-Z19/DHT22/MS5611): %d/%d/%ld.%02ld *C | h: %d %% | P: %ld Pa / %ld.%02ld mmHg\n",
							  mx_z19_get_co2(),
							  mx_z19_get_temp(), dht_last.dht_t, ms5611_meas.temp_0_01C / 100, ms5611_meas.temp_0_01C % 100,
							  dht_last.dht_h,
							  ms5611_meas.p_pa, ms5611_meas.p_0_01mmhg / 100, ms5611_meas.p_0_01mmhg % 100);
			}
		}
	}
}

void usbd_cdc_rx(const uint8_t *data, uint32_t size)
{
	usbd_cdc_unlock();
	console_cb((const char *)data, size);
}