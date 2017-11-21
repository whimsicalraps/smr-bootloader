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

// prototypes
static void SystemClock_Config(void);
static void Error_Handler(void);
static void MPU_Config(void);
static void CPU_CACHE_Enable(void);

// private fns
void config_low_level( void )
{
	MPU_Config();
	CPU_CACHE_Enable();
	HAL_Init();
	SystemClock_Config();
}

static void Error_Handler(void)
{
	while(1) {
		;;
	}
}

static void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;
	HAL_StatusTypeDef ret = HAL_OK;
	
	// Enable Power Control clock
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	// Enable HSE Oscillator and activate PLL with HSE as source
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 432;  // for 216MHz use 432 !!!!!!!!!!
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 9;
	RCC_OscInitStruct.PLL.PLLR = 7;  

	ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if(ret != HAL_OK) { Error_Handler(); }

	// Activate the OverDrive to reach the 216 MHz Frequency
	ret = HAL_PWREx_EnableOverDrive();
	if(ret != HAL_OK) { Error_Handler(); }

	// Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
	if(ret != HAL_OK) { Error_Handler(); }
}

static void MPU_Config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct;
	
	// Disable the MPU
	HAL_MPU_Disable();

	// Configure the MPU attributes as WT for SRAM
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress = 0x20020000;
	MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
	MPU_InitStruct.Number = MPU_REGION_NUMBER0;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	// Enable the MPU
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

static void CPU_CACHE_Enable(void)
{
	// Enable I-Cache
	SCB_EnableICache();

	// Enable D-Cache
	SCB_EnableDCache();
}

typedef void (*pFunc)(void);
void JumpTo(uint32_t address)
{
	// Deinit open drivers
	Debug_HW_Deinit();
	Debug_USART_Deinit();
	ONE_HW_Deinit();

	HAL_DeInit();

    // Reinitialize the Stack pointer
    __set_MSP(*(__IO uint32_t*) address);
    // jump to application address
    ((pFunc) (*(__IO uint32_t*) (address + 4)))();
    
    while(1){}
}

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

uint8_t check_boot( void )
{
	uint8_t state_mask = 0;
	int32_t dly = 0x20;
	uint32_t debounce = 0;

	while( dly-- ){
		ONE_getstates( &state_mask );
		debounce += (state_mask == 1);
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
	JumpTo(kStartExecutionAddress);
}