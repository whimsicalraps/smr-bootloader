#include "music.h"

#include <math.h>
#include "wrOscSine.h"
#include "wrLpGate.h"
#include "wrMath.h"

osc_sine_t sine_struct[4];
osc_sine_t sine_mod[2];
lpgate_t   lpgate_struct[4];

#define JUST_OFFSET 0.015

float just_pitch( int8_t num, int8_t deno );

void init_music_maker( void )
{
    // we are the music makers
    for( uint8_t j=0; j<4; j++ ){
        osc_sine_init( &sine_struct[j] );
        lpgate_init( &lpgate_struct[j], 1, 1, BLOCK_SIZE );
    }
    osc_sine_time( &sine_struct[0], JUST_OFFSET * just_pitch( 4, 3 ) );
    osc_sine_time( &sine_struct[1], JUST_OFFSET * just_pitch( 3, 2 ) );
    osc_sine_time( &sine_struct[2], JUST_OFFSET * just_pitch( 18,8  ) );
    osc_sine_time( &sine_struct[3], JUST_OFFSET * just_pitch( 27,16 ) );
    
    osc_sine_init( &sine_mod[0] );
    osc_sine_time( &sine_mod[0], 0.000023 );
    osc_sine_init( &sine_mod[1] );
    osc_sine_time( &sine_mod[1], 0.000027 );
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
    static int8_t ratios[2][2] = {{1, 1}, {1, 1}};
    const int8_t ilookup[2][24] = {
        { 0, 1, 3, 4, 6, 15, 12, 14, 18, 24, 15, 44, 36, 26, 35, 40, 27, 34, 9,  19, 15, 21, 44, 69 },
        { 0, 2, 5, 6, 3, 15, 9,  21, 9,  57, 45, 33, 81, 39, 63, 45, 27, 51, 27, 19, 25, 21, 66, 23 }};
    static uint8_t switcher = 0;
    int16_t j;
    if( samp_dir != 0.0
     && !(--melody_hold) ){
        melody_hold = 384*(int16_t)_Abs(samp_dir); // something like 8ths at 160bpm+~

        // use ASR style pitch assignment for the oscillators
        // simple LUT of nice harmonics w/ a wide-pitch range
        // each new firmware will play a different sequence
        //
        // perhaps the ASR could be triggered by different things
        // voices 3/4 get triggered if >16 samples in either dir
        // 1/2 always trail
        //
        // rhythm counter (melody_hold) incremented by ui_level
        // destructively change the arpeggio rate over time.
        // bounds checking & perhaps inversion or overflow?
         
        ratios[switcher][samp_dir > 0.0] = (int8_t)_Abs(samp_dir);
        osc_sine_time( &sine_struct[switcher]
                     , JUST_OFFSET
                       * just_pitch( ratios[switcher][0]
                                   , ilookup[0][ratios[switcher][0]] )
                     );
        osc_sine_time( &sine_struct[switcher+2]
                     , JUST_OFFSET
                       * just_pitch( ilookup[1][ratios[switcher][0]]
                                   , ratios[switcher][0] )
                       //* just_pitch( ratios[switcher][1]
                        //           , ilookup[1][ratios[switcher][1]] )
                     );
        switcher ^= 1;
    }

    // now we generate the tones
    float zeroes[BLOCK_SIZE];
    float zeroes2[BLOCK_SIZE];
    float ones[BLOCK_SIZE];
    float lpmod[2][BLOCK_SIZE];
    float soutput[BLOCK_SIZE];
    float lpout[BLOCK_SIZE];
    float moutput[BLOCK_SIZE];
    float* sout = soutput;
    float* mout = moutput;
    for( uint16_t i=0; i<BLOCK_SIZE; i++ ){
        zeroes[i]  = 0.0;
        zeroes2[i] = 0.0;
        ones[i]    = 1.0;
        lpmod[0][i]= 0.0;
        lpmod[1][i]= 0.0;
        lpout[i]   = 0.0;
        moutput[i] = 0.0;
    }
    for( j=0; j<2; j++ ){
        osc_sine_process_v( &sine_mod[j]
                          , BLOCK_SIZE
                          , ones
                          , zeroes2
                          , lpmod[j] );
        mul_vf_f( lpmod[j], 0.49, lpmod[j], BLOCK_SIZE );
        add_vf_f( lpmod[j], 0.5, lpmod[j], BLOCK_SIZE );
    }
    for( j=0; j<4; j++ ){
        osc_sine_process_v( &sine_struct[j]
                          , BLOCK_SIZE
                          , ones
                          , zeroes
                          , soutput
                          );
        lpgate_v( &lpgate_struct[j], lpmod[j&1], soutput, lpout );
     	muladd_vf_f_vf( lpout
		              , 0.125
		              , moutput
		              , moutput
		              , BLOCK_SIZE
				      );
        if( j>1 ){
            mul_vf_f( soutput, 0.004, zeroes, BLOCK_SIZE );
        }
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

