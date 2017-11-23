#include "sd_ll.h"


#include "debug_usart.h"

SD_HandleTypeDef uSdHandle;

uint8_t BSP_SD_Init(void)
{
	uint8_t sd_state = MSD_OK;

	// uSD device interface configuration
	uSdHandle.Instance = thisMMC;
	uSdHandle.Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
	uSdHandle.Init.ClockBypass         = SDMMC_CLOCK_BYPASS_DISABLE;
	uSdHandle.Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	uSdHandle.Init.BusWide             = SDMMC_BUS_WIDE_1B;
	uSdHandle.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
	uSdHandle.Init.ClockDiv            = SDMMC_TRANSFER_CLK_DIV;

	// Msp SD Detect pin initialization
	BSP_SD_Detect_MspInit(&uSdHandle, NULL);
	
	// Check if SD card is present
	if(BSP_SD_IsDetected() != SD_PRESENT){
		return MSD_ERROR_SD_NOT_PRESENT;
	}

	// Msp SD initialization
	BSP_SD_MspInit(&uSdHandle, NULL);

	// HAL SD initialization
	if(HAL_SD_Init(&uSdHandle) != HAL_OK) {
		sd_state = MSD_ERROR;
	}

	// Configure SD Bus width
	if(sd_state == MSD_OK) {
		// Enable wide operation
		if(HAL_SD_ConfigWideBusOperation(&uSdHandle, SDMMC_BUS_WIDE_4B) != HAL_OK){
			sd_state = MSD_ERROR;
		} else {
			sd_state = MSD_OK;
		}
	}
	return  sd_state;
}

uint8_t BSP_SD_DeInit(void)
{
	uint8_t sd_state = MSD_OK;

	uSdHandle.Instance = thisMMC;

	// HAL SD deinitialization
	if(HAL_SD_DeInit(&uSdHandle) != HAL_OK) { sd_state = MSD_ERROR; }

	// Msp SD deinitialization
	uSdHandle.Instance = thisMMC;
	BSP_SD_MspDeInit(&uSdHandle, NULL);

	return  sd_state;
}

uint8_t BSP_SD_ITConfig(void)
{
	GPIO_InitTypeDef gpio_init_structure;

	// Configure Interrupt mode for SD detection pin
	gpio_init_structure.Pin = SD_DETECT_PIN;
	gpio_init_structure.Pull = GPIO_PULLUP;
	gpio_init_structure.Speed = GPIO_SPEED_FAST;
	gpio_init_structure.Mode = GPIO_MODE_IT_RISING_FALLING;
	HAL_GPIO_Init(SD_DETECT_GPIO_PORT, &gpio_init_structure);

	// Enable and set SD detect EXTI Interrupt to the lowest priority
	// HAL_NVIC_SetPriority((IRQn_Type)(SD_DETECT_EXTI_IRQn), 0x0F, 0x00);
	// HAL_NVIC_EnableIRQ((IRQn_Type)(SD_DETECT_EXTI_IRQn));

	return MSD_OK;
}

uint8_t BSP_SD_IsDetected(void)
{
	__IO uint8_t status = SD_PRESENT;

	// Check SD card detect pin
	if (HAL_GPIO_ReadPin(SDIO_DETECT_GPIO, SDIO_DETECT_PIN) == GPIO_PIN_SET){
		status = SD_NOT_PRESENT;
	}
	return status;
}

uint8_t BSP_SD_ReadBlocks(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks, uint32_t Timeout)
{
	if(HAL_SD_ReadBlocks(&uSdHandle, (uint8_t *)pData, ReadAddr, NumOfBlocks, Timeout) != HAL_OK) {
		return MSD_ERROR;
	} else {
		return MSD_OK;
	}
}

uint8_t BSP_SD_WriteBlocks(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks, uint32_t Timeout)
{
	if(HAL_SD_WriteBlocks(&uSdHandle, (uint8_t *)pData, WriteAddr, NumOfBlocks, Timeout) != HAL_OK) {
		return MSD_ERROR;
	} else {
		return MSD_OK;
	}
}

uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks)
{  
	if(HAL_SD_ReadBlocks_DMA(&uSdHandle, (uint8_t *)pData, ReadAddr, NumOfBlocks) != HAL_OK){
		return MSD_ERROR;
	} else {
		return MSD_OK;
	}
}

uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks)
{
	if(HAL_SD_WriteBlocks_DMA(&uSdHandle, (uint8_t *)pData, WriteAddr, NumOfBlocks) != HAL_OK){
		return MSD_ERROR;
	} else {
		return MSD_OK;
	}
}

uint8_t BSP_SD_Erase(uint32_t StartAddr, uint32_t EndAddr)
{
	if(HAL_SD_Erase(&uSdHandle, StartAddr, EndAddr) != HAL_OK){
		return MSD_ERROR;
	} else {
		return MSD_OK;
	}
}

__weak void BSP_SD_MspInit(SD_HandleTypeDef *hsd, void *Params)
{
	static DMA_HandleTypeDef dma_rx_handle;
	static DMA_HandleTypeDef dma_tx_handle;
	GPIO_InitTypeDef gpio_init_structure;

	SDIO_P_CLK();

	/* Enable DMA2 clocks */
	__DMAx_TxRx_CLK_ENABLE();

	/* Enable GPIOs clock */
	SDIO_MAIN_GPIO_RCC();
	SDIO_CMD_GPIO_RCC();
	
	/* Common GPIO configuration */
	gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
	gpio_init_structure.Pull      = GPIO_PULLUP;
	gpio_init_structure.Speed     = GPIO_SPEED_HIGH;

	/* GPIOC configuration */
	gpio_init_structure.Alternate = SDIO_AF;
	gpio_init_structure.Pin = SDIO_D0_PIN | SDIO_D1_PIN | SDIO_D2_PIN | SDIO_D3_PIN | SDIO_CLK_PIN;
	HAL_GPIO_Init(SDIO_MAIN_GPIO, &gpio_init_structure);

	/* GPIOD configuration */
	gpio_init_structure.Alternate = SDIO_AF;
	gpio_init_structure.Pin = SDIO_CMD_PIN;
	HAL_GPIO_Init(SDIO_CMD_GPIO, &gpio_init_structure);
	
	/* NVIC configuration for thisMMC interrupts */
	HAL_NVIC_SetPriority(SDMMC1_IRQn, 0x02, 0);
	HAL_NVIC_EnableIRQ(SDMMC1_IRQn);

	/* Configure DMA Rx parameters */
	dma_rx_handle.Init.Channel             = SD_DMAx_Rx_CHANNEL;
	dma_rx_handle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
	dma_rx_handle.Init.PeriphInc           = DMA_PINC_DISABLE;
	dma_rx_handle.Init.MemInc              = DMA_MINC_ENABLE;
	dma_rx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
	dma_rx_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
	dma_rx_handle.Init.Mode                = DMA_PFCTRL;
	dma_rx_handle.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
	dma_rx_handle.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
	dma_rx_handle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
	dma_rx_handle.Init.MemBurst            = DMA_MBURST_INC4;
	dma_rx_handle.Init.PeriphBurst         = DMA_PBURST_INC4;

	dma_rx_handle.Instance = SD_DMAx_Rx_STREAM;

	/* Associate the DMA handle */
	__HAL_LINKDMA(hsd, hdmarx, dma_rx_handle);

	/* Deinitialize the stream for new transfer */
	HAL_DMA_DeInit(&dma_rx_handle);

	/* Configure the DMA stream */
	HAL_DMA_Init(&dma_rx_handle);

	/* Configure DMA Tx parameters */
	dma_tx_handle.Init.Channel             = SD_DMAx_Tx_CHANNEL;
	dma_tx_handle.Init.Direction           = DMA_MEMORY_TO_PERIPH;
	dma_tx_handle.Init.PeriphInc           = DMA_PINC_DISABLE;
	dma_tx_handle.Init.MemInc              = DMA_MINC_ENABLE;
	dma_tx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
	dma_tx_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
	dma_tx_handle.Init.Mode                = DMA_PFCTRL;
	dma_tx_handle.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
	dma_tx_handle.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
	dma_tx_handle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
	dma_tx_handle.Init.MemBurst            = DMA_MBURST_INC4;
	dma_tx_handle.Init.PeriphBurst         = DMA_PBURST_INC4;

	dma_tx_handle.Instance = SD_DMAx_Tx_STREAM;

	/* Associate the DMA handle */
	__HAL_LINKDMA(hsd, hdmatx, dma_tx_handle);

	/* Deinitialize the stream for new transfer */
	HAL_DMA_DeInit(&dma_tx_handle);

	/* Configure the DMA stream */
	HAL_DMA_Init(&dma_tx_handle);

	/* NVIC configuration for DMA transfer complete interrupt */
	HAL_NVIC_SetPriority(SD_DMAx_Rx_IRQn, 0x02, 2);
	HAL_NVIC_EnableIRQ(SD_DMAx_Rx_IRQn);

	/* NVIC configuration for DMA transfer complete interrupt */
	HAL_NVIC_SetPriority(SD_DMAx_Tx_IRQn, 0x02, 1);
	HAL_NVIC_EnableIRQ(SD_DMAx_Tx_IRQn);
}

__weak void BSP_SD_Detect_MspInit(SD_HandleTypeDef *hsd, void *Params)
{
	GPIO_InitTypeDef  gpio_init_structure;

	SDIO_DETECT_GPIO_RCC();

	/* GPIO configuration in input for uSD_Detect signal */
	gpio_init_structure.Pin       = SDIO_DETECT_PIN;
	gpio_init_structure.Mode      = GPIO_MODE_INPUT;
	gpio_init_structure.Pull      = GPIO_PULLUP;
	gpio_init_structure.Speed     = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(SDIO_DETECT_GPIO, &gpio_init_structure);
}

__weak void BSP_SD_MspDeInit(SD_HandleTypeDef *hsd, void *Params)
{
    static DMA_HandleTypeDef dma_rx_handle;
    static DMA_HandleTypeDef dma_tx_handle;

    /* Disable NVIC for DMA transfer complete interrupts */
    HAL_NVIC_DisableIRQ(SD_DMAx_Rx_IRQn);
    HAL_NVIC_DisableIRQ(SD_DMAx_Tx_IRQn);

    /* Deinitialize the stream for new transfer */
    dma_rx_handle.Instance = SD_DMAx_Rx_STREAM;
    HAL_DMA_DeInit(&dma_rx_handle);

    /* Deinitialize the stream for new transfer */
    dma_tx_handle.Instance = SD_DMAx_Tx_STREAM;
    HAL_DMA_DeInit(&dma_tx_handle);

    /* Disable NVIC for SDIO interrupts */
    HAL_NVIC_DisableIRQ(SDIO_IRQn);

    /* DeInit GPIO pins can be done in the application
       (by surcharging this __weak function) */

    /* Disable SDIO clock */
    __HAL_RCC_SDIO_CLK_DISABLE();

    /* GPOI pins clock and DMA cloks can be shut down in the applic
       by surcgarging this __weak function */
}

uint8_t BSP_SD_GetCardState(void)
{
	return((HAL_SD_GetCardState(&uSdHandle) == HAL_SD_CARD_TRANSFER ) ? SD_TRANSFER_OK : SD_TRANSFER_BUSY);
}
  
void BSP_SD_GetCardInfo(HAL_SD_CardInfoTypeDef *CardInfo)
{
	HAL_SD_GetCardInfo(&uSdHandle, CardInfo);
}

void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd)
{
	Debug_USART_putn( HAL_SD_GetError(hsd) );
	Debug_USART_putn( hsd->State );
	sd_ll_error_callback();
}
void HAL_SD_AbortCallback(SD_HandleTypeDef *hsd)
{
	sd_ll_abort_callback();
}

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
	sd_ll_write_callback();
}

void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
	sd_ll_read_callback();
}

__weak void BSP_SD_AbortCallback(void)
{
}

__weak void BSP_SD_WriteCpltCallback(void)
{
}

__weak void BSP_SD_ReadCpltCallback(void)
{
}

uint8_t sd_ll_read_audio( uint8_t* pickle_buf
	                    , uint32_t sd_address
	                    , uint32_t num_sd_pages
	                    )
{
	if(HAL_SD_ReadBlocks_DMA( &uSdHandle
		                    , pickle_buf
		                    , sd_address
		                    , num_sd_pages ) != HAL_OK ){
		return MSD_ERROR;
	} else {
		return MSD_OK;
	}
}

uint8_t sd_ll_write_audio( uint8_t* pickle_buf
	                     , uint32_t sd_address
	                     , uint32_t num_sd_pages
	                     )
{
	if(HAL_SD_WriteBlocks_DMA( &uSdHandle
		                     , pickle_buf
		                     , sd_address
		                     , num_sd_pages ) != HAL_OK ){
		return MSD_ERROR;
	} else {
		return MSD_OK;
	}
}

// Interrupt callbacks
void DMA2_Stream3_IRQHandler(void)
{
	HAL_DMA_IRQHandler(uSdHandle.hdmarx);
}

void DMA2_Stream6_IRQHandler(void)
{
	HAL_DMA_IRQHandler(uSdHandle.hdmatx);
}

void SDMMC1_IRQHandler(void)
{
	HAL_SD_IRQHandler(&uSdHandle);
}
