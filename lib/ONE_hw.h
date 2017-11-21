#pragma once

#include <stm32f7xx.h>

#define ONE_PUSH_GPIO_RCC()		__HAL_RCC_GPIOD_CLK_ENABLE()
#define ONE_PUSH_GPIO_RCC_D()	__HAL_RCC_GPIOD_CLK_DISABLE()
#define ONE_PUSH_GPIO			GPIOD
#define ONE_PUSH_MOTOR			GPIO_PIN_8
#define ONE_PUSH_TOUCH			GPIO_PIN_11
#define ONE_PUSH_CHALK			GPIO_PIN_10

#define ONE_TOG_GPIO_RCC()		__HAL_RCC_GPIOB_CLK_ENABLE()
#define ONE_TOG_GPIO_RCC_D()	__HAL_RCC_GPIOB_CLK_DISABLE()
#define ONE_TOG_GPIO			GPIOB
#define ONE_TOG_UP				GPIO_PIN_14
#define ONE_TOG_DOWN			GPIO_PIN_13

#define ONE_DETECT_GPIO_RCC()	__HAL_RCC_GPIOD_CLK_ENABLE()
#define ONE_DETECT_GPIO_RCC_D()	__HAL_RCC_GPIOD_CLK_DISABLE()
#define ONE_DETECT_GPIO			GPIOD
#define ONE_DETECT_IN			GPIO_PIN_4
#define ONE_DETECT_OUT			GPIO_PIN_5
#define ONE_DETECT_CV2			GPIO_PIN_1

#define ONE_DETECT_CV_GPIO_RCC()	__HAL_RCC_GPIOA_CLK_ENABLE()
#define ONE_DETECT_CV_GPIO_RCC_D()	__HAL_RCC_GPIOA_CLK_DISABLE()
#define ONE_DETECT_CV_GPIO			GPIOA
#define ONE_DETECT_CV				GPIO_PIN_15

#define ONE_SENSE_GPIO_RCC()	__HAL_RCC_GPIOC_CLK_ENABLE()
#define ONE_SENSE_GPIO_RCC_D()	__HAL_RCC_GPIOC_CLK_DISABLE()
#define ONE_SENSE_GPIO			GPIOC
#define ONE_SENSE_PIN			GPIO_PIN_0
#define ONE_SENSE_IRQn			EXTI0_IRQn

typedef enum
	{ adda_draw
	, adda_paint
	, adda_count
	} adda_channel;

typedef enum
	{ adda_input
	, adda_output
	} adda_mode;

void ONE_HW_Init(void);
void ONE_HW_Deinit(void);

// Public get-ui-state access
uint8_t ONE_getstates( uint8_t* mask );
uint8_t ONE_getdetects( uint8_t* mask );

void set_adda_mode( uint8_t ch, adda_mode output );

// Power Sense IRQ Handler
void EXTI0_IRQHandler(void);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);