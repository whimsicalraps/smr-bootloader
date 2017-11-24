#pragma once

#include <stdint.h>

#define BLOCK_SIZE 24
#define MAX32B	           0x7FFFFFFF

void init_music_maker( void );

int32_t* make_music( int32_t* output
                   , float    samp_direction
                   );
