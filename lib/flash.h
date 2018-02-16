#pragma once

#include <stm32f7xx.h>

const uint32_t kStartExecutionAddress = 0x08018000;
const uint32_t kStartReceiveAddress   = 0x08100000;
const uint32_t EndOfMemory            = 0x081FFFFC; // allows 1MB program size

// 2MB flash organization
const uint32_t kSectorBaseAddress[] =
	{ 0x08000000 // 32k
	, 0x08008000 // 32k
	, 0x08010000 // 32k
	, 0x08018000 // 32k
	, 0x08020000 // 128k
	, 0x08040000 // 256k
	, 0x08080000 // 256k
	, 0x080C0000 // 256k
	, 0x08100000 // 256k
	, 0x08140000 // 256k
	, 0x08180000 // 256k
	, 0x081C0000 // 256k
	};

const uint32_t sector_count = 12;

uint8_t CopyMemory( uint32_t src_addr
                  , uint32_t dst_addr
                  , size_t size
                  );
uint8_t ProgramPage( uint32_t*      current_address
				   , const uint8_t* data
				   , size_t         size
				   );
