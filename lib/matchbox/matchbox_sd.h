/*
 * matchbox_sd.h
 *
 *  Created on: Aug 27, 2016
 *      Author: jmiller
 */
#ifndef __MATCHBOX_SD_H
#define __MATCHBOX_SD_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define SD_CardInfo HAL_SD_CardInfoTypedef

HAL_StatusTypeDef BSP_SD_Init(void);
HAL_StatusTypeDef BSP_SD_ITConfig(void);
HAL_SD_ErrorTypedef BSP_SD_ReadBlocks(uint32_t *data, uint64_t readAddr, uint32_t blockSize,
        uint32_t blockCount);
HAL_SD_ErrorTypedef BSP_SD_WriteBlocks(uint32_t *data, uint64_t writeAddr, uint32_t blockSize,
        uint32_t blockCount);
HAL_SD_ErrorTypedef BSP_SD_ReadBlocks_DMA(uint32_t *data, uint64_t readAddr, uint32_t blockSize,
        uint32_t blockCount);
HAL_SD_ErrorTypedef BSP_SD_WriteBlocks_DMA(uint32_t *data, uint64_t writeAddr, uint32_t blockSize,
        uint32_t blockCount);
HAL_SD_TransferStateTypedef BSP_SD_GetStatus(void);
HAL_SD_ErrorTypedef BSP_SD_Erase(uint64_t startAddr, uint64_t endAddr);
HAL_SD_ErrorTypedef BSP_SD_GetCardInfo(HAL_SD_CardInfoTypedef *cardInfo);
uint8_t BSP_SD_IsDetected(void);

void    BSP_SD_DetectIT(void);
void    BSP_SD_DetectCallback(void);

#ifdef __cplusplus
}
#endif

#endif /* __MATCHBOX_SD_H */
