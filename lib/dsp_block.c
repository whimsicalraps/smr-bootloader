#include "dsp_block.h"

void DSP_Block_Process( __IO int32_t* in_codec
	                  , __IO int32_t* out_codec
	                  ,      uint16_t b_size
	                  )
{
	;;
}
/*
#include <stdlib.h>

#include "sd_ll.h"
#include "sdio.h"
#include "debug_usart.h"
#include "tape.h"
#include "wrMath.h"
#include "led_pwm.h"
#include "ONE_hw.h"
#include "statem.h"
#include "adc.h"
#include "dac.h"

#include "wrLpGate.h"

uint32_t   s_rate     = 48000; // default

tapedeck_t   tapedeck;
statem_t     statem;
filter_lp1_t over_led;
lpgate_t     logate;

// private func prototypes
void codec_to_floats(uint16_t b_size, __IO int32_t* codec, float* f);
void floats_to_codec(uint16_t b_size, float* f, __IO int32_t *codec);

// exported functions
float* fb_buf;
void DSP_Block_Init( uint16_t sample_rate
	               , uint16_t b_size
	               )
{
	s_rate = sample_rate;
	tape_init(&tapedeck, b_size);
	sm_init( &statem );
	lp1_init( &over_led ); lp1_set_coeff( &over_led, 0.05 );
	fb_buf = malloc(sizeof(float) * b_size);
	lpgate_init( &logate
		       , LPGATE_HPF_ON
		       , LPGATE_FILTER
		       , b_size
		       );
}

void DSP_Block_Process( __IO int32_t* in_codec
	                  , __IO int32_t* out_codec
	                  ,      uint16_t b_size
	                  )
{
	////////////////////////////////////////////////////////
	//////////////////// PARAM SETUP ///////////////////////
	////////////////////////////////////////////////////////

	// UI state machine
	static uint8_t  state_mask;
	statem.timer++; // increment block timer
	if( ONE_getstates( &state_mask ) ){
		statem.dofun( &statem
				    , state_mask
		            , &tapedeck
		            );
		statem.timer = 0; // reset timer
		statem.pstate = state_mask; // save new mask after use
	}
	static uint8_t detect_mask;
	if( ONE_getdetects( &detect_mask ) ){
		//
	}

	// ADC?
	// float draw_cv  = get_adc( adda_draw  );
	// set_dac( adda_draw, draw_cv ); // feedback??

	// float adc_val = ONE_getadc( 0 );
	// tapedeck.feedback = ONE_getadc();

	PWM_step( statem.pwm_active );
	

	////////////////////////////////////////////////////////
	///////////////////// DSP PROCESS //////////////////////
	////////////////////////////////////////////////////////

	float	 in[b_size], out[b_size], in_copy[b_size], lpg_lev[b_size], tmp[b_size];
	float*   in_c = in_copy;
	uint16_t i, j;

	// Convert codec to buffers
	codec_to_floats(b_size, in_codec, in);

	// copy input (and add feedback here!)
	// for(i=0;i<b_size;i++){ lpg_lev[i] = draw_cv; }
	lpgate_v( &logate, lpg_lev, fb_buf, tmp );
	for(i=0;i<b_size;i++){
		in_copy[i] = in[i];// + tmp[i];
	}
	
	// Add scaled input to tape
	in_c = tape_process_block( &tapedeck
                             , in_copy
                             );
	
	// copy output (for feedback)
	for(i=0;i<b_size;i++){ fb_buf[i] = in_copy[i]; }
	
	muladd_vf_f_vf( in
		          , statem.actions.throughput
		          , in_copy
		          , out
		          , b_size
				  );


	// LED output level display
	float lev = lp1_step( &over_led, (*in) * (*in) ); // filtered square
	const float CLIP_LEVEL = 0.12;  // 10V p2p
	const float WARM_LEVEL = 0.001; // 1V p2p
	if(      lev > CLIP_LEVEL ){ PWM_set_level( OVER_L, 1.0 , 0 ); }
	else if( lev > WARM_LEVEL ){ PWM_set_level( OVER_L, 0.05, 0 ); }
	else                       { PWM_set_level( OVER_L, 0.0 , 0 ); }

	// Clip output to avoid overflow
	lim_vf_audio(out, b_size);

	// Convert & Output Buffers
	floats_to_codec(b_size, out, out_codec);
	
	// Initiate the next sd read/write if the card is free
	sd_try_rw();
	
	////////////////////////////////////////////////////////
	////////////////////// CTRL OUT ////////////////////////
	////////////////////////////////////////////////////////

	// DAC output jack if active

	// ii output if master (later later)
}

// private funcs
void codec_to_floats(      uint16_t b_size
	                , __IO int32_t* codec
	                ,      float*   f
	                )
{
	for( uint16_t i=0; i<b_size; i++ ){
		*f++ = (float)(*codec++ << 8) * iMAX32B_NORMALIZED;
	}
}

void floats_to_codec(      uint16_t b_size
	                ,      float*   f
	                , __IO int32_t* codec
	                )
{
	for( uint16_t i=0; i<b_size; i++ ){
		// use saturating intrinsics!
		*codec++ = (int32_t)(*f++ * -MAX32B) >> 8;
	}
}*/