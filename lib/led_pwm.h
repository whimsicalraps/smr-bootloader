#pragma once

#include <stm32f7xx.h>

#define LED_1_GPIO_RCC()	__HAL_RCC_GPIOD_CLK_ENABLE()
#define LED_1_GPIO_RCC_D()	__HAL_RCC_GPIOD_CLK_DISABLE()
#define LED_1_GPIO			GPIOD

#define LED_L_OVER			GPIO_PIN_3
#define LED_R_OVER			GPIO_PIN_6
#define LED_R_BOTTOM		GPIO_PIN_7
#define LED_L_TOP			GPIO_PIN_9
#define LED_L_MID			GPIO_PIN_12
#define LED_L_BOTTOM		GPIO_PIN_13

#define LED_2_GPIO_RCC()	__HAL_RCC_GPIOB_CLK_ENABLE()
#define LED_2_GPIO_RCC_D()	__HAL_RCC_GPIOB_CLK_DISABLE()
#define LED_2_GPIO			GPIOB

#define LED_R_MID			GPIO_PIN_3
#define LED_R_TOP			GPIO_PIN_4
#define LED_R_MODE			GPIO_PIN_5
#define LED_L_MODE			GPIO_PIN_15


#define PWM_LEVELS 32

typedef enum 	// pin 				// port
	{ MODE_L	// LED_L_MODE		LED_2_GPIO
	, ACTION_L	// LED_L_TOP		LED_1_GPIO
	, MOTOR_L	// LED_L_MID		LED_1_GPIO
	, RECORD_L	// LED_L_BOTTOM		LED_1_GPIO
	, OVER_L	// LED_L_OVER		LED_1_GPIO
	, MODE_R	// LED_R_MODE		LED_2_GPIO
	, ACTION_R	// LED_R_TOP		LED_2_GPIO
	, MOTOR_R	// LED_R_MID		LED_2_GPIO
	, RECORD_R	// LED_R_BOTTOM		LED_1_GPIO
	, OVER_R	// LED_R_OVER		LED_1_GPIO
	, PWM_COUNT // < length of the enum
	} ONE_led_t;

typedef enum
	{ pwm_main
	, pwm_global
	, pwm_tapes
	, pwm_page_count
	} pwm_page;

void PWM_Init( void );
void PWM_Deinit( void );
void PWM_set_all( float level, pwm_page alt );
void PWM_set_level( ONE_led_t ch, float level, pwm_page alt );
void PWM_step( pwm_page alt );

// higher-level functions
void led_motor_cue( float position );
void led_motor_off( void );

void led_boot_in( float level );
void led_sprite( int8_t new_sprite );
void led_boot_all( float level );
void led_boot_wait( void );
void led_boot_rx( void );
void led_boot_error( void );
void led_boot_write( void );
