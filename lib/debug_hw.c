#include "debug_hw.h"

#ifdef RELEASE
void Debug_HW_Init( void ){ return; }
void Debug_HW_set( uint8_t chan, uint8_t state ){ return; }
void Debug_HW_toggle( uint8_t chan ){ return; }
#endif // RELEASE

#ifdef DEBUG
void Debug_HW_Init( void )
{
	GPIO_InitTypeDef gpio;

	DBG_METER_GPIO_RCC();
	gpio.Pin  = DBG_METER_BM_0
			  | DBG_METER_BM_1
			  ;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	HAL_GPIO_Init( DBG_METER_GPIO, &gpio );

	DBG_LED_GPIO_RCC();
	gpio.Pin  = DBG_LED_X
			  | DBG_LED_Y
			  ;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	HAL_GPIO_Init( DBG_LED_GPIO, &gpio );
}

void Debug_HW_Deinit( void )
{
	DBG_METER_GPIO_RCC_D();
	DBG_LED_GPIO_RCC_D();
}

const uint32_t ch[4] = { DBG_METER_BM_0
                       , DBG_METER_BM_1
					   , DBG_LED_X
					   , DBG_LED_Y
					   };
const GPIO_TypeDef* port[4] = { DBG_METER_GPIO
                              , DBG_METER_GPIO
					          , DBG_LED_GPIO
					          , DBG_LED_GPIO
					          };
void Debug_HW_set( debug_pins_t chan, uint8_t state )
{
	const uint8_t st[2] = { GPIO_PIN_RESET
	                      , GPIO_PIN_SET
						  };
	HAL_GPIO_WritePin( (GPIO_TypeDef*)port[chan]
	                 , ch[chan]
					 , st[state]
					 );
}

void Debug_HW_toggle( debug_pins_t chan )
{
	HAL_GPIO_TogglePin( (GPIO_TypeDef*)port[chan]
		              , ch[chan]
		              );
}
#endif // DEBUG
