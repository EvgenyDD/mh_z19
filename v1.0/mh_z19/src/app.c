#include "adc.h"
#include "debug.h"
#include "dht22.h"
#include "display.h"
#include "flash.h"
#include "helper.h"
#include "main.h"
#include "mh_z19.h"
#include "screen.h"
#include "usbd_cdc_if.h"
#include "ws2812.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern CRC_HandleTypeDef hcrc;
extern TIM_HandleTypeDef htim1;
extern USBD_HandleTypeDef hUsbDeviceFS;

static inline int map(int x, int in_min, int in_max, int out_min, int out_max)
{
	if(x < in_min)
	{
		x = in_min;
	}
	if(x > in_max)
	{
		x = in_max;
	}
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void loop(void)
{
	// debug_init();
	// adc_init();

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = WS2812_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	hal_flash_unlock();

	ws2812_init();

	display_init();
	screen_init();

	screen_upd_temperature(256);
	screen_upd_humidity(56);

	mh_z19_init();
	dht11_init();

	while(1)
	{
		static uint32_t time_ms_prev = 0;
		uint32_t time_ms_now = HAL_GetTick();
		uint32_t diff_ms = time_ms_now < time_ms_prev
							   ? 0xFFFFFFFFUL + time_ms_now - time_ms_prev
							   : time_ms_now - time_ms_prev;
		time_ms_prev = time_ms_now;

		static uint32_t prev_tick = 0;
		if(prev_tick < HAL_GetTick())
		{
			prev_tick = HAL_GetTick() + 500;
			static uint8_t buf[5];
			int sts_dht = dht11_poll(buf);

			if(sts_dht == DHT11_OK)
			{
				screen_upd_humidity(buf[0]);
				screen_upd_temperature(buf[2] * 10);
			}
			else if(sts_dht == DHT11_CS_ERROR)
			{
				screen_upd_humidity(1);
			}
			else if(sts_dht == DHT11_NO_CONN)
			{
				screen_upd_humidity(2);
			}
		}

		mh_z19_poll(diff_ms);

		static uint32_t prev_ts = 0;
		prev_ts += diff_ms;
		if(prev_ts > 2) ws2812_set_color((color_t[]){hsv2rgb(map(mx_z19_get_co2(), 400, 900, 240, 360),
															 1,
															 map(mx_z19_get_co2(), 400, 900, 0, 60))});

		screen_poll(diff_ms);

		if(mx_z19_get_co2() == MH_Z19_INVALID_VALUE)
		{
			screen_upd_co2_fail();
		}
		else
		{
			screen_upd_co2(mx_z19_get_co2());
		}

		// int32_t lvl = (prev_tick - HAL_GetTick());
		// float v = interval_hit_dim(lvl, 0, 500, 1000);
		// display_set_pwm(0, 0.25);
		// display_set_pwm(1, 0.5);
		// display_set_pwm(2, v);
		// display_set_pwm(3, v);

		// if(hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) to_without_press = 0;
	}
}

static void tx(uint8_t *buf, uint32_t len)
{
	CDC_Transmit_FS(buf, len);
}

void usb_if_rx(uint8_t *buf, uint32_t len)
{
	tx(buf, len);
}

inline static void memcpy_volatile(void *dst, const volatile void *src, size_t size)
{
	for(uint32_t i = 0; i < size; i++)
	{
		*((uint8_t *)dst + i) = *((const volatile uint8_t *)src + i);
	}
}

void process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len)
{
	// // debug("RX: %d > Size: %d | %d %d %d %d\n", sender_node_id, data_len, *(data-4), *(data-3),*(data-2),*(data-1));
	// // debug(DBG_INFO"R %d > %d\n", sender_node_id, data[0]);
	// if(data_len > 0)
	// {
	//     switch(data[0])
	//     {
	//     case RFM_NET_CMD_DEBUG:
	//         memcpy_volatile(dbg_buffer, data + 1, (size_t)(data_len - 1));
	//         if(dbg_buffer[data_len - 2] != '\n')
	//         {
	//             dbg_buffer[data_len - 1] = '\n';
	//             dbg_buffer[data_len - 0] = '\0';
	//         }
	//         else
	//         {
	//             dbg_buffer[data_len - 1] = '\0';
	//         }
	//         debug("[***]\t%s\n", dbg_buffer);
	//         // for(uint8_t i = 1; i < data_len; i++)
	//         // debug("%c", data[i]);
	//         // if(data[data_len - 1] != '\n') debug("\n");
	//         break;

	//         // case RFM_NET_CMD_HB:
	//         // {
	//         //     if(data_len == 2)
	//         //     {
	//         //         bool not_found = hb_tracker_update(sender_node_id);
	//         //         if(not_found) debug("HB unknown %d\n", sender_node_id);
	//         //         if(sender_node_id == RFM_NET_ID_HEAD)
	//         //         {
	//         //             to_from_head = data[1];
	//         //         }
	//         //     }
	//         // }
	//         // break;

	//     case RFM_NET_CMD_FLASH:
	//     {
	//         static uint8_t temp_data[CDC_DATA_FS_OUT_PACKET_SIZE];
	//         uint32_t sz = data_len < CDC_DATA_FS_OUT_PACKET_SIZE ? data_len : CDC_DATA_FS_OUT_PACKET_SIZE;
	//         for(uint32_t i = 0; i < sz; i++)
	//         {
	//             temp_data[i] = data[i];
	//         }
	//         CDC_Transmit_FS(temp_data, sz);
	//     }
	//     break;

	//     default:
	//         debug(DBG_WARN "Unknown cmd %d\n", data[0]);
	//         break;
	//     }
	// }
}