#pragma once

#include <stm32f7xx.h>

#define HALF32B	           0x3FFFFFFF
#define MAX32B	           0x7FFFFFFF
#define MAX32f	           ((float)0x7FFFFFFF)
#define iMAX32B	           (1/ (float)MAX32B)
#define IO_SCALE           ((float)0.725)
#define iMAX32B_NORMALIZED (iMAX32B * IO_SCALE)

void DSP_Block_Init( uint16_t sample_rate, uint16_t b_size );
void DSP_Block_Process( __IO int32_t* in_codec, __IO int32_t* out_codec, uint16_t b_size );