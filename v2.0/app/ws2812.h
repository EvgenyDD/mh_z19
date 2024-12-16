#ifndef WS2812_H__
#define WS2812_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} color_t;

void ws2812_init(void);

void ws2812_poll(void);
void ws2812_set_color(color_t *color);

color_t hsv2rgb(float h, float s, float v);

#endif // WS2812_H__