#pragma once

#include "stm32f769i_discovery.h"

// #define SDMMC_TRANSFER_CLK_DIV ((uint8_t)0x2)

#define thisMMC               SDMMC1
#define SDIO_P_CLK()          __HAL_RCC_SDMMC1_CLK_ENABLE()
#define SDIO_P_CLK_DISABLE()  __HAL_RCC_SDMMC1_CLK_DISABLE()

#define SDIO_AF               GPIO_AF12_SDMMC1
#define SDIO_MAIN_GPIO_RCC()  __HAL_RCC_GPIOC_CLK_ENABLE()
#define SDIO_MAIN_GPIO        GPIOC
#define SDIO_D0_PIN           GPIO_PIN_8
#define SDIO_D1_PIN           GPIO_PIN_9
#define SDIO_D2_PIN           GPIO_PIN_10
#define SDIO_D3_PIN           GPIO_PIN_11
#define SDIO_CLK_PIN          GPIO_PIN_12

#define SDIO_CMD_GPIO_RCC()   __HAL_RCC_GPIOD_CLK_ENABLE()
#define SDIO_CMD_GPIO         GPIOD
#define SDIO_CMD_PIN          GPIO_PIN_2

#define SDIO_DETECT_GPIO_RCC()  __HAL_RCC_GPIOB_CLK_ENABLE()
#define SDIO_DETECT_GPIO      GPIOB
#define SDIO_DETECT_PIN       GPIO_PIN_12

// SD Card information structure
#define BSP_SD_CardInfo HAL_SD_CardInfoTypeDef

// SD status structure definition
#define   MSD_OK                        ((uint8_t)0x00)
#define   MSD_ERROR                     ((uint8_t)0x01)
#define   MSD_ERROR_SD_NOT_PRESENT      ((uint8_t)0x02)

// SD transfer state definition
#define   SD_TRANSFER_OK                ((uint8_t)0x00)
#define   SD_TRANSFER_BUSY              ((uint8_t)0x01)
   
// Exported_Constants SD Exported Constants
#define SD_PRESENT               ((uint8_t)0x01)
#define SD_NOT_PRESENT           ((uint8_t)0x00)
#define SD_DATATIMEOUT           ((uint32_t)10000)

/* DMA definitions for SD DMA transfer */
#define __DMAx_TxRx_CLK_ENABLE            __HAL_RCC_DMA2_CLK_ENABLE
#define SD_DMAx_Tx_CHANNEL                DMA_CHANNEL_4
#define SD_DMAx_Rx_CHANNEL                DMA_CHANNEL_4
#define SD_DMAx_Tx_STREAM                 DMA2_Stream6
#define SD_DMAx_Rx_STREAM                 DMA2_Stream3
#define SD_DMAx_Tx_IRQn                   DMA2_Stream6_IRQn
#define SD_DMAx_Rx_IRQn                   DMA2_Stream3_IRQn

#define BSP_SDMMC_IRQHandler              SDMMC1_IRQHandler   
#define BSP_SDMMC_DMA_Tx_IRQHandler       DMA2_Stream6_IRQHandler
#define BSP_SDMMC_DMA_Rx_IRQHandler       DMA2_Stream3_IRQHandler
#define SD_DetectIRQHandler()             HAL_GPIO_EXTI_IRQHandler(SD_DETECT_PIN)

uint8_t BSP_SD_Init(void);
uint8_t BSP_SD_DeInit(void);
uint8_t BSP_SD_ITConfig(void);
uint8_t BSP_SD_ReadBlocks(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks, uint32_t Timeout);
uint8_t BSP_SD_WriteBlocks(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks, uint32_t Timeout);
uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks);
uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks);
uint8_t BSP_SD_Erase(uint32_t StartAddr, uint32_t EndAddr);
uint8_t BSP_SD_GetCardState(void);
void    BSP_SD_GetCardInfo(HAL_SD_CardInfoTypeDef *CardInfo);
uint8_t BSP_SD_IsDetected(void);

/* These functions can be modified in case the current settings (e.g. DMA stream)
   need to be changed for specific application needs */
void    BSP_SD_MspInit(SD_HandleTypeDef *hsd, void *Params);
void    BSP_SD_Detect_MspInit(SD_HandleTypeDef *hsd, void *Params);
void    BSP_SD_MspDeInit(SD_HandleTypeDef *hsd, void *Params);
// void    BSP_SD_ErrorCallback(void);
void    BSP_SD_AbortCallback(void);
void    BSP_SD_WriteCpltCallback(void);
void    BSP_SD_ReadCpltCallback(void);

void    sd_ll_error_callback(void);
void    sd_ll_abort_callback(void);

uint8_t WR_SD_Read(uint8_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks);
void    sd_ll_read_callback(void);
uint8_t WR_SD_Write(uint8_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks);
void    sd_ll_write_callback(void);

// formal i/o functions for sd handling
uint8_t sd_ll_read_audio( uint8_t* pickle_buf
                        , uint32_t sd_address
                        , uint32_t num_sd_pages
                        );
uint8_t sd_ll_write_audio( uint8_t* pickle_buf
                         , uint32_t sd_address
                         , uint32_t num_sd_pages
                         );

void DMA2_Stream3_IRQHandler(void);
void DMA2_Stream6_IRQHandler(void);
void SDMMC1_IRQHandler(void);
