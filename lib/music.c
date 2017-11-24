#include "music.h"

#include <math.h>
#include "wrOscSine.h"
#include "wrMath.h"

osc_sine_t sine_struct[4];

float just_pitch( int8_t num, int8_t deno );

void init_music_maker( void )
{
    // we are the music makers
    for( uint8_t j=0; j<4; j++ ){
        osc_sine_init( &sine_struct[j] );
        osc_sine_time( &sine_struct[j]
                     , 0.01
                       * just_pitch( j+5, 6-j )
                     );
    }
}

int32_t* make_music( int32_t* output
                   , float    samp_dir
                   )
{
    // determine rhythm & pitch
    // based on history
    // block counter
    // samp_dir param input (and transformations of)
    //
    // then just run the 4 oscillator process fns

    static int16_t melody_hold = 1;

    int16_t j;
//    if( samp_dir != 0.0
//     && !(--melody_hold) ){
//        melody_hold = 187; // something like 16s at 160bpm+~
//
//        // use ASR style pitch assignment for the oscillators
//        // simple LUT of nice harmonics w/ a wide-pitch range
//        // each new firmware will play a different sequence
//        //
//        // perhaps the ASR could be triggered by different things
//        // voices 3/4 get triggered if >16 samples in either dir
//        // 1/2 always trail
//        //
//        // rhythm counter (melody_hold) incremented by ui_level
//        // destructively change the arpeggio rate over time.
//        // bounds checking & perhaps inversion or overflow?
//        //
//        // osc_sine_time( &sine_struct[x], 0.0-1.0 );
//    }

    // now we generate the tones
    float zeroes[BLOCK_SIZE];
    float ones[BLOCK_SIZE];
    float soutput[BLOCK_SIZE];
    float moutput[BLOCK_SIZE];
    float* sout = soutput;
    float* mout = moutput;
    for( uint16_t i=0; i<BLOCK_SIZE; i++ ){
        zeroes[i] = 0.0;
        ones[i] = 1.0;
        moutput[i] = 0.0;
    }
    for( j=0; j<4; j++ ){
        osc_sine_process_v( &sine_struct[j]
                          , BLOCK_SIZE
                          , ones
                          , zeroes
                          , soutput
                          );
     	muladd_vf_f_vf( soutput
		              , 0.125
		              , moutput
		              , moutput
		              , BLOCK_SIZE
				      );
    }

	lim_vf_audio( moutput, BLOCK_SIZE );
    // convert floats to int32 output>>>
    for( uint16_t i=0; i<BLOCK_SIZE; i++ ){
	    *output++ = (int32_t)(*mout++ * -MAX32B) >> 8;
    }
}


float just_pitch( int8_t num, int8_t deno )
{
	return (logf( (float)(num)/(float)(deno) ) / logf( 2.0 ));
}

