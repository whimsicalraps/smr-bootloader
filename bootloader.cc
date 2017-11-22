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
uint16_t manual_exit_primed;
uint8_t exit_updater;

extern __IO int32_t inBuff[AUDIO_BUFF_SIZE];
extern __IO int32_t outBuff[AUDIO_BUFF_SIZE];

void init_audio_in( void )
{
	ak4556_Init(DSP_SAMPLE_RATE);
	H_DELAY(100000);
	ak4556_Start();
	H_DELAY(100000);
}

void Init( void )
{
	config_low_level();

	Debug_HW_Init();
	Debug_USART_Init();
	ONE_HW_Init();
	HAL_Delay(1);

	PWM_set_all( 0.4, pwm_main );
}

void DeInit( void )
{
   	// Deinit open drivers
	Debug_HW_Deinit();
	Debug_USART_Deinit();
	ONE_HW_Deinit();
}

uint8_t check_boot( void )
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
				? 0
				: 1 );
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

const uint32_t in_threshold = 0xFFF; // 4096
uint16_t discard_samples = 8000;

void DSP_Block_Process( __IO int32_t* in_codec
	                  , __IO int32_t* out_codec
	                  ,      uint16_t b_size
	                  )
{
	uint8_t sample;
	static uint8_t last_sample = 0;
	int32_t scaled_in;

	PWM_step( pwm_main ); // led-update

	while( b_size-- ){
		scaled_in = -(int32_t)(*in_codec << 8);

		if( last_sample == 1 ){
			if( scaled_in < -in_threshold ){
				sample = 0;
			} else {
				sample = 1;
			}
		} else {
			if( scaled_in > in_threshold ){
				sample = 1;
			} else {
				sample = 0;
			}
		}
		last_sample = sample;

		if( !discard_samples ) {
			demodulator.PushSample(sample);
		} else {
			--discard_samples;
		}

		if( ui_state == UI_STATE_ERROR ){
			*out_codec++ = 0;
		} else {
			*out_codec++ = *in_codec++; // echo in to output
			// instead we could play a little melody?!?!?!
		}
	}
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
	ui_state = UI_STATE_WAITING;
}

int main( void )
{
	uint32_t symbols_processed = 0;
	uint8_t i;

	Init();
	InitializeReception(); //FSK

	if ( !check_boot() ){
		init_audio_in();
	}
	exit_updater = check_boot();

	manual_exit_primed=0;
	while (!exit_updater) {
		g_error = 0;
		while( demodulator.available()
			&& !g_error
			&& !exit_updater ){

			uint8_t symbol = demodulator.NextSymbol();
			PacketDecoderState state = decoder.ProcessSymbol(symbol);
			symbols_processed++;

			switch( state ){
				case PACKET_DECODER_STATE_OK:
					ui_state = UI_STATE_RECEIVING;
					memcpy(recv_buffer + (packet_index % kPacketsPerBlock) * kPacketSize, decoder.packet_data(), kPacketSize);
					++packet_index;
					if( (packet_index % kPacketsPerBlock) == 0 ){
						ui_state = UI_STATE_WRITING;
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
					exit_updater = 1;

					//Copy from Receive buffer to Execution memory
					CopyMemory( kStartReceiveAddress
							  , kStartExecutionAddress
							  , current_address - kStartReceiveAddress
							  );
					break;

				default:
					break;
			}
		}
		if( g_error ){ // allow easy restarting
			ui_state = UI_STATE_ERROR;
			// while (check_speed()){;}
			// while (check_speed()){;}
			// startup animation
			InitializeReception();
			manual_exit_primed=0;
			exit_updater=0;
		}
	}
    DeInit();
	JumpTo(kStartExecutionAddress);
}
