/*
 * hello.cpp
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#include <stddef.h>
#include <cassert>
#include <cstdio>
#include <cstdlib> // rand()
#include <cstring> // memcmmp
#include <cassert>
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

bool readBlock(void* data, uint32_t block) {
    uint8_t status;
    debug("%s:\n", __func__);
//    assert(0 == (((size_t)data) & 0x03)); // must be 32-bit aligned
    while ((status = BSP_SD_ReadBlocks_DMA((uint32_t*) data, block, 1)) == HAL_BUSY) {
        debug("\tbusy\n");
    }

    // Wait for it to complete
    if (status == MSD_OK) {
        while (BSP_SD_GetCardState() != SD_TRANSFER_OK) {
            osThreadYield();
        }
    }

    if (status != MSD_OK) {
        error("%\tfailed, status = %d\n", status);
        return false;
    }

    debug("\tcomplete\n");
    return true;
}

bool writeBlock(void* data, uint32_t block) {
    uint8_t status;
    debug("%s:\n", __func__);
    while ((status = BSP_SD_WriteBlocks_DMA((uint32_t*) data, block, 1))== HAL_BUSY) {
        debug("\tbusy\n");
    }

    // Wait for it to complete
    if (status == MSD_OK) {
        while (BSP_SD_GetCardState() != SD_TRANSFER_OK) {
            osThreadYield();
        }
    }
    if (status != MSD_OK) {
        error("\tfailed, status = %d\n",status);
        return false;
    }

    debug("\tcomplete\n");

    return true;
}

void dumpBlock(uint8_t* data, int block) {
    for (int i = 0; i < 512; i++) {
        if ((i % 16) == 0) {
            debug("%08x: ", block * 512 + i);
        }
        debug("%02x", data[i]);
        if ((i % 16) == 15) {
            debug(" ");
            for (int j = 15; j > 0; j--) {
                int ch = data[i - j];
                ch = ch < 32 || ch > 127 ? '.' : ch;
                printf("%c", ch);
            }
            debug((i % 16) == 15 ? "\n" : " ");
        }
    }
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
        while (!BSP_SD_IsDetected())
            ;
    }

    if (BSP_SD_Init() != MSD_OK) {
        printf("Waiting to initialize SD card");
        while (BSP_SD_Init() != MSD_OK) {
            printf(".");
            osDelay(500);
        }
    }

    if(BSP_SD_GetCardInfo(&info) != MSD_OK) {
        printf("Waiting for card info...\n");
        while (BSP_SD_GetCardInfo(&info) != MSD_OK) {
            printf(".");
            osDelay(500);
        }
    }
    printf("Type: %x\n", info.CardType);
    printf("Block size: %d\n", info.BlockSize);
    printf("Capacity: %dMB\n", info.BlockNbr / (1024*1024 / info.BlockSize)); // Nb * mb/block = mb
    printf("Card type: %d\n", info.CardType);

    printf("Starting BSP SDIO tests\n");
    int block = 0;
    char buff[512];
    printf("Buff = %p\n", &buff[0]);
    while (1) {
        for (int i = 0; i < sizeof(buff); i++) {
            buff[i] = rand() & 0xff;
        }

        // TODO: Is there a better way to get this info?
        while (BSP_SD_GetCardState() == SD_TRANSFER_BUSY) {
            debug("%s: card not ready\n", __func__);
        }

        char fail = '.'; // no failure
        if (writeBlock(&buff[0], block)) {
            char tmp[sizeof(buff)]; // temporary read buffer, for verification
            if (readBlock(&tmp[0], block)) {
                if (0 != memcmp(tmp, buff, sizeof(buff))) {
                    fail = 'c'; // failed to compare
                }
            } else {
                fail = 'r'; // failed to read
            }
        } else {
            fail = 'w'; // failed to write
        }

        if (!(block % 64)) {
            printf("\n"); // bug in printf requires a separate call for this
            printf("Block %08x: ", block);
        }

        printf("%c", fail);

        //dumpBlock(&buff[0], count);
        led.write(block++ & 1);
    }
}
