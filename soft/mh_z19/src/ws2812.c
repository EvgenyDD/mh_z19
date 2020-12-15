#include "ws2812.h"
#include "main.h"

#include <string.h>

extern SPI_HandleTypeDef hspi2;

#define BITS_PER_SIG 15

#define LO BITS_PER_SIG / 4
#define HI BITS_PER_SIG * 3 / 4

static uint8_t tx_arr[BITS_PER_SIG * 8 * 3 + 5];

static void set_pattern(uint32_t sig, bool set)
{
    uint32_t start = sig * BITS_PER_SIG + 5;
    uint32_t end = start + BITS_PER_SIG - 1;
}

void ws2812_init(void)
{
    memset(tx_arr, 0, sizeof(tx_arr));
    for(uint32_t i = 0; i < BITS_PER_SIG * 8 * 3; i++)
    {
        uint32_t pos = i % BITS_PER_SIG;
        if(pos < 5)
            tx_arr[i / 8] |= (1 << (i % 8));
        else
            tx_arr[i / 8] &= ~(1 << (i % 8));
    }
    color_t c = {0};
    c.b = 255;
    ws2812_set_color(&c);
}

void ws2812_poll(void) { HAL_SPI_Transmit_DMA(&hspi2, tx_arr, sizeof(tx_arr)); }

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