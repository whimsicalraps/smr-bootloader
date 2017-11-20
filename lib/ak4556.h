#pragma once

#include <stm32f7xx.h>

#define DSP_SAMPLE_RATE     48000
#define DSP_BLOCK_SIZE		24
//#define AUDIO_BUFF_SIZE		(DSP_BLOCK_SIZE * 2) // flipflop
#define AUDIO_BUFF_SIZE     48

// SAI peripheral configuration defines
#define AUDIO_SAI_A                           SAI1_Block_A
#define AUDIO_SAI_B                           SAI1_Block_B

#define AUDIO_SAI_A_CLK_ENABLE()              __HAL_RCC_SAI1_CLK_ENABLE()
#define AUDIO_SAI_B_CLK_ENABLE()              __HAL_RCC_SAI1_CLK_ENABLE()

#define AUDIO_SAI_A_FS_GPIO_PORT              GPIOE
#define AUDIO_SAI_A_FS_AF                     GPIO_AF6_SAI1
#define AUDIO_SAI_A_FS_PIN                    GPIO_PIN_4

#define AUDIO_SAI_A_SCK_GPIO_PORT             GPIOE
#define AUDIO_SAI_A_SCK_AF                    GPIO_AF6_SAI1
#define AUDIO_SAI_A_SCK_PIN                   GPIO_PIN_5

#define AUDIO_SAI_A_SD_GPIO_PORT              GPIOE
#define AUDIO_SAI_A_SD_AF                     GPIO_AF6_SAI1
#define AUDIO_SAI_A_SD_PIN                    GPIO_PIN_6

#define AUDIO_SAI_A_MCLK_GPIO_PORT            GPIOE
#define AUDIO_SAI_A_MCLK_AF                   GPIO_AF6_SAI1
#define AUDIO_SAI_A_MCLK_PIN                  GPIO_PIN_2

#define AUDIO_SAI_B_SD_GPIO_PORT              GPIOE
#define AUDIO_SAI_B_SD_AF                     GPIO_AF6_SAI1
#define AUDIO_SAI_B_SD_PIN                    GPIO_PIN_3
   
#define AUDIO_SAI_A_MCLK_ENABLE()             __HAL_RCC_GPIOE_CLK_ENABLE()
#define AUDIO_SAI_A_SCK_ENABLE()              __HAL_RCC_GPIOE_CLK_ENABLE()
#define AUDIO_SAI_A_FS_ENABLE()               __HAL_RCC_GPIOE_CLK_ENABLE()
#define AUDIO_SAI_A_SD_ENABLE()               __HAL_RCC_GPIOE_CLK_ENABLE()
#define AUDIO_SAI_B_SD_ENABLE()               __HAL_RCC_GPIOE_CLK_ENABLE()

#define AUDIO_SAI_RESET_GPIO_PORT              GPIOE
#define AUDIO_SAI_RESET_PIN                    GPIO_PIN_1
#define AUDIO_SAI_RESET_ENABLE()              __HAL_RCC_GPIOE_CLK_ENABLE()


void ak4556_Init( uint32_t s_rate );
void ak4556_Start( void );
void HAL_SAI_MspInit(SAI_HandleTypeDef *hsai);
// void DMA2_Stream1_IRQHandler(void);
void DMA2_Stream5_IRQHandler(void);
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai);
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai);

