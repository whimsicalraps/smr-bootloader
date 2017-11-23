// Bootloader.cc
// Copyright 2012 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
// Modified for SMR project: Dan Green (danngreen1@gmail.com) 2015

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.

#include <stm32f7xx.h>

// #include "system.h"

#include <cstring>


#include "../stmlib/dsp/dsp.h"
#include "../stmlib/utils/ring_buffer.h"
#include "../stmlib/system/flash_programming.h"

#include "../stm-audio-bootloader/fsk/packet_decoder.h"
#include "../stm-audio-bootloader/fsk/demodulator.h"


extern "C" {
#include "bootloader.h"

#include <stm32f7xx_hal.h>
#include <stm32f7xx_hal_conf.h>
#include <stm32f7xx_hal_rcc.h>
#include <stm32f7xx_hal_cortex.h>
#include "stm32f7xx_it.h"
	
#include <stddef.h> /* size_t */
#include "lib/sys.h"
#include "lib/ONE_hw.h"
#include "lib/led_pwm.h"
#include "lib/debug_hw.h"
#include "lib/debug_usart.h"
#include "lib/ak4556.h"
#include "lib/flash.h"

#define H_DELAY(n) do { register unsigned int i; \
		for (i = 0; i < n; ++i) \
			__asm__ __volatile__ ("nop\n\t":::"memory"); \
		} while (0);

inline void *memcpy(void *dest, const void *src, size_t n)
{
    char *dp = (char *)dest;
    const char *sp = (const char *)src;
    while( n-- ){
        *dp++ = *sp++;
    }
    return dest;
}

uint16_t State=0;

extern __IO int32_t inBuff[AUDIO_BUFF_SIZE];
extern __IO int32_t outBuff[AUDIO_BUFF_SIZE];

void init_audio_in( void )
{
	ak4556_Init(DSP_SAMPLE_RATE);
	//H_DELAY(100000);
	ak4556_Start();
	//H_DELAY(100000);
}

void Init( void )
{
	config_low_level();

	Debug_HW_Init();
	Debug_USART_Init();
	ONE_HW_Init();
	HAL_Delay(1);
}

void DeInit( void )
{
   	// Deinit open drivers
	Debug_HW_Deinit();
	Debug_USART_Deinit();
	ONE_HW_Deinit();
}

uint8_t is_boot( void )
{
	uint8_t state_mask = 0;
	int32_t dly = 0x20;
	uint32_t debounce = 0;

	while( dly-- ){
		ONE_getstates( &state_mask );
		debounce += (state_mask == 4);
		H_DELAY(1000);
	}
	return ((debounce > 0x10)
				? 1
				: 0 );
}

uint8_t is_exit( void )
{
	uint8_t state_mask = 0;
	int32_t dly = 0x20;
	uint32_t debounce = 0;

	while( dly-- ){
		ONE_getstates( &state_mask );
		debounce += (state_mask == 8);
		H_DELAY(1000);
	}
	return ((debounce > 0x10)
				? 1
				: 0 );
}

}

using namespace stmlib;
using namespace stm_audio_bootloader;

PacketDecoder decoder;
Demodulator demodulator;

enum UiState {
  UI_STATE_WAITING,
  UI_STATE_RECEIVING,
  UI_STATE_ERROR,
  UI_STATE_WRITING
};
volatile UiState ui_state;

void set_ui( UiState s )
{
    ui_state = s;
    led_boot_all( 0.0 );
    switch( s ){
        case UI_STATE_WAITING:
            led_boot_wait();
            break;
        case UI_STATE_RECEIVING:
            led_boot_rx();
            break;
        case UI_STATE_ERROR:
            led_boot_error();
            break;
        case UI_STATE_WRITING:
            led_boot_write();
            break;
    }
}

const int32_t in_threshold = 99999999; //8 nines = 1/3rd macbook
uint32_t discard_samples = 120000; //2.5s for DC pop on startup

void DSP_Block_Process( __IO int32_t* in_codec
	                  , __IO int32_t* out_codec
	                  ,      uint16_t b_size
	                  )
{
	static uint8_t last_sample = 0;
	uint8_t sample;
	int32_t scaled_in;
    float ui_level = 0.0;

	PWM_step( pwm_main ); // led-update

	while( b_size-- ){
		scaled_in = (int32_t)(*in_codec++ << 8);

		if( scaled_in < -in_threshold ){
			sample = 0;
            ui_level -= 1.0;
        } else if( scaled_in > in_threshold ){
			sample = 1;
            ui_level += 1.0;
		} else {
			sample = last_sample;
            ui_level = 0.0;
		}
		last_sample = sample;

		if( !discard_samples ) {
			demodulator.PushSample(sample);
		} else {
			--discard_samples;
		}
		*out_codec++ = 0;
	}
    led_boot_in( ui_level );
}

uint16_t packet_index;
uint16_t old_packet_index=0;

uint8_t g_error;

static uint32_t current_address;
const uint32_t kBlockSize = 16384;
const uint16_t kPacketsPerBlock = kBlockSize / kPacketSize;
uint8_t recv_buffer[kBlockSize];

void InitializeReception( void )
{
	//FSK
	decoder.Init();
	decoder.Reset();

	demodulator.Init(16, 8, 4);
	demodulator.Sync();

	current_address = kStartReceiveAddress;
	packet_index = 0;
	old_packet_index = 0;
    set_ui( UI_STATE_WAITING );
}

int main( void )
{
	uint32_t symbols_processed = 0;
	uint8_t i;

	Init();

// no key check, forces bootloader
	//if ( is_boot() ){
	    InitializeReception(); //FSK
		init_audio_in();
    //} else {
        //goto StartApp; // do not pass go. do not collect $200
    //}

    while (1) {
		g_error = 0;
		while( demodulator.available()
			&& !g_error ){

			uint8_t symbol = demodulator.NextSymbol();
			PacketDecoderState state = decoder.ProcessSymbol(symbol);
			symbols_processed++;

			switch( state ){
				case PACKET_DECODER_STATE_OK:
                    set_ui( UI_STATE_RECEIVING );
					memcpy( recv_buffer
                            + (packet_index % kPacketsPerBlock)
                              * kPacketSize
                          , decoder.packet_data()
                          , kPacketSize
                          );
					++packet_index;
					if( (packet_index % kPacketsPerBlock) == 0 ){
                        set_ui( UI_STATE_WRITING );
						ProgramPage( &current_address
								   , recv_buffer
								   , kBlockSize
								   );
						decoder.Reset();
						demodulator.Sync(); //FSK
					} else {
						decoder.Reset(); //FSK
					}
					break;

				case PACKET_DECODER_STATE_ERROR_SYNC:
					g_error = 1;
					break;

				case PACKET_DECODER_STATE_ERROR_CRC:
					g_error = 1;
					break;

				case PACKET_DECODER_STATE_END_OF_TRANSMISSION:
					//Copy from Receive buffer to Execution memory
					CopyMemory( kStartReceiveAddress
							  , kStartExecutionAddress
							  , current_address - kStartReceiveAddress
							  );
                    goto StartApp; // we're done! time to start the application

				default:
					break;
			}
		}
		if( g_error ){
            set_ui( UI_STATE_ERROR );

            while( !is_boot() ){ // wait until a key occurs
                if( is_exit() ){ // if it's 'UP'
                    goto StartApp; // exit bootloader -> goto application
                }
            }
            while( is_boot() ){;} // wait for release

            InitializeReception();
		}
        if( is_exit() ){
            goto StartApp;
        }
	}

StartApp:
    while( is_exit() ){;} // wait until nothing pressed
    DeInit();
	JumpTo(kStartExecutionAddress);
}
