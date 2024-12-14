#include "flash.h"
#include "flash_regions.h"
#include "main.h"

#include <stdbool.h>
#include <string.h>

static bool app_image_present = false, app_present = false, app_image_not_empty = false;
static volatile uint32_t flash_counter = 0;
static volatile uint32_t period = 120;
static volatile bool flash_fast = false;
static uint32_t app_image_end, app_end;

void HAL_SYSTICK_Callback(void)
{
    flash_counter++;

    if(flash_fast)
    {
        static uint32_t prev_tick = 0;
        prev_tick++;
        if((prev_tick % period) < 5)
        {
            LED_GPIO_Port->ODR |= LED_Pin;
        }
        else
        {
            LED_GPIO_Port->ODR &= (uint32_t) ~(LED_Pin);
        }
    }
}

static uint32_t seek_flash_start(uint32_t start, uint32_t end)
{
    uint8_t *_end = (uint8_t *)end;
    uint8_t *_start = (uint8_t *)start;
    for(; _end > _start; _end--)
    {
        if(*_end != 0xFF) return (uint32_t)_end;
    }
    return start;
}

static void flash_led(uint32_t count, uint32_t period_flash)
{
    for(flash_counter = 0; flash_counter < count * period_flash * 2;)
    {
        if((flash_counter % (2 * period_flash)) == 0)
        {
            LED_GPIO_Port->ODR |= LED_Pin;
        }
        else if((flash_counter % period_flash) == 0)
        {
            LED_GPIO_Port->ODR &= (uint32_t) ~(LED_Pin);
        }
        else
        {
            asm("nop");
        }
    }
}

static inline void goto_app(void)
{
    // flash_led(3, 250);

    __disable_irq();
    SysTick->CTRL = 0;
    // HAL_DeInit();
    for(size_t i = 0; i < 256; i++)
    {
        __NVIC_DisableIRQ(i);
        __NVIC_ClearPendingIRQ(i);
    }

    __enable_irq();

    SCB->VTOR = ADDR_APP;

    typedef void (*app_entry_t)(void);
    app_entry_t app = (app_entry_t)(*(uint32_t *)(ADDR_APP + 4));

    // for(volatile size_t i = 0; i < 100000; i++)
    // {
    //     asm("nop");
    // }
    // LED_GPIO_Port->ODR |= LED_Pin;

    // for(volatile size_t i = 0; i < 100000; i++)
    // {
    //     asm("nop");
    // }

    __set_MSP(*(uint32_t *)(ADDR_APP));
    app();
}

static bool compare(void)
{
    for(uint32_t i = ADDR_APP, j = ADDR_APP_IMAGE; i <= app_end || j < app_image_end; i++, j++)
    {
        if(*(uint8_t *)i != *(uint8_t *)j) return false;
    }
    return true;
}

static void copy_image(void)
{
    hal_flash_unlock();

    for(uint32_t i = ADDR_APP; i < ADDR_APP + LEN_APP; i += FLASH_PAGE_SIZE)
    {
        hal_flash_page_erase(i);
    }

    const uint16_t *data = (uint16_t *)ADDR_APP_IMAGE;
    for(uint32_t i = 0; i <= app_image_end - ADDR_APP_IMAGE; i += 2)
    {
        hal_flash_program_u16(ADDR_APP + i, data[i / 2]);
    }
    hal_flash_lock();
}

static void erase_app_image(void)
{
    hal_flash_unlock();

    for(uint32_t i = ADDR_APP_IMAGE; i < ADDR_APP_IMAGE + LEN_APP_IMAGE; i += FLASH_PAGE_SIZE)
    {
        hal_flash_page_erase(i);
    }

    hal_flash_lock();
}

void main(void)
{
    HAL_InitTick(TICK_INT_PRIORITY);

    SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOAEN);
    GPIOA->MODER |= (1 << (7 * 2));

    app_end = seek_flash_start(ADDR_APP, ADDR_APP + LEN_APP - 1);
    app_present = app_end != ADDR_APP;

    app_image_end = seek_flash_start(ADDR_APP_IMAGE, ADDR_APP_IMAGE + LEN_APP_IMAGE - 1);
    app_image_not_empty = app_image_end != ADDR_APP_IMAGE;
    app_image_present = app_image_not_empty && *(uint32_t *)ADDR_APP_IMAGE != 0xFFFFFFFF;

    // if(app_present)
    //     flash_led(1, 250);
    // else
    //     flash_led(2, 250);
    // HAL_Delay(800);

    // if(app_image_not_empty)
    //     flash_led(3, 250);
    // else
    //     flash_led(4, 250);
    // HAL_Delay(800);

    // if(app_image_present)
    //     flash_led(5, 250);
    // else
    //     flash_led(6, 250);
    // HAL_Delay(800);

    // return;

    if(app_present == false && app_image_not_empty == false)
    {
    }
    else if(app_present && app_image_not_empty == false) // erase app image & goto APP
    {
        if(*(uint32_t *)ADDR_APP_IMAGE != 0xFFFFFFFF)
        {
            erase_app_image();
            flash_led(2, 250);
        }
        // HAL_Delay(800);
        goto_app();
    }
    else if(app_present && app_image_not_empty)
    {
        bool equal = compare();
        if(equal == false && app_image_present)
        {
            copy_image();
            flash_led(1, 250);
        }
        {
            erase_app_image();
            flash_led(4, 250);
        }
        goto_app();
    }
    else if(app_present == false && app_image_not_empty) // copy code & erase app image & goto APP
    {
        if(app_image_present)
        {
            copy_image();
        }
        erase_app_image();
        flash_led(3, 250);
        goto_app();
    }

    while(1)
    {
        flash_fast = true;
    }
}
