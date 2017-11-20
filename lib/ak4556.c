#include "ak4556.h"
#include "dsp_block.h"
#include "debug_hw.h"
#include "debug_usart.h"
#include <stm32f7xx_hal.h>

// private structs & vars
SAI_HandleTypeDef       SaiHandle;
SAI_HandleTypeDef       SaiHandle2;
DMA_HandleTypeDef		hSaiDma;
DMA_HandleTypeDef		hSaiDma2;
// AUDIO_DrvTypeDef		*audio_drv;

// audio buffers for codec DMA
// prefer to malloc these in the Init fn?
__IO int32_t            inBuff[AUDIO_BUFF_SIZE];
__IO int32_t            outBuff[AUDIO_BUFF_SIZE];


// exported fns
void ak4556_Init( uint32_t s_rate )
{
	RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct;
	// Configure PLLSAI prescalers
		// PLLSAI_VCO: VCO_429M
		// SAI_CLK(first level) = PLLSAI_VCO/PLLSAIQ = 429/2 = 214.5 Mhz
		// SAI_CLK_x = SAI_CLK(first level)/PLLSAIDIVQ = 214.5/19 = 11.289 Mhz
// nb: this uses SAI2 pll clock?!
	RCC_PeriphCLKInitStruct.PeriphClockSelection 	= RCC_PERIPHCLK_SAI1;
	RCC_PeriphCLKInitStruct.Sai1ClockSelection 		= RCC_SAI1CLKSOURCE_PLLSAI;
												   // 48kHz;  44k  24k
	RCC_PeriphCLKInitStruct.PLLSAI.PLLSAIN 			= 344; // 429  344
	RCC_PeriphCLKInitStruct.PLLSAI.PLLSAIQ 			= 7;   // 2	   7
	RCC_PeriphCLKInitStruct.PLLSAIDivQ 				= 1;   // 19   2
	HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);

	// Initialize SAI
	__HAL_SAI_RESET_HANDLE_STATE(&SaiHandle);
	__HAL_SAI_RESET_HANDLE_STATE(&SaiHandle2);

	// DAC
	SaiHandle.Instance = AUDIO_SAI_A;
	__HAL_SAI_DISABLE(&SaiHandle);
	SaiHandle.Init.AudioMode		= SAI_MODEMASTER_TX;
	SaiHandle.Init.Synchro			= SAI_ASYNCHRONOUS;
	SaiHandle.Init.SynchroExt       = SAI_SYNCEXT_OUTBLOCKA_ENABLE;
	SaiHandle.Init.OutputDrive		= SAI_OUTPUTDRIVE_ENABLE;
	SaiHandle.Init.NoDivider		= SAI_MASTERDIVIDER_ENABLE;
	SaiHandle.Init.FIFOThreshold	= SAI_FIFOTHRESHOLD_1QF; //HF instead of 1QF?
	SaiHandle.Init.AudioFrequency	= s_rate;
	SaiHandle.Init.Protocol			= SAI_FREE_PROTOCOL;
	SaiHandle.Init.DataSize			= SAI_DATASIZE_24;
	SaiHandle.Init.FirstBit			= SAI_FIRSTBIT_MSB;
	SaiHandle.Init.ClockStrobing	= SAI_CLOCKSTROBING_RISINGEDGE; // unsure?

	SaiHandle.FrameInit.FrameLength			= 64;
	SaiHandle.FrameInit.ActiveFrameLength	= 32;
	SaiHandle.FrameInit.FSDefinition		= SAI_FS_CHANNEL_IDENTIFICATION;
	SaiHandle.FrameInit.FSPolarity			= SAI_FS_ACTIVE_LOW;
	SaiHandle.FrameInit.FSOffset			= SAI_FS_BEFOREFIRSTBIT;

	SaiHandle.SlotInit.FirstBitOffset	= 0;
	SaiHandle.SlotInit.SlotSize			= SAI_SLOTSIZE_DATASIZE;
	SaiHandle.SlotInit.SlotNumber		= 2;
	SaiHandle.SlotInit.SlotActive		= SAI_SLOTACTIVE_0;

	if( HAL_OK != HAL_SAI_Init(&SaiHandle) ){
		Debug_USART_printf("SAI failed init");
	}

	// ADC
	SaiHandle2.Instance = AUDIO_SAI_B;
	__HAL_SAI_DISABLE(&SaiHandle2);
	SaiHandle2.Init.AudioMode		= SAI_MODESLAVE_RX;
	SaiHandle2.Init.Synchro			= SAI_SYNCHRONOUS; // lock to other block
	SaiHandle2.Init.SynchroExt      = SAI_SYNCEXT_OUTBLOCKA_ENABLE;
	SaiHandle2.Init.OutputDrive		= SAI_OUTPUTDRIVE_DISABLE;
	SaiHandle2.Init.NoDivider		= SAI_MASTERDIVIDER_ENABLE;
	SaiHandle2.Init.FIFOThreshold	= SAI_FIFOTHRESHOLD_1QF;
	SaiHandle2.Init.AudioFrequency	= s_rate;
	SaiHandle2.Init.Protocol		= SAI_FREE_PROTOCOL;
	SaiHandle2.Init.DataSize		= SAI_DATASIZE_24;
	SaiHandle2.Init.FirstBit		= SAI_FIRSTBIT_MSB;
	SaiHandle2.Init.ClockStrobing	= SAI_CLOCKSTROBING_RISINGEDGE; // unsure?

	SaiHandle2.FrameInit.FrameLength		= 64;
	SaiHandle2.FrameInit.ActiveFrameLength	= 32;
	SaiHandle2.FrameInit.FSDefinition		= SAI_FS_CHANNEL_IDENTIFICATION;
	SaiHandle2.FrameInit.FSPolarity			= SAI_FS_ACTIVE_LOW;
	SaiHandle2.FrameInit.FSOffset			= SAI_FS_BEFOREFIRSTBIT;

	SaiHandle2.SlotInit.FirstBitOffset	= 0;
	SaiHandle2.SlotInit.SlotSize		= SAI_SLOTSIZE_DATASIZE;
	SaiHandle2.SlotInit.SlotNumber		= 2;
	SaiHandle2.SlotInit.SlotActive		= SAI_SLOTACTIVE_0;

	if( HAL_OK != HAL_SAI_Init(&SaiHandle2) ){
		Debug_USART_printf("SAI failed init");
	}

	// Enable SAI to generate clock used by audio driver
	__HAL_SAI_ENABLE(&SaiHandle2); // adc before dac
	__HAL_SAI_ENABLE(&SaiHandle);

	// Reset codec
	HAL_GPIO_WritePin(AUDIO_SAI_RESET_GPIO_PORT, AUDIO_SAI_RESET_PIN, GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(AUDIO_SAI_RESET_GPIO_PORT, AUDIO_SAI_RESET_PIN, GPIO_PIN_SET);
}

void ak4556_Start( void )
{
	// Zero the output buffer
	for( uint16_t i=0; i<AUDIO_BUFF_SIZE; i++ ){
		inBuff[i]  = 0;
		outBuff[i] = 0x77777777;
	}

	// Enable DAC output
	if( HAL_OK != HAL_SAI_Transmit_DMA(&SaiHandle, (uint8_t *)outBuff, AUDIO_BUFF_SIZE) ){
		Debug_USART_printf("failed to transmit sai dma");
	}
	// Enable ADC audio input
	if( HAL_OK != HAL_SAI_Receive_DMA(&SaiHandle2, (uint8_t *)inBuff, AUDIO_BUFF_SIZE) ){
		Debug_USART_printf("failed to receive sai dma");
	}
}

void HAL_SAI_MspInit(SAI_HandleTypeDef *hsai)
{
	GPIO_InitTypeDef  GPIO_Init;
	GPIO_Init.Mode	= GPIO_MODE_AF_PP;
	GPIO_Init.Pull	= GPIO_PULLUP;
	GPIO_Init.Speed	= GPIO_SPEED_FREQ_VERY_HIGH;

	__HAL_RCC_DMA2_CLK_ENABLE();

	if(hsai == &SaiHandle){
		AUDIO_SAI_A_CLK_ENABLE(); // RCC

		AUDIO_SAI_A_MCLK_ENABLE();
		AUDIO_SAI_A_SCK_ENABLE();
		AUDIO_SAI_A_FS_ENABLE();
		AUDIO_SAI_A_SD_ENABLE();

		GPIO_Init.Alternate	    = AUDIO_SAI_A_FS_AF;
		GPIO_Init.Pin           = AUDIO_SAI_A_FS_PIN;
		HAL_GPIO_Init(AUDIO_SAI_A_FS_GPIO_PORT, &GPIO_Init);
		GPIO_Init.Alternate	    = AUDIO_SAI_A_SCK_AF;
		GPIO_Init.Pin           = AUDIO_SAI_A_SCK_PIN;
		HAL_GPIO_Init(AUDIO_SAI_A_SCK_GPIO_PORT, &GPIO_Init);
		GPIO_Init.Alternate	    = AUDIO_SAI_A_SD_AF;
		GPIO_Init.Pin           = AUDIO_SAI_A_SD_PIN;
		HAL_GPIO_Init(AUDIO_SAI_A_SD_GPIO_PORT, &GPIO_Init);
		GPIO_Init.Alternate	    = AUDIO_SAI_A_MCLK_AF;
		GPIO_Init.Pin           = AUDIO_SAI_A_MCLK_PIN;
		HAL_GPIO_Init(AUDIO_SAI_A_MCLK_GPIO_PORT, &GPIO_Init);

		AUDIO_SAI_RESET_ENABLE();
		GPIO_Init.Mode          = GPIO_MODE_OUTPUT_PP;
		GPIO_Init.Pull          = GPIO_PULLUP;
		GPIO_Init.Pin           = AUDIO_SAI_RESET_PIN;
		HAL_GPIO_Init(AUDIO_SAI_RESET_GPIO_PORT, &GPIO_Init);

		// Configure DMA used for SAI_A
		// st3, ch0
		hSaiDma.Init.Channel				= DMA_CHANNEL_0;
		hSaiDma.Init.Direction				= DMA_MEMORY_TO_PERIPH;
		hSaiDma.Init.PeriphInc				= DMA_PINC_DISABLE;
		hSaiDma.Init.MemInc					= DMA_MINC_ENABLE;
		hSaiDma.Init.PeriphDataAlignment	= DMA_PDATAALIGN_WORD;
		hSaiDma.Init.MemDataAlignment		= DMA_MDATAALIGN_WORD;
		hSaiDma.Init.Mode					= DMA_CIRCULAR;
		hSaiDma.Init.Priority				= DMA_PRIORITY_HIGH;
		hSaiDma.Init.FIFOMode				= DMA_FIFOMODE_DISABLE;
		hSaiDma.Init.FIFOThreshold			= DMA_FIFO_THRESHOLD_FULL;
		hSaiDma.Init.MemBurst				= DMA_MBURST_SINGLE;
		hSaiDma.Init.PeriphBurst			= DMA_PBURST_SINGLE;

		// Select the DMA instance to be used for the transfer : DMA2_Stream6
		hSaiDma.Instance                    = DMA2_Stream1;

		// Associate the DMA handle
		__HAL_LINKDMA(hsai, hdmatx, hSaiDma);

		// Deinitialize the Stream for new transfer
		HAL_DMA_DeInit(&hSaiDma);

		// Configure the DMA Stream
		if( HAL_OK != HAL_DMA_Init(&hSaiDma) ){
			Debug_USART_printf("dma1 failed to init");
		}

		// HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0x02, 0);
		// HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
		
	} else { // BLOCK B
		AUDIO_SAI_B_CLK_ENABLE(); // RCC
		
		AUDIO_SAI_B_SD_ENABLE();
		
		GPIO_Init.Alternate	    = AUDIO_SAI_B_SD_AF;
		GPIO_Init.Pin           = AUDIO_SAI_B_SD_PIN;
		HAL_GPIO_Init(AUDIO_SAI_B_SD_GPIO_PORT, &GPIO_Init);
		
		// Configure DMA used for SAI_B
		// st5, ch0
		hSaiDma2.Init.Channel				= DMA_CHANNEL_0;
		hSaiDma2.Init.Direction				= DMA_PERIPH_TO_MEMORY;
		hSaiDma2.Init.PeriphInc				= DMA_PINC_DISABLE;
		hSaiDma2.Init.MemInc				= DMA_MINC_ENABLE;
		hSaiDma2.Init.PeriphDataAlignment	= DMA_PDATAALIGN_WORD;
		hSaiDma2.Init.MemDataAlignment		= DMA_MDATAALIGN_WORD;
		hSaiDma2.Init.Mode					= DMA_CIRCULAR;
		hSaiDma2.Init.Priority				= DMA_PRIORITY_HIGH;
		hSaiDma2.Init.FIFOMode				= DMA_FIFOMODE_DISABLE;
		hSaiDma2.Init.FIFOThreshold			= DMA_FIFO_THRESHOLD_FULL;
		hSaiDma2.Init.MemBurst				= DMA_MBURST_SINGLE;
		hSaiDma2.Init.PeriphBurst			= DMA_PBURST_SINGLE;

		// Select the DMA instance to be used for the transfer : DMA2_Stream6
		hSaiDma2.Instance                    = DMA2_Stream5;

		// Associate the DMA handle
		__HAL_LINKDMA(hsai, hdmarx, hSaiDma2);

		// Deinitialize the Stream for new transfer
		HAL_DMA_DeInit(&hSaiDma2);

		// Configure the DMA Stream
		if( HAL_OK != HAL_DMA_Init(&hSaiDma2) ){
			Debug_USART_printf("dma2 failed to init");
		}

		// Codec request triggers transfer & new frame calc
		HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 0x02, 0);
		HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
	}
}

// DMA triggered by codec requesting more ADC!
void DMA2_Stream5_IRQHandler(void)
{
	// essentially triggers below callbacks
	HAL_DMA_IRQHandler(SaiHandle2.hdmarx);
}

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
	// Debug_USART_printf("txh\n\r");
	Debug_HW_set(bm_1, 1);
	Debug_HW_set(led_x, 1);
	DSP_Block_Process(&inBuff[0], &outBuff[0], DSP_BLOCK_SIZE);
	Debug_HW_set(led_x, 0);
	Debug_HW_set(bm_1, 0);
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
	// Debug_USART_printf("txc\n\r");
	Debug_HW_set(bm_1, 1);
	Debug_HW_set(led_x, 1);
	DSP_Block_Process(&inBuff[DSP_BLOCK_SIZE], &outBuff[DSP_BLOCK_SIZE], DSP_BLOCK_SIZE);
	Debug_HW_set(led_x, 0);
	Debug_HW_set(bm_1, 0);
}
