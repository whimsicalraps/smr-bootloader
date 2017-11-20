#pragma once

#include <stm32f7xx.h>

#define SDCARD_BLOCK_SIZE 		512
#define SDCARD_SAMPLES	 		168

typedef enum
  { sdq_empty
  , sdq_read
  , sdq_write
  } sd_access_t;

typedef struct {
  sd_access_t type;
  int32_t*    audio;
  uint32_t    locn;
  uint32_t    pages;
} sd_queue_t;

typedef struct {
  // DMA access details
  uint8_t   w_status;
  uint8_t*  w_pickle_buf;

  uint8_t   r_status;
  int32_t*  r_dest;
  uint32_t  r_pages;
  uint8_t*  r_pickle_buf;

  // sd access queue
  // separate read/write bufs?
  int8_t      q_count;
  int8_t      q_ix;
  uint8_t     q_busy;
  sd_queue_t* q_elems;
  uint8_t     q_force;

  // page access buffers (replace below with queue)
  /*uint8_t   wb_full;
  int32_t*  wb_audio;
  uint32_t  wb_locn;
  uint32_t  wb_pages;

  uint8_t   rb_full;
  uint32_t  rb_locn;
  uint32_t  rb_pages;
  int32_t*  rb_audio;*/
} sd_rw_t;

void sdio_Init(void);

uint8_t sd_busywait( void );
void sd_ll_error_callback(void);
void sd_ll_abort_callback(void);

void sd_read_audio_now( uint32_t locn, uint32_t pages, int32_t* audio );
void sd_read_audio( uint32_t locn, uint32_t pages, int32_t* audio );
void sd_ll_read_callback(void);

void sd_write_audio_now( int32_t* audio, uint32_t locn, uint32_t pages );
void sd_write_audio( int32_t* audio, uint32_t locn, uint32_t pages );
void sd_ll_write_callback(void);

uint8_t sd_try_rw( void );
