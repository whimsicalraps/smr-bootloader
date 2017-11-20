#pragma once

#include <stm32f7xx.h>

#define DBG_METER_GPIO_RCC()	__HAL_RCC_GPIOA_CLK_ENABLE()
#define DBG_METER_GPIO			GPIOA
#define DBG_METER_BM_0			GPIO_PIN_6
#define DBG_METER_BM_1			GPIO_PIN_7

#define DBG_LED_GPIO_RCC()		__HAL_RCC_GPIOB_CLK_ENABLE()
#define DBG_LED_GPIO			GPIOB
#define DBG_LED_X				GPIO_PIN_8
#define DBG_LED_Y				GPIO_PIN_9

typedef enum
	{ bm_0
	, bm_1
	, led_x
	, led_y
	} debug_pins_t;

void Debug_HW_Init( void );
void Debug_HW_set( debug_pins_t chan, uint8_t state );
void Debug_HW_toggle( debug_pins_t chan );
