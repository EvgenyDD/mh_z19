#include "config_system.h"
#include "console.h"
#include "crc.h"
#include "fw_header.h"
#include "platform.h"
#include "prof.h"
#include "ret_mem.h"
#include "usb_core.h"
#include "usb_core_cdc.h"
#include "usb_hw.h"

#define SYSTICK_IN_MS (72000000 / 1000)

bool g_stay_in_boot = false;
volatile uint32_t system_time_ms = 0;
uint32_t g_uid[3];

uint8_t tmp = 0xAA;
config_entry_t g_device_config[] = {{"rf_chnl", sizeof(tmp), 0, &tmp}};
const uint32_t g_device_config_count = sizeof(g_device_config) / sizeof(g_device_config[0]);

static int32_t prev_systick = 0;

void delay_ms(volatile uint32_t delay_ms)
{
	volatile uint32_t start = 0;
	int32_t mark_prev = 0;
	prof_mark(&mark_prev);
	const uint32_t time_limit = delay_ms * SYSTICK_IN_MS;
	for(;;)
	{
		start += (uint32_t)prof_mark(&mark_prev);
		if(start >= time_limit)
			return;
	}
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

	PIN_SET(DIG_0);
	PIN_RST(DIG_1);
	PIN_SET(DIG_3);

	PIN_SET_(GPIOB, 1 << 7);
	PIN_SET_(GPIOA, 1 << 13);

	for(;;)
	{
		uint32_t time_diff_systick = (uint32_t)prof_mark(&prev_systick);

		static uint32_t remain_systick_us_prev = 0, remain_systick_ms_prev = 0;
		// uint32_t diff_us = (time_diff_systick + remain_systick_us_prev) / (SYSTICK_IN_US);
		// remain_systick_us_prev = (time_diff_systick + remain_systick_us_prev) % SYSTICK_IN_US;

		uint32_t diff_ms = (time_diff_systick + remain_systick_ms_prev) / (SYSTICK_IN_MS);
		remain_systick_ms_prev = (time_diff_systick + remain_systick_ms_prev) % SYSTICK_IN_MS;

		system_time_ms += diff_ms;
		platform_watchdog_reset();

		usb_poll(diff_ms);

		PIN_WR_(GPIOB, 1 << 3, usb_prop.bDeviceState != 1);
		PIN_WR_(GPIOB, 1 << 7, usb_prop.bDeviceState != 2);
		PIN_WR_(GPIOA, 1 << 9, usb_prop.bDeviceState != 3);
		PIN_WR_(GPIOA, 1 << 10, usb_prop.bDeviceState != 4);
		PIN_WR_(GPIOB, 1 << 4, usb_prop.bDeviceState != 5);
	}
}

void usbd_cdc_rx(const uint8_t *data, uint32_t size)
{
	usbd_cdc_unlock();
	console_print("RX: %.*s\n", size, data);
}