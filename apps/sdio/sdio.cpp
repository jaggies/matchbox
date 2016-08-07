/*
 * hello.cpp
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#include "matchbox.h"
#include "pin.h"

osThreadId defaultTaskHandle;
void StartDefaultTask(void const * argument);

int main(void) {
    MatchBox* mb = new MatchBox();

    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 2048);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    while (1)
        ;
    return 0;
}

const int SDIO_DET = PB8;

void detectCb(uint32_t pin, void* arg) {
    printf("SD DETECT!\n");
}

extern "C" {
uint8_t BSP_SD_Init(void);
uint8_t BSP_SD_ITConfig(void);
void    BSP_SD_DetectIT(void);
void    BSP_SD_DetectCallback(void);
uint8_t BSP_SD_ReadBlocks(uint32_t *pData, uint64_t ReadAddr, uint32_t BlockSize, uint32_t NumOfBlocks);
uint8_t BSP_SD_WriteBlocks(uint32_t *pData, uint64_t WriteAddr, uint32_t BlockSize, uint32_t NumOfBlocks);
uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint64_t ReadAddr, uint32_t BlockSize, uint32_t NumOfBlocks);
uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData, uint64_t WriteAddr, uint32_t BlockSize, uint32_t NumOfBlocks);
uint8_t BSP_SD_Erase(uint64_t StartAddr, uint64_t EndAddr);
void    BSP_SD_IRQHandler(void);
void    BSP_SD_DMA_Tx_IRQHandler(void);
void    BSP_SD_DMA_Rx_IRQHandler(void);
HAL_SD_TransferStateTypedef BSP_SD_GetStatus(void);
void    BSP_SD_GetCardInfo(HAL_SD_CardInfoTypedef *CardInfo);
uint8_t BSP_SD_IsDetected(void);
}

void StartDefaultTask(void const * argument) {
    Pin led(LED_PIN, Pin::Config().setMode(Pin::MODE_OUTPUT));
    Pin det(SDIO_DET, Pin::Config().setMode(Pin::MODE_INPUT).setEdge(Pin::EDGE_FALLING)
            .setPull(Pin::PULL_UP), detectCb, (void*) 0);
    int count = 0;
    HAL_SD_CardInfoTypedef info = {0};
    uint8_t stat = BSP_SD_Init();
    printf("stat=%d\n", stat);
    BSP_SD_GetCardInfo(&info);

    printf("IsDetected: %d\n", BSP_SD_IsDetected());
    printf("Type: %x\n", info.CardType);
    printf("Block size: %d\n", info.CardBlockSize);
    printf("Capacity: %x\n", info.CardCapacity);
    printf("ManufacturerID: %x\n", info.SD_cid.ManufacturerID);
    printf("Mfg date: %04x\n", info.SD_cid.ManufactDate);
    printf("SN: %08x\n", info.SD_cid.ProdSN);

    while (1) {
        led.write(count++ & 1);
        osDelay(250);
    }
}
