#include "sdio.h"

#include <stdlib.h>
#include "sd_ll.h"
#include "lib/debug_usart.h"
#include "lib/debug_hw.h"

#include "wrGlobals.h" // testing only (for max/min24b defines )

HAL_SD_CardInfoTypeDef CardInfo;

sd_rw_t sdcard; // storage for sd status

#define SD_OFFSET 0x100 // use to reduce wear on first segment
	// prob need something like this for production code too
#define SD_QUEUE_LENGTH         8

// private declarations
uint8_t _sd_dma_busy( void );
void sdio_make_osc(void);
void _sd_write( int32_t* audio, uint32_t sd_page );
uint8_t _sd_read_audio( uint32_t locn, uint32_t pages, int32_t* audio );
uint8_t _sd_write_audio( int32_t* audio, uint32_t locn, uint32_t pages );

sd_queue_t* sd_q_nextslot( void );
void sd_q_freehead( void );

// public definitions
void sdio_Init(void)
{
	while( BSP_SD_Init() != MSD_OK ){
		Debug_USART_printf("retry\n\r");
		HAL_Delay(2);
	}
	while( BSP_SD_IsDetected() != SD_PRESENT ){
		Debug_USART_printf("NoSD\r\n");
		HAL_Delay(2);
	}

	sdcard.w_status = 0;
	sdcard.w_pickle_buf = malloc( SDCARD_BLOCK_SIZE * 8 );
	if(sdcard.w_pickle_buf == NULL){ Debug_USART_printf("wp malloc\n\r"); }
	
	sdcard.r_status = 0;
	sdcard.r_dest   = NULL;
	sdcard.r_pages  = 1;
	sdcard.r_pickle_buf = malloc( SDCARD_BLOCK_SIZE * 8 );
	if(sdcard.r_pickle_buf == NULL){ Debug_USART_printf("rp malloc\n\r"); }

	// sdcard.wb_full  = 0;
	// sdcard.rb_full  = 0;

	sdcard.q_count = 0;
	sdcard.q_ix    = 0;
	sdcard.q_busy  = 0;
	sdcard.q_elems = malloc( sizeof(sd_queue_t) * SD_QUEUE_LENGTH );
	for( uint16_t i=0; i<SD_QUEUE_LENGTH; i++ ){
		sdcard.q_elems[i].type  = sdq_empty;
		sdcard.q_elems[i].audio = NULL;
		sdcard.q_elems[i].locn  = 0;
		sdcard.q_elems[i].pages = 0;
	}


    /*// generate sawtooth waveform
    HAL_Delay(100);
    Debug_USART_printf("gen\n\r");
    int32_t audio[168*8];
    for( uint16_t page=0; page<12; page++ ){
        int32_t saw = 0;
        for( uint16_t s=0; s<(168*8); s++ ){
            audio[s] = saw;
            saw += 100; // speed of sawtooth
            if( saw > (MAX24b >> 3) ){
                saw = (MIN24b >> 3);
            } // should produce 10v(?) p2p
        }
        sd_write_audio_now( audio, page+SD_OFFSET,  1 );
        while( sdcard.q_force ){ HAL_Delay(1); }
    }*/
}

void sdio_DeInit(void)
{
	BSP_SD_DeInit();
    // this leaves a lot of malloc'd data
    // but for now this deinit just needs to turn off drivers
    // the stack pointer gets reset momentarily anyway
}

uint8_t _sd_dma_busy( void )
{
	return sdcard.q_busy;
	/*if( sdcard.r_status == 1
	 || sdcard.w_status == 1 ){
		return 1; // wait for callback before new command
	} else {
		return 0;
	}*/
}

void sd_ll_error_callback(void)
{
	Debug_USART_printf("error\n\r");
	sdcard.r_status = 0;
	sdcard.w_status = 0;
}
void sd_ll_abort_callback(void)
{
	Debug_USART_printf("abort\n\r");
}

void sd_read_audio( uint32_t locn, uint32_t pages, int32_t* audio )
{
	sd_queue_t* q = sd_q_nextslot();

	q->type  = sdq_read;
	q->audio = audio;
	q->locn  = locn + SD_OFFSET;
	q->pages = pages;

/*	sdcard.rb_full  = 1;
	sdcard.rb_locn  = locn + SD_OFFSET;
	sdcard.rb_pages = pages;
	sdcard.rb_audio = audio;*/
}

void sd_write_audio( int32_t* audio, uint32_t locn, uint32_t pages )
{
	sd_queue_t* q = sd_q_nextslot();

	q->type  = sdq_write;
	q->audio = audio;
	q->locn  = locn + SD_OFFSET;
	q->pages = pages;
/*
	sdcard.wb_full  = 1;
	sdcard.wb_audio = audio;
	sdcard.wb_locn  = locn + SD_OFFSET;
	sdcard.wb_pages = pages;*/
}

void sd_read_audio_now( uint32_t locn, uint32_t pages, int32_t* audio )
{
    sdcard.q_force = 1;
	while( _sd_dma_busy() ){ ;; }
	if(_sd_read_audio( locn, pages, audio )){
		// Debug_USART_printf("retry\n\r");
		sd_read_audio_now( locn, pages, audio );
			// can lock up the cpu!
	}
}
uint8_t _sd_read_audio( uint32_t locn, uint32_t pages, int32_t* audio )
{
	int16_t timeout = 10;
	sdcard.r_dest = audio;
	sdcard.r_pages = pages;
	while(BSP_SD_GetCardState()){
		if(timeout <= 0) {
			sdcard.r_status = 0;
			return 1; // should return error flag!
		}
		timeout--;
	}
	if(sd_ll_read_audio( sdcard.r_pickle_buf
				       , locn
				       , pages ) ){
		Debug_USART_printf("rRetry\n\r");
		sdcard.r_status = 0;
		return 1; // error
	}
	// sdcard.rb_full = 0; // free buffer
	return 0; // success
}

void sd_ll_read_callback(void)
{
	// unpickle
	uint8_t* src = sdcard.r_pickle_buf;
	uint8_t* dst = (uint8_t*)sdcard.r_dest;

	for(uint16_t j=0; j<sdcard.r_pages; j++){ // double-page!
		for(uint16_t i=0;i<SDCARD_SAMPLES;i++) {
			*dst++; // skip lowest 8b (gets shifted out below)
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
			*sdcard.r_dest++ >>= 8;
		}
		src += 8; // last 8bytes on page are empty
					// soon to be cv & clock!!
	}
	// sdcard.r_status = 0; // read complete
	sd_q_freehead();
}

void sd_write_audio_now( int32_t* audio, uint32_t locn, uint32_t pages )
{
	//while( _sd_dma_busy() ){ ;; }
    sdcard.q_force = 1;
	while( BSP_SD_GetCardState()
        || sdcard.q_busy ){
        Debug_USART_printf("wait\n\r");
        HAL_Delay(10);
    }
    sdcard.q_busy = 1;
	if( _sd_write_audio( audio, locn, pages ) ){
		HAL_Delay(100); // lookout!
		volatile int16_t timeout = 0xFFF; // tune this to a realistic val
			// should probably a real timeout for failure
		while(timeout>0) { timeout--; }
        Debug_USART_printf("retrying w\n\r");
		sd_write_audio_now( audio, locn, pages );
	}
    Debug_USART_printf("write succeed\n\r");
}

uint8_t _sd_write_audio( int32_t* audio, uint32_t locn, uint32_t pages )
{
	int16_t timeout = 100;
	uint8_t* src = (uint8_t*)audio;
	uint8_t* buf2 = sdcard.w_pickle_buf;

	for(uint16_t j=0;j<pages;j++) {
		for(uint16_t i=0;i<SDCARD_SAMPLES;i++) { // pickle
			// no need to shift as 8xMSBs are sign is reflected in b24 (when clipped)
			*buf2++ = *src++; // LSB
			*buf2++ = *src++;
			*buf2++ = *src++; // 24bMSB w/ sign
				  	  *src++; // ignore MSB
		}
		buf2 += 8; // skip to next page
						// but not after adding cv & tempo recording
	}
	// DMA into SD card
	while(BSP_SD_GetCardState()){
		if(timeout <= 0) {
			sdcard.w_status = 0;
			return 1;
		}
		timeout--;
	}
	if(sd_ll_write_audio( sdcard.w_pickle_buf
		                , locn
		                , pages
		                )) {
		Debug_USART_printf("wRetry\n\r");
		sdcard.w_status = 0;
		return 1; // error
	}
	return 0; // success
}

void sd_ll_write_callback(void)
{
	// sdcard.w_status = 0;
	// sdcard.wb_full  = 0; // free buffer
    Debug_USART_printf("cb\n\r");
	sd_q_freehead();
}

// asynchronous r/w function
	// overflow means the old data is no longer valid
/*uint8_t sd_try_rw( void ){
	Debug_HW_set(led_y, 1);
	uint8_t retval = 0;

	if( _sd_dma_busy() ){
		retval = 7; // waiting for a callback
	} else if(sdcard.rb_full == 1){
		if( !BSP_SD_GetCardState() ){
			sdcard.rb_full = 2;
			sdcard.r_status = 1;
			_sd_read_audio( sdcard.rb_locn
				          , sdcard.rb_pages
				          , sdcard.rb_audio );
			retval = 3;
		} else { retval = 1; }
	} else if(sdcard.rb_full == 2){
		retval = 6;
	} else if(sdcard.wb_full == 1){
		if( !BSP_SD_GetCardState() ){
			sdcard.wb_full = 2;
			sdcard.w_status = 1;
			_sd_write_audio( sdcard.wb_audio
				           , sdcard.wb_locn
				           , sdcard.wb_pages );
			retval = 4;
		} else { retval = 2; }
	} else if(sdcard.wb_full == 2){
		retval = 5; // write is waiting for callback
	}
	Debug_HW_set(led_y, 0);
	return retval;
}*/

sd_queue_t* sd_q_nextslot( void )
{
	if( sdcard.q_count++ >= SD_QUEUE_LENGTH ){
		Debug_USART_printf( "too many sd\n\r");
		return &(sdcard.q_elems[sdcard.q_ix]); // won't blowup, but will trash data
	}
	int8_t ix = sdcard.q_count + sdcard.q_ix;
	if( ix >= SD_QUEUE_LENGTH ){ ix -= SD_QUEUE_LENGTH; }
	return &(sdcard.q_elems[ix]);
}

void sd_q_freehead( void )
{
    if( sdcard.q_force ){
        sdcard.q_busy = 0;
        sdcard.q_force = 0;
        return; // using a 'now' function
    }
	if( !sdcard.q_count ){
        Debug_USART_printf( "nothing in queue\n\r" );
        sdcard.q_busy = 0;
        return; // nothing in queue
    }
	sdcard.q_count--;
	sdcard.q_ix++; if( sdcard.q_ix >= SD_QUEUE_LENGTH ){ sdcard.q_ix = 0; }
    sdcard.q_busy = 0;
}

uint8_t sd_try_rw( void ){
    if( sdcard.q_force ){ return 3; } // 'now' fn is in progress
    if( !sdcard.q_count ){ return 0; } // nothing to do
    if( sdcard.q_busy ){  return 1; } // sdcard is busy
	if( !sdcard.q_busy
	 &&  sdcard.q_count ){
		if( !BSP_SD_GetCardState() ){ // only execute if !q_busy
			sd_queue_t* q = &(sdcard.q_elems[sdcard.q_ix]);
			if( q->type == sdq_read ){
                sdcard.q_busy = 1;
				_sd_read_audio( q->locn
					          , q->pages
					          , q->audio
					          );
                return 2;
			} else if( q->type == sdq_write ){
                sdcard.q_busy = 1;
				_sd_write_audio( q->audio
					           , q->locn
					           , q->pages
					           );
                return 2;
			}
		}
	}
	return 1; // should have called a r/w fn
}
