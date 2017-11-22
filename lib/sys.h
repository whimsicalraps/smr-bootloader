#pragma once

#include <stm32f7xx.h>
#include <stm32f7xx_hal.h>

void config_low_level( void );
void JumpTo( uint32_t address );
