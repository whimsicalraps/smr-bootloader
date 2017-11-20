#pragma once

#include "wrFilter.h"
#include "chalk.h"

// could likely redefine BLOCK SIZE to 4096 to reflect 8 pages
#define SDCARD_PAGE_SIZE   512
#define REEL_BLOCK_PAGES   8
#define REEL_PAGE_SAMPLES  168
	// 168 * pages
#define REEL_BLOCK_SAMPLES (REEL_PAGE_SAMPLES * REEL_BLOCK_PAGES)
#define PAGES_PER_TAPE     64   // more like 100million on SD

#define TR_MAX_SPEED  ((float)2.0)

typedef struct page_buffer {
	int32_t  audio[REEL_BLOCK_SAMPLES]; // playback buffer
	int32_t  write[REEL_BLOCK_SAMPLES]; // write buffer
	uint32_t cue_ix;                    // sd-card index
	uint8_t  dirty;                     // data changed
} page_buffer_t;

typedef enum page_in_buf
    { a
    , b
    , c
    , d
// write a custom SD card read driver to pull this as 3 sets of 2(*8) pages
  // might need a tmp buffer as the 2 destination pages are split
  // point is to pull the SD card as bigger blocks to reduce operation count
    // these are used ping-pong for next/prev
    , BUF_A0
    , BUF_A1
    , BUF_B0
    , BUF_B1
    , BUF_NNEXT
    , BUF_NNNEXT

    , page_count
    } page_in_buf_t;

typedef enum p_access_list
    { p_skrt
    , p_prev
    , p_here
    , p_next

    , p_before0
    , p_before1
    , p_after0
    , p_after1

    , p_nnext  // likely don't need these - use an abcd instead
    , p_nnnext

    , p_access_count
    } p_access_list_t;

typedef enum
    { std
    , loop1
    , loop2
    , loop3
    , next
    } page_motion_t;

// reel internally handles backup / error checking of chalk list
typedef struct reel {
	uint32_t       pos_page;    // sample page of head
	int32_t        goto_page;   // sample page to jump to next (-1 is inactive)
	float          pos_samp;    // sub-sample pos within page
	int8_t         page_advance;// check this flag after audio to advance tape
	chalk_t        chalks;      // RAM buffer of sd-card stored chalk markers
	page_buffer_t  page[page_count];     // buffer sd-card pages in RAM
	page_buffer_t* p_access[p_access_count]; // exchange pointers to keep order
    page_motion_t  p_motion;    // stores next page buffer action
    int8_t         p_queue;

	// fnptr: tapehead action
	float* (*reel_rw)( struct reel* self
		             , float*       headbuf
		             );

	// array of pointers for buffer access
	int32_t**      samp_access; // pointers to pointers!
	uint16_t       hb_count;    // how many array ix's actually touched
    uint16_t       hb_origin;   // which sample is '0' point
	float          nxt_pos_samp;// (f)samps motion this block
} reel_t;

// same as page_in_buf above??
// this is about what to pull into RAM
// above is named accessors for samps already in RAM
typedef enum new_page
	{ P_PREV = -1
	, P_NONE = 0
	, P_NEXT
	, P_NEXT_CHALK
	, P_PREV_CHALK
	, P_LOOP
	} new_page_t;

typedef struct transport {
	float        speed;      // actual speed
	float        speed_dest; // destination of 1p filter
	uint8_t      motor_active; // is the motor pulling tape?
	float        nudge;      // how much are we currently nudging?
	float        nudge_accum;
	filter_lp1_t speed_slew; // smoothing for speed changes
	filter_lp1_t speed_manual; // smoothing for manual changes

    uint16_t     b_size;     // blocks per processing frame
    float*       ix;         // list of sample indices within page

    float*       speed_v;    // array of speeds per sample
} transport_t;

typedef enum tape_mode
	{ READONLY
	, OVERDUB
	, OVERWRITE
	, LOOPER       // overwrite, but returns old tape content (external fb)
	, tape_mode_count
	} tape_mode_t;

typedef struct tapedeck {
	uint16_t     b_size; // r/w sample count
	transport_t  transport;
	tape_mode_t  rec_mode;
	reel_t       reel;
} tapedeck_t;
