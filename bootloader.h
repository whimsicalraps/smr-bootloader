#pragma once

#include <stm32f7xx.h>

void DSP_Block_Process( __IO uint32_t* in_codec
	                  , __IO uint32_t* out_codec
	                  ,      uint16_t  b_size
	                  );
