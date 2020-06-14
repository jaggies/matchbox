/*
 * hello.cpp
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#include <cstdio>
#include <cstring> // memcmmp
#include <cstdlib> // rand()
#include "matchbox.h"
#include "stm32f415_matchbox_sd.h" // low-level BSP testing
#include "pin.h"
#include "util.h"

osThreadId defaultTaskHandle;
void StartDefaultTask(void const * argument);
const int timeout = 1000; // 1s

int main(void) {
    MatchBox* mb = new MatchBox();

    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 1, 2048);
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

void ioTest(const int count, bool useDma) {
    static uint8_t buff[512];
    static uint8_t tmp[sizeof(buff)]; // temporary read buffer for verification
    printf("Test %s DMA", useDma ? "with" : "without");
    for (int block = 0; block < count; block++) {
        memset(buff, block, sizeof(buff));
        char fail = '.';
        if (useDma) {
            if (BSP_SD_WriteBlocks_DMA((uint32_t*)&buff[0], block, 1) == HAL_OK) {
                if (BSP_SD_ReadBlocks_DMA((uint32_t*)&tmp[0], block, 1) == HAL_OK) {
                    if (0 != memcmp(tmp, buff, sizeof(buff))) {
                        fail = 'c';
                    }
                } else {
                    fail = 'r';
                }
            } else {
                fail = 'w';
            }
        } else {
            if (BSP_SD_WriteBlocks((uint32_t*)&buff[0], block, 1, timeout) == HAL_OK) {
                if (BSP_SD_ReadBlocks_DMA((uint32_t*)&tmp[0], block, timeout) == HAL_OK) {
                    if (0 != memcmp(tmp, buff, sizeof(buff))) {
                        fail = 'c';
                    }
                } else {
                    fail = 'r';
                }
            } else {
                fail = 'w';
            }
        }
        if (0 == (block % 64)) {
            printf("\nBlock %08x: ", block);
        }
        printf("%c", fail);
    }
    printf("\n");
}

void StartDefaultTask(void const * argument) {
    Pin led(LED_PIN, Pin::Config().setMode(Pin::MODE_OUTPUT));
    Pin det(SDIO_DET, Pin::Config()
        .setMode(Pin::MODE_INPUT)
        .setEdge(Pin::EDGE_FALLING)
        .setPull(Pin::PULL_UP), detectCb, (void*) 0);
    HAL_SD_CardInfoTypeDef info = { 0 };

    if (!BSP_SD_IsDetected()) {
        printf("No SD card detected\n");
        osDelay(500);
    }

    while (BSP_SD_Init() != MSD_OK) {
        printf("Failed to initialize SD card...\n");
        osDelay(500);
    }

    while(BSP_SD_GetCardInfo(&info) != MSD_OK) {
        printf("Waiting for card info...\n");
        osDelay(500);
    }
    printf("IsDetected: %d\n", BSP_SD_IsDetected());
    printf("Type: %x\n", info.CardType);

    printf("Block size: %d\n", info.BlockSize);
    printf("Capacity: %d (blocks)\n", info.BlockNbr);

    // HAL_SD_GetCardCID(SD_HandleTypeDef *hsd, HAL_SD_CardCIDTypeDef *pCID)
//    printf("ManufacturerID: %x\n", info.SD_cid.ManufacturerID);
//    printf("Mfg date: %04x\n", info.SD_cid.ManufactDate);
//    printf("SN: %08x\n\n", info.SD_cid.ProdSN);

    printf("Starting BSP SDIO tests\n");
    int count = 0;
    int n = 1;
    while (n--) {
        led.write(++count & 1);
        ioTest(64*8, true);
        ioTest(64*8, false);
    }
    printf("done\n");
    while(1)
        ;
}
