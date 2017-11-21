#include "flash.h"

uint8_t CopyMemory(uint32_t src_addr, uint32_t dst_addr, size_t size)
{
	HAL_FLASH_Unlock();

	for( size_t written = 0; written < size; written += 8 ){

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

		if( dst_addr > (kStartReceiveAddress - 8) ){
			return 1; // trying to overwrite receive buffer
		}

		//Program the word
		// FLASH_ProgramWord(dst_addr, *(uint32_t*)src_addr);
		HAL_FLASH_Program( FLASH_TYPEPROGRAM_DOUBLEWORD
						 , dst_addr
						 , *(uint64_t*)src_addr
						 );
		src_addr += 8;
		dst_addr += 8;
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

	uint64_t* words = (uint64_t*)data;
	for( size_t written = 0; written < size; written += 8 ){
		HAL_FLASH_Program( FLASH_TYPEPROGRAM_DOUBLEWORD
						 , *current_address
						 , *words++
						 );
		*current_address += 8;
		if( *current_address >= EndOfMemory ){
			return 1; // end of memory
		}
	}
	// FLASH_WaitForLastOperation(uint32_t Timeout);
	return 0; // no error
}