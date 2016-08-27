/*
 * hello.cpp
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#include <string.h> // memcmmp
#include <stdlib.h> // rand()
#include "matchbox.h"
#include "matchbox_sd.h"
#include "pin.h"
#include "util.h"

osThreadId defaultTaskHandle;
void StartDefaultTask(void const * argument);

#define USE_DMA

#ifdef USE_DMA
#define BSP_SD_WriteBlocks BSP_SD_WriteBlocks_DMA
#define BSP_SD_ReadBlocks BSP_SD_ReadBlocks_DMA
#endif

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

void StartDefaultTask(void const * argument) {
    Pin led(LED_PIN, Pin::Config().setMode(Pin::MODE_OUTPUT));
    Pin det(SDIO_DET,
            Pin::Config().setMode(Pin::MODE_INPUT).setEdge(Pin::EDGE_FALLING).setPull(Pin::PULL_UP),
            detectCb, (void*) 0);
    HAL_SD_CardInfoTypedef info = { 0 };
    HAL_StatusTypeDef halStatus;
    if (!BSP_SD_IsDetected()) {
        printf("No SD card detected\n");
        while (1)
            ;
    }
    if ((halStatus = BSP_SD_Init()) != HAL_OK) {
        printf("Can't init SD card, status = %d\n", halStatus);
        while (1)
            ;
    }
    HAL_SD_ErrorTypedef sdStatus;
    if ((sdStatus = BSP_SD_GetCardInfo(&info)) != SD_OK) {
        printf("Couldn't get card info, status=%d\n", sdStatus);
        while (1)
            ;
    }
    printf("IsDetected: %d\n", BSP_SD_IsDetected());
    printf("Type: %x\n", info.CardType);
    printf("Block size: %d\n", info.CardBlockSize);
    printf("Capacity: %x\n", info.CardCapacity);
    printf("ManufacturerID: %x\n", info.SD_cid.ManufacturerID);
    printf("Mfg date: %04x\n", info.SD_cid.ManufactDate);
    printf("SN: %08x\n\n", info.SD_cid.ProdSN);
#ifdef USE_DMA
    printf("Starting BSP SDIO tests (DMA)\n");
#else
    printf("Starting BSP SDIO tests\n");
#endif
    int count = 0;
    while (1) {
        uint8_t buff[512];
        HAL_SD_ErrorTypedef status;
        uint8_t tmp[sizeof(buff)]; // temporary read buffer for verification
        for (int i = 0; i < sizeof(buff); i++) {
            buff[i] = rand() & 0xff;
        }
        if ((sdStatus = BSP_SD_WriteBlocks((uint32_t*)&buff[0], count * 512, 512, 1)) != SD_OK) {
            printf("write block %d failed with status=%d\n", count, sdStatus);
            osDelay(1000);
        }
        if ((sdStatus = BSP_SD_ReadBlocks((uint32_t*)&tmp[0], count * 512, 512, 1)) != SD_OK) {
            printf("read block %d failed with status=%d\n", count, sdStatus);
            osDelay(1000);
        }
        if (0 != memcmp(tmp, buff, sizeof(buff))) {
            error("*** readback error: blocks differ! ***\n");
            osDelay(1000);
        } else {
            if (!(count % 64)) {
                printf("\n");
                printf("Block %08x", count);
            } else {
                printf(".");
            }
        }
        led.write(count++ & 1);
    }
}
