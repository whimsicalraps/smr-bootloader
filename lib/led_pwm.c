#include "led_pwm.h"

#include "wrMath.h"

int8_t sprite;

uint8_t pwm_states[pwm_page_count][PWM_COUNT]; // leds (2nd array for globals)

void PWM_Init( void )
{
	LED_1_GPIO_RCC();
	GPIO_InitTypeDef gpio;
	gpio.Pin = LED_L_OVER
	         | LED_R_OVER
	         | LED_R_BOTTOM
	         | LED_L_TOP
	         | LED_L_MID
	         | LED_L_BOTTOM
	         ;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(LED_1_GPIO, &gpio);

	LED_2_GPIO_RCC();
    gpio.Pin = LED_R_MID
             | LED_R_TOP
             | LED_R_MODE
             | LED_L_MODE
             ;
	HAL_GPIO_Init(LED_2_GPIO, &gpio);

	uint8_t* pwm_init = pwm_states[0];
	for( uint8_t i=0; i<PWM_COUNT*2; i++ ){
		*pwm_init++ = 0;
	}
    sprite = -1;
}

void PWM_Deinit( void )
{
	LED_1_GPIO_RCC_D();
	LED_2_GPIO_RCC_D();
}

void PWM_set_all( float level, pwm_page alt )
{
	const float pwm_max = (float)PWM_LEVELS;

	uint8_t tmp = (uint8_t)(lim_f_0_1( level ) * pwm_max);
	for( uint8_t i=0; i<PWM_COUNT; i++ ){
		pwm_states[alt][i] = tmp;
	}
}

void PWM_set_level( ONE_led_t ch, float level, pwm_page alt )
{
	const float pwm_max = (float)PWM_LEVELS;

	pwm_states[alt][ch] = (uint8_t)(lim_f_0_1( level ) * pwm_max);
}

void ll_led_set( uint8_t ch, uint8_t state );

void PWM_step( pwm_page alt )
{
	static uint8_t ix;

    if( sprite != -1 ){
        switch( sprite ){
            case 0: // waiting
                led_boot_wait();
                break;
            case 1: //UI_STATE_RECEIVING:
                led_boot_rx();
                break;
            case 2: //UI_STATE_ERROR:
                led_boot_error();
                break;
            case 3: //UI_STATE_WRITING:
                led_boot_write();
                break;
        }
    }
    // process leds
    for( uint8_t i=0; i<PWM_COUNT; i++ ){
		// refactor to only call change on state change
		ll_led_set( i
			      , (pwm_states[alt][i] > ix)
			      );
	}
	ix++; if(ix >= PWM_LEVELS){ ix = 0; } // next step
}

void ll_led_set( uint8_t ch, uint8_t state )
{
	const uint8_t st[2] = { GPIO_PIN_RESET
		                  , GPIO_PIN_SET
		                  };
	const GPIO_TypeDef* port[10] = { LED_2_GPIO
							       , LED_1_GPIO
							       , LED_1_GPIO
							       , LED_1_GPIO
							       , LED_1_GPIO
							       , LED_2_GPIO
								   , LED_2_GPIO
								   , LED_2_GPIO
								   , LED_1_GPIO
								   , LED_1_GPIO
							       };
	const uint32_t pin[10] = { LED_L_MODE
		                     , LED_L_TOP
							 , LED_L_MID
							 , LED_L_BOTTOM
							 , LED_L_OVER
							 , LED_R_MODE
							 , LED_R_TOP
							 , LED_R_MID
							 , LED_R_BOTTOM
							 , LED_R_OVER
							 };

	HAL_GPIO_WritePin( (GPIO_TypeDef*)port[ch]
	                 , pin[ch]
					 , st[state]
					 );	
}

void led_motor_cue( float position )
{
	PWM_set_level( MOTOR_L, 1.0-position, 0 );
	//PWM_set_level( MOTOR_L2, position, 0 );
}

void led_motor_off( void )
{
	PWM_set_level( MOTOR_L, 0.0, 0 );
	//PWM_set_level( MOTOR_L2, position, 0 );
}


// bootloader animations
void led_sprite( int8_t new_sprite )
{
    sprite = new_sprite;
}
void led_boot_in( float level )
{
    PWM_set_level( OVER_L
                 , -level / 24.0
                 , pwm_main
                 );
    PWM_set_level( OVER_R
                 , level / 24.0
                 , pwm_main
                 );
}
void led_boot_all( float level )
{
   	const float pwm_max = (float)PWM_LEVELS;

	uint8_t tmp = (uint8_t)(lim_f_0_1( level ) * pwm_max);
	for( uint8_t i=0; i<4; i++ ){
		pwm_states[0][i] = tmp;
		pwm_states[0][i+5] = tmp;
	}
}
void led_boot_wait( void )
{
    static float state = 0.0;
    float astate = state*state;
    led_boot_all( 0.0 );
    PWM_set_level( RECORD_R
                 , _Abs(astate)
                 , pwm_main
                 );
    state += 0.0007;
    if(state > 1.0){ state = -1.0; }
}
void led_boot_rx( void )
{
    static float state = 0.0;
    float astate = _Abs(state);
    PWM_set_level( MODE_R
                 , astate-3.0
                 , pwm_main
                 );
    PWM_set_level( ACTION_R
                 , astate-2.0
                 , pwm_main
                 );
    PWM_set_level( MOTOR_R
                 , astate-1.0
                 , pwm_main
                 );
    PWM_set_level( RECORD_R
                 , astate
                 , pwm_main
                 );
    state += 0.001;
    if(state > 4.0){ state = -4.0; }
}
void led_boot_error( void )
{
    static int16_t count = 0;
    static int8_t pulse = 0;
    static float state = 0.0;
    led_boot_all( 0.0 );
    PWM_set_level( RECORD_L
                 , state
                 , pwm_main
                 );
    count++;
    if( count > 280
     && pulse < 3 ){ state = 1.0; }
    if( count > 350 ){
        count = 0;
        state = 0.0;
        pulse++;
        if( pulse > 5 ){ pulse = 0; }
    }
}
void led_boot_write( void )
{
    static float state = 0.0;
    state += 0.001;
    if(state > 1.0){ state = -1.0; }
    float astate = state*state;
    astate = _Abs(astate);
    PWM_set_level( MODE_L
                 , astate
                 , pwm_main
                 );
    PWM_set_level( ACTION_R
                 , astate
                 , pwm_main
                 );
    PWM_set_level( MOTOR_L
                 , astate
                 , pwm_main
                 );
    PWM_set_level( RECORD_R
                 , astate
                 , pwm_main
                 );

    PWM_set_level( MODE_R
                 , 1.0-astate
                 , pwm_main
                 );
    PWM_set_level( ACTION_L
                 , 1.0-astate
                 , pwm_main
                 );
    PWM_set_level( MOTOR_R
                 , 1.0-astate
                 , pwm_main
                 );
    PWM_set_level( RECORD_L
                 , 1.0-astate
                 , pwm_main
                 );

}
