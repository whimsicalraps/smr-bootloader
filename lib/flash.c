#include "flash.h"
#include "debug_usart.h"

uint8_t CopyMemory(uint32_t src_addr, uint32_t dst_addr, size_t size)
{
	HAL_FLASH_Unlock();

	for( size_t written = 0; written < size; written += 4 ){

		// erase sector if we just reached one
		for( int32_t i=0; i < sector_count; ++i ){
			if( dst_addr == kSectorBaseAddress[i] ){
				uint32_t sector_error;
				FLASH_EraseInitTypeDef erase_setup =
					{ .TypeErase    = FLASH_TYPEERASE_SECTORS
					, .Sector       = i
					, .NbSectors    = 1
					, .VoltageRange = FLASH_VOLTAGE_RANGE_3
					};
				HAL_FLASHEx_Erase( &erase_setup
								 , &sector_error
								 );
			}
		}

		if( dst_addr > (kStartReceiveAddress - 4) ){
			return 1; // trying to overwrite receive buffer
		}

		//Program the word
		HAL_FLASH_Program( FLASH_TYPEPROGRAM_WORD
						 , dst_addr
						 , *(uint32_t*)src_addr
						 );
		src_addr += 4;
		dst_addr += 4;
	}
	return 0;
}


uint8_t ProgramPage( uint32_t*      current_address
				   , const uint8_t* data
				   , size_t         size
				   )
{
	HAL_FLASH_Unlock();

	for( int32_t i=0; i < sector_count; ++i ){
		if( *current_address == kSectorBaseAddress[i] ){
			uint32_t sector_error;
	        FLASH_WaitForLastOperation( 10000 );
			FLASH_EraseInitTypeDef erase_setup =
				{ .TypeErase    = FLASH_TYPEERASE_SECTORS
				, .Sector       = i
				, .NbSectors    = 1
				, .VoltageRange = FLASH_VOLTAGE_RANGE_3
				};
			HAL_FLASHEx_Erase( &erase_setup
							 , &sector_error
							 );
			// check sector_error value for erase error reporting
		}
	}

	uint32_t* words = (uint32_t*)data;
	for( size_t written = 0; written < size; written += 4 ){
	    FLASH_WaitForLastOperation( 10000 );
		HAL_FLASH_Program( FLASH_TYPEPROGRAM_WORD
						 , *current_address
						 , *words++
						 );
		*current_address += 4;
		if( *current_address >= EndOfMemory ){
			return 1; // end of memory
		}
	}
	return 0; // no error
}
